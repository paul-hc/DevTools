
#include "stdafx.h"
#include "AlbumModel.h"
#include "SearchSpec.h"
#include "ImageArchiveStg.h"
#include "ImageFileEnumerator.h"
#include "Workspace.h"
#include "Application.h"
#include "resource.h"
#include "utl/ComparePredicates.h"
#include "utl/EnumTags.h"
#include "utl/PathMaker.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/ScopedValue.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	// if custom order is used, preserve the existing order:
	//	 - append the new files (last in custom order).
	//	 - remove from m_customOrder vector entries of files that are no longer in m_fileAttributes snapshot.
	//
	class CRetainCustomOrder
	{
	public:
		CRetainCustomOrder( const std::vector< CFileAttr >* pFileAttributes, const std::vector< int >* pCustomOrder );

		void RestoreInitialOrder( std::vector< CFileAttr >* pFileAttributes, std::vector< int >* pCustomOrder );
	private:
		// initial order
		std::map< fs::CFlexPath, int > m_pathToDispIndexMap;
		std::vector< fs::CFlexPath > m_orderedFilePaths;
	};
}


// CAlbumModel implementation

CAlbumModel::CAlbumModel( void )
	: m_modelSchema( app::Slider_LatestModelSchema )
	, m_inGeneration( false )
	, m_persistFlags( 0 )
	, m_randomSeed( ::GetTickCount() )
	, m_fileOrder( OriginalOrder )
{
}

CAlbumModel::~CAlbumModel()
{
}

const CEnumTags& CAlbumModel::GetTags_Order( void )
{
	static const CEnumTags tags
	(
		_T("Original order|User-defined order|Random shuffle|Random shuffle on same seed|")
		_T("File Name|(File Name)|Folder Name|(Folder Name)|File Size|(File Size)|")
		_T("File Date|(File Date)|Image Dimensions|(Image Dimensions)|")
		_T("Images with same file size|Images with same file size and dimensions|Corrupted files")
	);
	return tags;
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

	serial::StreamItems( archive, m_fileAttributes );
	serial::SerializeValues( archive, m_archiveStgPaths );
	serial::SerializeValues( archive, m_customOrder );

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


bool CAlbumModel::CancelGeneration( void )
{
	if ( !m_inGeneration )
		return false;

	m_inGeneration = false;
	return true;
}

// release the image storages associated with this (if any), as well as the ones found in the generation process
void CAlbumModel::CloseAssocImageArchiveStgs( void )
{
	if ( !m_stgDocPath.IsEmpty() )
		CImageArchiveStg::Factory().ReleaseStorage( m_stgDocPath );

	for ( std::vector< CSearchSpec* >::const_iterator itSearch = m_searchModel.GetSpecs().begin(); itSearch != m_searchModel.GetSpecs().end(); ++itSearch )
		if ( ( *itSearch )->IsImageArchiveDoc() )
			CImageArchiveStg::Factory().ReleaseStorage( ( *itSearch )->GetFilePath() );

	ClearArchiveStgPaths();
}

size_t CAlbumModel::FindPosFileAttr( const CFileAttr* pFileAttr ) const
{
	for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
		if ( pFileAttr == &m_fileAttributes[ pos ] )
			return pos;

	ASSERT( false );			// pFileAttr should be from within m_fileAttributes
	return utl::npos;
}

int CAlbumModel::DisplayToTrueFileIndex( size_t displayIndex ) const
{
	// translate the display index if we have custom order
	if ( CustomOrder == m_fileOrder )
	{
		ASSERT( m_customOrder.size() == m_fileAttributes.size() );
		return m_customOrder[ displayIndex ];
	}
	else
		ASSERT( m_customOrder.empty() );

	return (int)displayIndex;
}

void CAlbumModel::QueryFileAttrs( std::vector< CFileAttr* >& rFileAttrs ) const
{
	rFileAttrs.clear();
	rFileAttrs.reserve( m_fileAttributes.size() );
	for ( size_t i = 0; i != m_fileAttributes.size(); ++i )
		rFileAttrs.push_back( const_cast< CFileAttr* >( &GetFileAttr( i ) ) );
}

// returns the display index of the found file
int CAlbumModel::FindFileAttr( const fs::CFlexPath& filePath, bool wantDisplayIndex /*= true*/ ) const
{
	if ( !IsCustomOrder() )
		wantDisplayIndex = false;

	for ( UINT i = 0; i != m_fileAttributes.size(); ++i )
	{
		const CFileAttr& fileAttr = m_fileAttributes[ wantDisplayIndex ? m_customOrder[ i ] : i ];
		if ( filePath == fileAttr.GetPath() )
			return i;
	}
	return -1;
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
		std::for_each( m_fileAttributes.begin(), m_fileAttributes.end(), func::StripStgDocPrefix() );		// clear stg prefix

		if ( !HasFlag( m_persistFlags, UseDeepStreamPaths ) )
			if ( !m_fileAttributes.empty() )
			{
				CPathMaker maker;
				maker.StoreSrcFromPaths( m_fileAttributes );
				if ( maker.MakeDestStripCommonPrefix() )				// convert to relative paths based on common prefix
					maker.QueryDestToPaths( m_fileAttributes );			// found and removed the common prefix
			}

		// reparent with the new doc stg
		std::for_each( m_fileAttributes.begin(), m_fileAttributes.end(),
			func::MakeComplexPath( m_stgDocPath, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) ? func::Deep : func::Flat ) );
	}

	if ( Saving == op )
		SetPersistFlag( UseDeepStreamPaths, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) );	// will save to keep track of saved mode

	return true;
}

