
#include "stdafx.h"
#include "AlbumModel.h"
#include "SearchPattern.h"
#include "FileAttr.h"
#include "FileAttrAlgorithms.h"
#include "ImageFileEnumerator.h"
#include "ProgressService.h"
#include "Workspace.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/RuntimeException.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Resequence.hxx"


CAlbumModel::CAlbumModel( void )
	: m_modelSchema( app::Slider_LatestModelSchema )
	, m_persistFlags( 0 )
	, m_randomSeed( ::GetTickCount() )
	, m_fileOrder( fattr::OriginalOrder )
{
	SetPersistFlag( UseDeepStreamPaths, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) );		// copy flag from worksoace to keep track of storage saving structure
}

CAlbumModel::~CAlbumModel()
{
}

void CAlbumModel::Clear( void )
{
	m_searchModel.ClearPatterns();
	m_imagesModel.Clear();

	CloseAllStorages();
}

ICatalogStorage* CAlbumModel::GetCatalogStorage( void ) const
{
	if ( !m_docStgPath.IsEmpty() )
		return m_storageHost.Find( m_docStgPath );

	return NULL;
}

void CAlbumModel::OpenAllStorages( void )
{
	if ( !m_docStgPath.IsEmpty() )										// catalog-based album?
		m_storageHost.Push( m_docStgPath, MainStorage );

	std::vector< fs::CPath > subStoragePaths;
	QueryEmbeddedStorages( subStoragePaths );

	m_storageHost.PushMultiple( subStoragePaths, EmbeddedStorage );		// open embedded storages
}

void CAlbumModel::QueryEmbeddedStorages( std::vector< fs::CPath >& rSubStoragePaths ) const
{
	rSubStoragePaths = m_imagesModel.GetStoragePaths();
	m_searchModel.AugmentStoragePaths( rSubStoragePaths );		// add embedded storages in search model
}

void CAlbumModel::CloseAllStorages( void )
{
	m_storageHost.Clear();
}

void CAlbumModel::StoreCatalogDocPath( const fs::CPath& docStgPath )
{
	REQUIRE( app::IsCatalogFile( docStgPath.GetPtr() ) );
	REQUIRE( fs::IsValidStructuredStorage( docStgPath.GetPtr() ) );

	REQUIRE( m_searchModel.IsEmpty() );			// no mixing with .sld album model

	m_docStgPath = docStgPath;
}

bool CAlbumModel::SetupSingleSearchPattern( CSearchPattern* pSearchPattern )
{
	ASSERT_PTR( pSearchPattern );

	m_searchModel.ClearPatterns();
	m_docStgPath.Clear();
	m_imagesModel.ClearInvalidStoragePaths();

	if ( !pSearchPattern->IsValidPath() )
	{
		delete pSearchPattern;
		return false;
	}

	m_searchModel.AddPattern( pSearchPattern );
	return true;
}

