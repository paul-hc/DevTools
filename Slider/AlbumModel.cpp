
#include "stdafx.h"
#include "AlbumModel.h"
#include "SearchSpec.h"
#include "FileAttr.h"
#include "FileAttrAlgorithms.h"
#include "ImageArchiveStg.h"
#include "ImageFileEnumerator.h"
#include "ProgressService.h"
#include "Workspace.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/ComparePredicates.h"
#include "utl/PathMaker.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/ScopedValue.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/Resequence.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	// StripStgDocPrefix implementation

	void StripStgDocPrefix::operator()( CFileAttr* pFileAttr ) const
	{
		ASSERT_PTR( pFileAttr );
		operator()( pFileAttr->RefPath() );
	}


	// MakeComplexPath implementation

	void MakeComplexPath::operator()( CFileAttr* pFileAttr ) const
	{
		ASSERT_PTR( pFileAttr );
		operator()( pFileAttr->RefPath() );
	}
}


// CAlbumModel implementation

CAlbumModel::CAlbumModel( void )
	: m_modelSchema( app::Slider_LatestModelSchema )
	, m_persistFlags( 0 )
	, m_randomSeed( ::GetTickCount() )
	, m_fileOrder( fattr::OriginalOrder )
{
}

CAlbumModel::~CAlbumModel()
{
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

		serial::StreamOwningPtrs( archive, m_searchModel.m_searchSpecs );
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
				SearchForFiles( NULL, false );			// regenerate the file list on archive load
			}
			catch ( CException* pExc )
			{
				app::HandleReportException( pExc );
			}
}

bool CAlbumModel::SetupSearchPath( const fs::CPath& searchPath )
{
	if ( !searchPath.FileExist() )
		return false;

	m_searchModel.ClearSpecs();
	m_stgDocPath.Clear();

	if ( fs::IsValidDirectory( searchPath.GetPtr() ) )
		m_searchModel.AddSearchPath( searchPath );
	else if ( app::IsImageArchiveDoc( searchPath.GetPtr() ) )
		m_stgDocPath = searchPath;
	else
		return false;

	return true;
}


bool CAlbumModel::IsAutoDropRecipient( bool checkValidPath /*= true*/ ) const
{
	if ( const CSearchSpec* pSingleSpec = m_searchModel.GetSingleSpec() )
		return pSingleSpec->IsAutoDropDirPath( checkValidPath );

	return false;
}

// release the image storages associated with this (if any), as well as the ones found in the generation process
void CAlbumModel::CloseAssocImageArchiveStgs( void )
{
	if ( !m_stgDocPath.IsEmpty() )
		CImageArchiveStg::Factory().ReleaseStorage( m_stgDocPath );

	for ( std::vector< CSearchSpec* >::const_iterator itSearch = m_searchModel.GetSpecs().begin(); itSearch != m_searchModel.GetSpecs().end(); ++itSearch )
		if ( ( *itSearch )->IsImageArchiveDoc() )
			CImageArchiveStg::Factory().ReleaseStorage( ( *itSearch )->GetFilePath() );

	m_imagesModel.Clear();
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
	size_t foundPos = fattr::FindPosWithPath( m_imagesModel.GetFileAttrs(), filePath );
	if ( utl::npos == foundPos )
		return -1;
	return static_cast<int>( foundPos );
}

bool CAlbumModel::HasConsistentDeepStreams( void ) const
{
	return HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) == HasPersistFlag( UseDeepStreamPaths );
}

bool CAlbumModel::CheckReparentFileAttrs( const TCHAR* pDocPath, PersistOp op )
{
	fs::CPath docPath = fs::CFlexPath( pDocPath ).GetPhysicalPath();			// extract "C:\Images\storage.ias" from "C:\Images\storage.ias>_Album.sld"

	if ( app::IsImageArchiveDoc( docPath.GetPtr() ) )
		return ReparentStgFileAttrsImpl( docPath, op );

	return false;
}

bool CAlbumModel::ReparentStgFileAttrsImpl( const fs::CPath& stgDocPath, PersistOp op )
{
	ASSERT( app::IsImageArchiveDoc( stgDocPath.GetPtr() ) );

	if ( stgDocPath != m_stgDocPath )								// different stgDocPath, need to store and reparent
	{
		m_searchModel.ClearSpecs();
		m_stgDocPath = stgDocPath;

		// clear the common prefix to the actual logical root of the album
		std::vector< CFileAttr* >& rFileAttrs = m_imagesModel.RefFileAttrs();
		std::for_each( rFileAttrs.begin(), rFileAttrs.end(), func::StripStgDocPrefix() );		// clear stg prefix

		if ( !HasFlag( m_persistFlags, UseDeepStreamPaths ) )
			if ( !m_imagesModel.IsEmpty() )
			{
				CPathMaker maker;
				maker.StoreSrcFromPaths( rFileAttrs );
				if ( maker.MakeDestStripCommonPrefix() )			// convert to relative paths based on common prefix
					maker.QueryDestToPaths( rFileAttrs );			// found and removed the common prefix
			}

		// reparent with the new doc stg
		std::for_each( rFileAttrs.begin(), rFileAttrs.end(),
			func::MakeComplexPath( m_stgDocPath, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) ? func::Deep : func::Flat ) );
	}

	if ( Saving == op )
		SetPersistFlag( UseDeepStreamPaths, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) );			// will save to keep track of saved mode

	return true;
}

void CAlbumModel::SetCustomOrderSequence( const std::vector< CFileAttr* >& customSequence )
{
	m_fileOrder = fattr::CustomOrder;

	ASSERT( utl::SameContents( m_imagesModel.RefFileAttrs(), customSequence ) );
	m_imagesModel.RefFileAttrs() = customSequence;
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

void CAlbumModel::SearchForFiles( CWnd* pParentWnd, bool reportEmpty /*= true*/ ) throws_( CException* )
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
			if ( !m_stgDocPath.IsEmpty() )
				imageEnum.SearchImageArchive( m_stgDocPath );
			else
				imageEnum.Search( m_searchModel.GetSpecs() );
		}
		catch ( CUserAbortedException& exc )
		{
			app::TraceException( exc );		// cancelled by the user: keep the images found so far
		}

		imageEnum.SwapFoundImages( foundImagesModel );

		if ( reportEmpty && !imageEnum.GetIssueStore().IsEmpty() )
			app::GetUserReport().ReportIssues( imageEnum.GetIssueStore(), MB_OK | MB_ICONEXCLAMATION );
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

	m_imagesModel.Swap( foundImagesModel );		// commit the transaction
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
			::srand( m_randomSeed % 0x7FFF );	// randomize on m_randomSeed - note: the client of the randomization with the seed is CImagesModel (for shuffling)
			break;
	}

	pImagesModel->OrderFileAttrs( m_fileOrder, pProgressSvc );
	return true;
}

bool CAlbumModel::ModifyFileOrder( fattr::Order fileOrder )
{
	StoreFileOrder( fileOrder );
	return DoOrderImagesModel( &m_imagesModel, ui::CNullProgressService::Instance() );
}