void CAlbumModel::SetFileOrder( Order fileOrder )
{
	if ( CustomOrder == fileOrder && m_fileOrder != CustomOrder )
	{	// reset custom order to the initial order
		m_customOrder.resize( m_fileAttributes.size() );

		for ( size_t i = 0; i != m_customOrder.size(); ++i )
			m_customOrder[ i ] = static_cast< int >( i );
	}

	m_fileOrder = fileOrder;

	if ( m_fileOrder != CustomOrder )
		m_customOrder.clear();
}

void CAlbumModel::SetCustomOrder( const std::vector< CFileAttr* >& customOrder )
{
	m_fileOrder = CustomOrder;
	m_customOrder.resize( m_fileAttributes.size() );

	for ( size_t i = 0; i != customOrder.size(); ++i )
		m_customOrder[ i ] = static_cast< int >( FindPosFileAttr( customOrder[ i ] ) );
}

// Assumes that rToMoveIndexes is sorted ascending.
// At return:
//	- rToDestIndex: contains the moved position of the insertion index;
//	- rToMoveIndexes: contains the moved display indexes;
// Returns true if any display indexes were actually moved, otherwise false
bool CAlbumModel::MoveCustomOrderIndexes( int& rToDestIndex, std::vector< int >& rToMoveIndexes )
{
	// switch to custom order if not already
	if ( m_fileOrder != CustomOrder )
		SetFileOrder( CustomOrder );

	// if toDestIndex == m_customOrder.size() then append indexes to move
	ASSERT( rToDestIndex >= 0 && rToDestIndex <= (int)m_customOrder.size() );

	ASSERT( m_fileAttributes.size() == m_customOrder.size() );
	ASSERT( rToMoveIndexes.size() > 0 && rToMoveIndexes.size() < m_customOrder.size() );
	ASSERT( rToDestIndex >= 0 && rToDestIndex <= (int)m_customOrder.size() );

	// copy the true indexes at the custom order indexes specified in rToMoveIndexes
	std::vector< int > movedIndexes;

	movedIndexes.resize( rToMoveIndexes.size() );
	for ( size_t i = 0; i != rToMoveIndexes.size(); ++i )
		movedIndexes[ i ] = m_customOrder[ rToMoveIndexes[ i ] ];

	// cut each custom index to move from m_customOrder in reverse order,
	// assuming that display indexes in rToMoveIndexes are sorted ascending
	for ( size_t i = rToMoveIndexes.size(); i-- != 0; )
	{
		int toMoveIndex = rToMoveIndexes[ i ];

		m_customOrder.erase( m_customOrder.begin() + toMoveIndex );
		// decrement rToDestIndex if removed display index is below it (offset to left destination index)
		if ( toMoveIndex < rToDestIndex )
			--rToDestIndex;
	}

	// insert the moved display indexes at the insertion point in their ascending order
	for ( size_t i = 0; i != movedIndexes.size(); ++i, ++rToDestIndex )
	{
		m_customOrder.insert( m_customOrder.begin() + rToDestIndex, movedIndexes[ i ] );
		// also store (replace) the new display index corresponding to each moved index in m_customOrder
		movedIndexes[ i ] = rToDestIndex;
	}
	if ( movedIndexes == rToMoveIndexes )
		return false;			// after move nothing has changed

	rToMoveIndexes.swap( movedIndexes );
	return true;
}