void CAlbumModel::SearchForFiles( CWnd* pParentWnd ) throws_( CException* )
{
	std::auto_ptr< fattr::CRetainFileOrder > pRetainCustomOrder;

	if ( fattr::CustomOrder == m_fileOrder )
		pRetainCustomOrder.reset( new fattr::CRetainFileOrder( m_imagesModel.GetFileAttrs() ) );

	CProgressService progress( pParentWnd );
	CImagesModel foundImagesModel;

	{	// scope for "Searching..." progress
		CImageFileEnumerator imageEnum( progress.GetProgressEnumerator() );

		imageEnum.SetMaxFiles( m_searchModel.GetMaxFileCount() );
		imageEnum.SetFileSizeFilter( m_searchModel.GetFileSizeRange() );

		try
		{
			if ( !m_docStgPath.IsEmpty() )
				imageEnum.SearchCatalogStorage( m_docStgPath );
			else
				imageEnum.Search( m_searchModel.GetPatterns() );
		}
		catch ( CUserAbortedException& exc )
		{
			app::TraceException( exc );		// cancelled by the user: keep the images found so far
		}

		imageEnum.SwapFoundImages( foundImagesModel );
	}

	ui::IProgressService* pProgressSvc = progress.GetService();
	pProgressSvc->GetHeader()->SetOperationLabel( _T("Order Images") );
	pProgressSvc->GetHeader()->SetStageLabel( str::GetEmpty() );
	pProgressSvc->SetMarqueeProgress();

	switch ( m_fileOrder )
	{
		case fattr::OriginalOrder:
			break;			// nothing to do: found files are already in original order
		case fattr::CustomOrder:
			pRetainCustomOrder->RestoreOriginalOrder( &foundImagesModel.RefFileAttrs() );
			break;
		default:
			DoOrderImagesModel( &foundImagesModel, pProgressSvc );
	}

	progress.DestroyDialog();

	// commit the transaction
	std::vector< fs::CPath > oldStoragePaths = m_imagesModel.GetStoragePaths();		// baseline: store old embedded storages

	m_imagesModel.Swap( foundImagesModel );
	m_storageHost.ModifyMultiple( m_imagesModel.GetStoragePaths(), oldStoragePaths );
}

bool CAlbumModel::DoOrderImagesModel( CImagesModel* pImagesModel, ui::IProgressService* pProgressSvc )
{
	ASSERT_PTR( pImagesModel );

	switch ( m_fileOrder )
	{
		case fattr::CustomOrder:
			return false;
		case fattr::Shuffle:
			m_randomSeed = ::GetTickCount();	// randomly init the random seed (with current tick count)
			// fall-through
		case fattr::ShuffleSameSeed:
			::srand( m_randomSeed % 0x7FFF );	// randomize on m_randomSeed - note: the client of the randomization with the seed is images model (for shuffling)
			break;
	}

	pImagesModel->OrderFileAttrs( m_fileOrder, pProgressSvc );
	return true;
}

void CAlbumModel::Stream( CArchive& archive )
{
	ASSERT( !archive.IsLoading() || m_modelSchema == app::GetLoadingSchema( archive ) );		// loading doc schema was stored before?

	if ( archive.IsLoading() && m_modelSchema < app::Slider_v4_2 )
	{	// backwards-compatible for app::Slider_v4_1 -
		archive >> (int&)m_fileOrder;
		archive >> m_randomSeed;
		archive >> m_searchModel.m_fileSizeRange;
		archive >> m_persistFlags;

		serial::StreamOwningPtrs( archive, m_searchModel.m_patterns );
	}
	else
	{
		m_searchModel.Stream( archive );

		if ( archive.IsStoring() )
		{
			archive << (int)m_fileOrder;
			archive << m_randomSeed;
			archive << m_persistFlags;
		}
		else
		{
			archive >> (int&)m_fileOrder;
			archive >> m_randomSeed;
			archive >> m_persistFlags;
		}
	}

	m_imagesModel.Stream( archive );

	if ( !archive.IsStoring() )
		if ( MustAutoRegenerate() )
			try
			{
				SearchForFiles( NULL );			// regenerate the file list on archive load
			}
			catch ( CException* pExc )
			{
				app::HandleReportException( pExc );
			}
}


bool CAlbumModel::ShouldUseDeepStreamPaths( void )
{
	return HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames );
}

bool CAlbumModel::IsAutoDropRecipient( bool checkValidPath /*= true*/ ) const
{
	if ( const CSearchPattern* pSinglePattern = m_searchModel.GetSinglePattern() )
		return pSinglePattern->IsAutoDropDirPath( checkValidPath );

	return false;
}