bool CAlbumModel::MoveBackCustomOrderIndexes( int newDestIndex, const std::vector< int >& rToMoveIndexes )
{
	// switch to custom order if not already
	ASSERT( IsCustomOrder() );

	// if newDestIndex == m_customOrder.size() then append indexes to move
	ASSERT( newDestIndex >=0 && newDestIndex <= (int)m_customOrder.size() );

	ASSERT( m_fileAttributes.size() == m_customOrder.size() );
	ASSERT( rToMoveIndexes.size() > 0 && rToMoveIndexes.size() < m_customOrder.size() );
	ASSERT( newDestIndex >= 0 && newDestIndex <= (int)m_customOrder.size() );

	// extract the originally moved entries in m_customOrder
	//
	std::vector< int > movedIndexes;
	std::vector< int >::iterator it;
	int count = (int)rToMoveIndexes.size();
	int toDestIndex = newDestIndex - count;		// dropped indexes are before the insertion point

	while ( count-- > 0 )
	{
		int movedIndex = m_customOrder[ toDestIndex ];

		movedIndexes.push_back( movedIndex );
		it = m_customOrder.erase( m_customOrder.begin() + toDestIndex );
	}

	for ( size_t i = 0; i != movedIndexes.size(); ++i )
	{
		int toMoveIndex = rToMoveIndexes[ i ];

		m_customOrder.insert( m_customOrder.begin() + toMoveIndex, movedIndexes[ i ] );
		// decrement toDestIndex if current true index is before insertion point index
		if ( toMoveIndex < toDestIndex )
			++toDestIndex;
	}

	TRACE( _T("toDestIndex=%d  %s\n"), toDestIndex, str::FormatSet( rToMoveIndexes ).c_str() );
	TRACE( _T("newDestIndex=%d  %s\n"), newDestIndex, str::FormatSet( movedIndexes ).c_str() );

	return true;
}

void CAlbumModel::FetchFilePathsFromIndexes( std::vector< fs::CPath >& rFilePaths, const std::vector< int >& displayIndexes ) const
{
	rFilePaths.resize( displayIndexes.size() );
	for ( size_t i = 0; i != displayIndexes.size(); ++i )
		rFilePaths[ i ] = GetFileAttr( displayIndexes[ i ] ).GetPath();
}

void CAlbumModel::ClearArchiveStgPaths( void )
{
	CImageArchiveStg::Factory().ReleaseStorages( m_archiveStgPaths );
	m_archiveStgPaths.clear();
}

void CAlbumModel::SearchForFiles( CWnd* pParentWnd, bool reportEmpty /*= true*/ ) throws_( CException* )
{
	ASSERT( !m_inGeneration );
	CScopedValue< bool > scopedGeneration( &m_inGeneration, true );
	std::auto_ptr< hlp::CRetainCustomOrder > pRetainCustomOrder( CustomOrder == m_fileOrder ? new hlp::CRetainCustomOrder( &m_fileAttributes, &m_customOrder ) : NULL );

	CImagesProgressCallback progress( pParentWnd != NULL ? pParentWnd : AfxGetMainWnd() );

	m_fileAttributes.clear();			// clear the file results
	ClearArchiveStgPaths();

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

		imageEnum.SwapResults( m_fileAttributes, &m_archiveStgPaths );

		if ( reportEmpty && !imageEnum.GetIssueStore().IsEmpty() )
			app::GetUserReport().ReportIssues( imageEnum.GetIssueStore(), MB_OK | MB_ICONEXCLAMATION );
	}

	ui::IProgressCallback* pProgressCallback = progress.GetCallback();
	progress.Section_OrderImageFiles( m_fileAttributes.size() );

	OrderFileList( pProgressCallback );

	switch ( m_fileOrder )
	{
		case CustomOrder:
			pRetainCustomOrder->RestoreInitialOrder( &m_fileAttributes, &m_customOrder );
			break;
		case FileSameSize:
		case FileSameSizeAndDim:
			FilterFileDuplicates( pProgressCallback );
			if ( FileSameSizeAndDim == m_fileOrder )
			{
				pProgressCallback->SetProgressRange( 0, m_fileAttributes.size(), true );
				std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByValue( pred::CompareFileAttr( m_fileOrder, true, pProgressCallback ) ) );
				pProgressCallback->AdvanceItemToEnd();

				FilterFileDuplicates( pProgressCallback, true );
			}
			break;
		case CorruptedFiles:
			FilterCorruptFiles( pProgressCallback );
			break;
	}
}

bool CAlbumModel::OrderFileList( ui::IProgressCallback* pProgressCallback )
{
	ASSERT_PTR( pProgressCallback );
	switch ( m_fileOrder )
	{
		case OriginalOrder:
		case CustomOrder:
		case CorruptedFiles:
			return false;
	}

	pProgressCallback->AdvanceStage( _T("Sorting Images") );
	pProgressCallback->SetProgressRange( 0, m_fileAttributes.size(), true );

	switch ( m_fileOrder )
	{
		case Shuffle:
			// init the random seed with current tick count
			m_randomSeed = ::GetTickCount();
			// fall-through
		case ShuffleSameSeed:
			srand( m_randomSeed % 0x7FFF );		// randomize on m_randomSeed
			std::random_shuffle( m_fileAttributes.begin(), m_fileAttributes.end() );
			break;
		default:
			std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByValue( pred::CompareFileAttr( m_fileOrder, false, pProgressCallback ) ) );
			break;
	}

	pProgressCallback->AdvanceItemToEnd();
	return true;
}

void CAlbumModel::FilterFileDuplicates( ui::IProgressCallback* pProgressCallback, bool compareImageDim /*= false*/ )
{	// removes files with single occurences according to m_fileOrder criteria, leaving only multiple occurences (e.g. files with the same size)
	ASSERT_PTR( pProgressCallback );

	size_t count = m_fileAttributes.size();

	pProgressCallback->AdvanceStage( _T("Filter Duplicate Images") );
	pProgressCallback->SetProgressRange( 0, count, true );

	pred::CompareFileAttr compare( m_fileOrder, compareImageDim, pProgressCallback );
	compare.m_useSecondaryComparison = false;		// turn on the accurate == comparisons

	for ( size_t i = 0; i < count; )
	{
		size_t j = i;

		while ( j < count - 1 && pred::Equal == compare( m_fileAttributes[ j + 1 ], m_fileAttributes[ j ] ) )
			j++;

		if ( j == i )
		{
			m_fileAttributes.erase( m_fileAttributes.begin() + i );
			--count;
		}
		else
			i = j + 1;
	}

	pProgressCallback->AdvanceItemToEnd();
}

void CAlbumModel::FilterCorruptFiles( ui::IProgressCallback* pProgressCallback )
{	// removes all invalid/non-existing files from this file list
	pProgressCallback->AdvanceStage( _T("Checking for invalid images") );
	pProgressCallback->SetProgressRange( 0, m_fileAttributes.size(), true );

	app::LogEvent( _T("---------- Search for corrupt images ----------") );

	CTimer timer;

	for ( std::vector< CFileAttr >::iterator itFileAttr = m_fileAttributes.begin(); itFileAttr != m_fileAttributes.end() && !m_inGeneration; )
	{
		pProgressCallback->AdvanceItem( itFileAttr->GetPath().Get() );

		if ( CWicImage::IsCorruptFile( itFileAttr->GetPath() ) )
		{
			app::LogEvent( _T("Found corrupt image file: %s"), itFileAttr->GetPath().GetPtr() );
			++itFileAttr;
		}
		else
			itFileAttr = m_fileAttributes.erase( itFileAttr );
	}

	pProgressCallback->AdvanceItemToEnd();

	app::LogEvent( _T("---------- End of search, elapsed %.2f seconds ----------"), timer.ElapsedSeconds() );
}


namespace pred
{
	typedef JoinCompare< CompareImageDimensions, CompareFileAttrPath > Compare_ImageDimensions;		// area | width | height


	// CompareFileAttr implementation

	CompareFileAttr::CompareFileAttr( CAlbumModel::Order fileOrder, bool compareImageDim /*= false*/, ui::IProgressCallback* pProgressCallback /*= NULL*/ )
		: m_fileOrder( fileOrder )
		, m_ascendingOrder( true )
		, m_compareImageDim( compareImageDim )
		, m_useSecondaryComparison( true )
		, m_pProgressCallback( pProgressCallback )
	{
		switch ( m_fileOrder )
		{
			case CAlbumModel::ByFileNameDesc:
			case CAlbumModel::ByFullPathDesc:
			case CAlbumModel::BySizeDesc:
			case CAlbumModel::ByDateDesc:
			case CAlbumModel::ByDimensionDesc:
				m_ascendingOrder = false;
		}
	}