void CAlbumModel::QueryFileAttrsSequence( std::vector< CFileAttr* >& rSequence, const std::vector< int >& selIndexes ) const
{
	rSequence.clear();
	rSequence.reserve( selIndexes.size() );

	for ( std::vector< int >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
		rSequence.push_back( const_cast< CFileAttr* >( GetFileAttr( *itSelIndex ) ) );
}

// returns the display index of the found file
int CAlbumModel::FindIndexFileAttrWithPath( const fs::CPath& filePath ) const
{
	return static_cast<int>( fattr::FindPosWithPath( m_imagesModel.GetFileAttrs(), filePath ) );
}

bool CAlbumModel::ModifyFileOrder( fattr::Order fileOrder )
{
	StoreFileOrder( fileOrder );
	return DoOrderImagesModel( &m_imagesModel, ui::CNoProgressService::Instance() );
}

void CAlbumModel::SetCustomOrderSequence( const std::vector< CFileAttr* >& customSequence )
{
	m_fileOrder = fattr::CustomOrder;

	ASSERT( utl::SameContents( m_imagesModel.RefFileAttrs(), customSequence ) );
	m_imagesModel.RefFileAttrs() = customSequence;
}


/** SERVICE API **/

TCurrImagePos CAlbumModel::DeleteFromAlbum( const std::vector< fs::CFlexPath >& selFilePaths )
{
	std::vector< size_t > indexes;
	indexes.reserve( selFilePaths.size() );

	for ( std::vector< fs::CFlexPath >::const_iterator itImagePath = selFilePaths.begin(); itImagePath != selFilePaths.end(); ++itImagePath )
	{
		size_t foundPos = m_imagesModel.FindPosFileAttr( *itImagePath );
		if ( foundPos != utl::npos )
			indexes.push_back( foundPos );
		else
			ASSERT( false );			// programming error?
	}

	if ( indexes.empty() )
		return -1;						// no change

	for ( size_t i = indexes.size(); i-- != 0; )
		m_imagesModel.RemoveFileAttrAt( indexes[ i ] );

	// reset remaining attributes order as original (since the old baseline positions got invalidated)
	m_imagesModel.StoreBaselineSequence();
	StoreFileOrder( fattr::OriginalOrder );

	size_t newSelPos = std::min( indexes.front(), m_imagesModel.GetFileAttrs().size() - 1 );
	return static_cast< TCurrImagePos >( newSelPos );
}


// Assumes that rSelIndexes is sorted ascending.
// At input:
//	- rDropIndex: the drop position;
//	- rSelIndexes: the drag selected display indexes;
// At return:
//	- rDropIndex: the new dropped position (after dropping);
//	- rSelIndexes: the dropped display indexes;
// Returns true if any display indexes were actually moved, otherwise false
bool CAlbumModel::DropCustomOrderIndexes( int& rDropIndex, std::vector< int >& rSelIndexes )
{
	// switch to custom order if not already
	if ( m_fileOrder != fattr::CustomOrder )
		StoreFileOrder( fattr::CustomOrder );

	std::vector< CFileAttr* > newSequence;
	std::vector< int > droppedSelIndexes;		// will be adjusted after dropping

	rDropIndex = seq::MakeDropSequence( newSequence, m_imagesModel.GetFileAttrs(), rDropIndex, rSelIndexes, &droppedSelIndexes );

	if ( droppedSelIndexes == rSelIndexes )
		return false;			// nothing has changed after drop

	m_imagesModel.RefFileAttrs().swap( newSequence );
	rSelIndexes.swap( droppedSelIndexes );
	return true;
}

bool CAlbumModel::UndropCustomOrderIndexes( int droppedIndex, const std::vector< int >& origDragSelIndexes )
{
	ASSERT( IsCustomOrder() );

	// undo (reverse) a drop operation - original means before drop
	std::vector< CFileAttr* > origSequence;
	int origDropIndex = seq::MakeUndoDropSequence( origSequence, m_imagesModel.GetFileAttrs(), droppedIndex, origDragSelIndexes );

	m_imagesModel.RefFileAttrs().swap( origSequence );

	TRACE( _T(" Undo drop: origDropIndex=%d  %s\n"), origDropIndex, str::FormatSet( origDragSelIndexes ).c_str() ); origDropIndex;
	return true;
}