	CompareResult CompareFileAttr::operator()( const CFileAttr& left, const CFileAttr& right ) const
	{
		static const std::tstring s_empty;

		if ( m_pProgressCallback != NULL )
			m_pProgressCallback->AdvanceItem( s_empty );

		CompareResult result = Equal;

		switch ( m_fileOrder )
		{
			case CAlbumModel::ByFullPathAsc:
			case CAlbumModel::ByFullPathDesc:
				result = CompareFileAttrPath()( left, right );
				break;
			case CAlbumModel::ByFileNameAsc:
			case CAlbumModel::ByFileNameDesc:
				result = TCompareNameExt()( left.GetPath(), right.GetPath() );
				break;
			case CAlbumModel::BySizeAsc:
			case CAlbumModel::BySizeDesc:
			case CAlbumModel::FileSameSize:
			case CAlbumModel::FileSameSizeAndDim:
				result = CompareFileAttrSize()( left, right );
				if ( Equal == result )
					if ( CAlbumModel::FileSameSizeAndDim == m_fileOrder && m_compareImageDim )
						result = CompareImageDimensions()( left, right );

				// also ordonate by filepath (if secondary comparison not disabled)
				if ( result == Equal && m_useSecondaryComparison )
					result = CompareFileAttrPath()( left, right );
				break;
			case CAlbumModel::ByDateAsc:
			case CAlbumModel::ByDateDesc:
				result = CompareFileTime( left.GetLastModifTime(), right.GetLastModifTime() );
				break;
			case CAlbumModel::ByDimensionAsc:
			case CAlbumModel::ByDimensionDesc:
				result = Compare_ImageDimensions()( left, right );
				break;
			default:
				ASSERT( false );
		}

		return GetResultInOrder( result, m_ascendingOrder );
	}

} //namespace pred


namespace hlp
{
	// CRetainCustomOrder class

	CRetainCustomOrder::CRetainCustomOrder( const std::vector< CFileAttr >* pFileAttributes, const std::vector< int >* pCustomOrder )
	{
		ASSERT_PTR( pFileAttributes );
		ASSERT_PTR( pCustomOrder );

		// backup the original mapping between original filepaths and their display indexes, as well as the original filepath-ordered vector
		m_orderedFilePaths.resize( pCustomOrder->size() );
		for ( UINT i = 0; i != pCustomOrder->size(); ++i )
		{
			int trueIndex = pCustomOrder->at( i );
			const fs::CFlexPath& filePath = pFileAttributes->at( i ).GetPath();

			m_pathToDispIndexMap[ filePath ] = trueIndex;		// add file at TRUE index
			m_orderedFilePaths[ i ] = filePath;					// add file at DISPLAY index
		}
	}

	void CRetainCustomOrder::RestoreInitialOrder( std::vector< CFileAttr >* pFileAttributes, std::vector< int >* pCustomOrder )
	{
		ASSERT_PTR( pFileAttributes );
		ASSERT_PTR( pCustomOrder );

		// restore what's left of the original order, remove what's not in the snapshot anymore, and append new files in display order.
		//
		std::vector< int > newFilesIndexes;			// true indexes for the newly added files in 'm_fileAttributes'
		std::map< fs::CFlexPath, int > newSnapshotMap;

		for ( int i = 0, countNew = static_cast< int >( pFileAttributes->size() ); i != countNew; ++i )
		{
			const fs::CFlexPath& filePath = pFileAttributes->at( i ).GetPath();

			if ( m_pathToDispIndexMap.find( filePath ) == m_pathToDispIndexMap.end() )
				newFilesIndexes.push_back( i );		// this is a newly added file in the snapshot -> store its the true index in newFilesIndexes

			newSnapshotMap[ filePath ] = i;			// also build the file-path map of the new snapshot for quick filepath lookup
		}
		// setup m_customOrder with the original ordered files (the ones that still are in the new snapshot)
		pCustomOrder->clear();

		for ( std::vector< fs::CFlexPath >::iterator itOrg = m_orderedFilePaths.begin(); itOrg != m_orderedFilePaths.end(); ++itOrg )
		{
			std::map< fs::CFlexPath, int >::const_iterator itNew = newSnapshotMap.find( *itOrg );
			if ( itNew != newSnapshotMap.end() )
				pCustomOrder->push_back( itNew->second );		// original file still in the new snapshot -> add to m_customOrder an entry with the true index of this filepath
		}

		// append the true indexes for the newly added files to the m_customOrder
		pCustomOrder->insert( pCustomOrder->end(), newFilesIndexes.begin(), newFilesIndexes.end() );

		ENSURE( pCustomOrder->size() == pFileAttributes->size() );
	}

} //namespace hlp
