
#include "stdafx.h"
#include "FileList.h"
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


// CFileList implementation

CFileList::CFileList( void )
	: m_fileOrder( OriginalOrder )
	, m_randomSeed( GetTickCount() )
	, m_fileSizeRange( 0, UINT_MAX )
	, m_perFlags( 0 )
	, m_inGeneration( false )
{
}

CFileList::~CFileList()
{
}

const CEnumTags& CFileList::GetTags_Order( void )
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

void CFileList::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		archive << (int)m_fileOrder;
		archive << m_randomSeed;
		archive << m_fileSizeRange;
		archive << m_perFlags;
	}
	else
	{
		archive >> (int&)m_fileOrder;
		archive >> m_randomSeed;
		archive >> m_fileSizeRange;
		archive >> m_perFlags;
	}

	serial::StreamItems( archive, m_searchSpecs );
	serial::StreamItems( archive, m_fileAttributes );
	serial::SerializeValues( archive, m_archiveStgPaths );
	serial::SerializeValues( archive, m_customOrder );

	if ( !archive.IsStoring() )
		if ( MustAutoRegenerate() )
			try
			{
				SearchForFiles( false );			// regenerate the file list on archive load
			}
			catch ( CException* pExc )
			{
				app::HandleReportException( pExc );
			}
}

bool CFileList::SetupSearchPath( const fs::CPath& filePath )
{
	if ( !filePath.FileExist() )
		return false;

	m_searchSpecs.clear();
	m_stgDocPath.Clear();

	if ( fs::IsValidDirectory( filePath.GetPtr() ) )
		m_searchSpecs.push_back( CSearchSpec( filePath ) );
	else if ( app::IsImageArchiveDoc( filePath.GetPtr() ) )
		m_stgDocPath = filePath;
	else
		return false;

	return true;
}

int CFileList::FindSearchSpec( const fs::CPath& searchPath ) const
{
	for ( std::vector< CSearchSpec >::const_iterator itSearch = m_searchSpecs.begin(); itSearch != m_searchSpecs.end(); ++itSearch )
		if ( itSearch->m_searchPath == searchPath )
			return static_cast< int >( std::distance( m_searchSpecs.begin(), itSearch ) );

	return -1;
}

bool CFileList::CancelGeneration( void )
{
	if ( !m_inGeneration )
		return false;

	m_inGeneration = false;
	return true;
}

// release the image storages associated with this (if any), as well as the ones found in the generation process
void CFileList::CloseAssocImageArchiveStgs( void )
{
	if ( !m_stgDocPath.IsEmpty() )
		CImageArchiveStg::Factory().ReleaseStorage( m_stgDocPath );

	for ( std::vector< CSearchSpec >::const_iterator itSearch = m_searchSpecs.begin(); itSearch != m_searchSpecs.end(); ++itSearch )
		if ( itSearch->IsImageArchiveDoc() )
			CImageArchiveStg::Factory().ReleaseStorage( itSearch->m_searchPath );

	ClearArchiveStgPaths();
}

size_t CFileList::FindPosFileAttr( const CFileAttr* pFileAttr ) const
{
	for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
		if ( pFileAttr == &m_fileAttributes[ pos ] )
			return pos;

	ASSERT( false );			// pFileAttr should be from within m_fileAttributes
	return utl::npos;
}

int CFileList::DisplayToTrueFileIndex( size_t displayIndex ) const
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

void CFileList::QueryFileAttrs( std::vector< CFileAttr* >& rFileAttrs ) const
{
	rFileAttrs.clear();
	rFileAttrs.reserve( m_fileAttributes.size() );
	for ( size_t i = 0; i != m_fileAttributes.size(); ++i )
		rFileAttrs.push_back( const_cast< CFileAttr* >( &GetFileAttr( i ) ) );
}

// returns the display index of the found file
int CFileList::FindFileAttr( const fs::CFlexPath& filePath, bool wantDisplayIndex /*= true*/ ) const
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

bool CFileList::HasConsistentDeepStreams( void ) const
{
	return HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) == HasFlag( m_perFlags, UseDeepStreamPaths );
}

bool CFileList::CheckReparentFileAttrs( const TCHAR* pDocPath, PersistOp op )
{
	fs::CFlexPath docPath( fs::CFlexPath( pDocPath ).GetPhysicalPath().Get() );			// extract "C:\Images\storage.ias" from "C:\Images\storage.ias>_Album.sld"

	return
		app::IsImageArchiveDoc( docPath.GetPtr() ) &&
		ReparentStgFileAttrsImpl( docPath, op );
}

bool CFileList::ReparentStgFileAttrsImpl( const fs::CFlexPath& stgDocPath, PersistOp op )
{
	ASSERT( app::IsImageArchiveDoc( stgDocPath.GetPtr() ) );

	if ( stgDocPath != m_stgDocPath )								// different stgDocPath, need to store and reparent
	{
		m_searchSpecs.clear();
		m_stgDocPath = stgDocPath;

		// clear the common prefix to the actual logical root of the album
		std::for_each( m_fileAttributes.begin(), m_fileAttributes.end(), func::StripStgDocPrefix() );		// clear stg prefix

		if ( !HasFlag( m_perFlags, UseDeepStreamPaths ) && !m_fileAttributes.empty() )
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
		SetFlag( m_perFlags, UseDeepStreamPaths, HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) );	// will save to keep track of saved mode

	return true;
}

void CFileList::SetFileOrder( Order fileOrder )
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

void CFileList::SetCustomOrder( const std::vector< CFileAttr* >& customOrder )
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
bool CFileList::MoveCustomOrderIndexes( int& rToDestIndex, std::vector< int >& rToMoveIndexes )
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

bool CFileList::MoveBackCustomOrderIndexes( int newDestIndex, const std::vector< int >& rToMoveIndexes )
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

void CFileList::FetchFilePathsFromIndexes( std::vector< fs::CPath >& rFilePaths, const std::vector< int >& displayIndexes ) const
{
	rFilePaths.resize( displayIndexes.size() );
	for ( size_t i = 0; i != displayIndexes.size(); ++i )
		rFilePaths[ i ] = GetFileAttr( displayIndexes[ i ] ).GetPath();
}

void CFileList::ClearArchiveStgPaths( void )
{
	CImageArchiveStg::Factory().ReleaseStorages( m_archiveStgPaths );
	m_archiveStgPaths.clear();
}

void CFileList::SearchForFiles( bool reportEmpty /*= true*/ ) throws_( CException* )
{
	ASSERT( !m_inGeneration );
	CScopedValue< bool > scopedGeneration( &m_inGeneration, true );
	std::auto_ptr< hlp::CRetainCustomOrder > pRetainCustomOrder( CustomOrder == m_fileOrder ? new hlp::CRetainCustomOrder( &m_fileAttributes, &m_customOrder ) : NULL );

	m_fileAttributes.clear();			// clear the file results
	ClearArchiveStgPaths();

	{	// scope for "Searching..." progress
		CImageFileEnumerator imageEnum;
		imageEnum.SetFileSizeFilter( m_fileSizeRange );

		if ( !m_stgDocPath.IsEmpty() )
			imageEnum.SearchImageArchive( m_stgDocPath );
		else
			imageEnum.Search( m_searchSpecs );

		imageEnum.SwapResults( m_fileAttributes, &m_archiveStgPaths );

		if ( reportEmpty && !imageEnum.GetIssueStore().IsEmpty() )
			app::GetUserReport().ReportIssues( imageEnum.GetIssueStore(), MB_OK | MB_ICONEXCLAMATION );
	}

	OrderFileList();

	switch ( m_fileOrder )
	{
		case CustomOrder:
			pRetainCustomOrder->RestoreInitialOrder( &m_fileAttributes, &m_customOrder );
			break;
		case FileSameSize:
		case FileSameSizeAndDim:
			FilterFileDuplicates();
			if ( FileSameSizeAndDim == m_fileOrder )
			{
				{
					app::CScopedProgress progress( 0, (int)m_fileAttributes.size(), 1, _T("Check for images with same sizes:") );
					progress.SetStepDivider( 7 );		// ~ empirically set to 7 as the best for the tested working set

					std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByValue( pred::CompareFileAttr( m_fileOrder, true, &progress ) ) );
					progress.GotoEnd();
				}
				FilterFileDuplicates( true );
			}
			break;
		case CorruptedFiles:
			FilterCorruptFiles();
			break;
	}
}

bool CFileList::OrderFileList( void )
{
	switch ( m_fileOrder )
	{
		case OriginalOrder:
		case CustomOrder:
		case CorruptedFiles:
			return false;
		case Shuffle:
			// init the random seed with current tick count
			m_randomSeed = GetTickCount();
			// fall-through
		case ShuffleSameSeed:
			srand( m_randomSeed % 0x7FFF );		// randomize on m_randomSeed
			std::random_shuffle( m_fileAttributes.begin(), m_fileAttributes.end() );
			break;
		default:
			std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByValue( pred::CompareFileAttr( m_fileOrder ) ) );
			break;
	}
	return true;
}

// removes all valid or non-existing files from this file list
size_t CFileList::FilterCorruptFiles( void )
{
	size_t index = 0;
	app::CScopedProgress progress( 0, (int)m_fileAttributes.size(), 1, _T("Checking for invalid files:") );
	CTime initialTime = CTime::GetCurrentTime();

	app::LogEvent( _T("---------- Search for corrupt images ----------") );
	while ( index < m_fileAttributes.size() )
		if ( !m_inGeneration || !ui::PumpPendingMessages() )
			break;
		else
		{
			const CFileAttr& fileAttr = m_fileAttributes[ index ];

			if ( CWicImage::IsCorruptFile( fileAttr.GetPath() ) )
			{
				app::LogEvent( _T("Found corrupt image file: %s"), fileAttr.GetPath().GetPtr() );
				++index;
			}
			else
				m_fileAttributes.erase( m_fileAttributes.begin() + index );
			progress.StepIt();
		}

	progress.GotoEnd();

	CTimeSpan duration = ( CTime::GetCurrentTime() - initialTime );

	app::LogEvent( _T("---------- End of search, elapsed time [%s] ----------"), duration.Format( _T("%H:%M:%S") ).GetString() );
	return m_fileAttributes.size();
}

// removes files with single occurences according to m_fileOrder criteria, leaving only multiple occurences (e.g. files with the same size)
bool CFileList::FilterFileDuplicates( bool compareImageDim /*= false*/ )
{
	size_t count = m_fileAttributes.size();
	pred::CompareFileAttr compare( m_fileOrder, compareImageDim );
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

	return !m_fileAttributes.empty();			// true if any duplicates found
}


namespace pred
{
	typedef JoinCompare< CompareImageDimensions, CompareFileAttrPath > Compare_ImageDimensions;		// area | width | height


	// CompareFileAttr implementation

	CompareResult CompareFileAttr::operator()( const CFileAttr& left, const CFileAttr& right ) const
	{
		if ( m_pProgress != NULL )
			m_pProgress->StepIt();

		CompareResult result = Equal;

		switch ( m_fileOrder )
		{
			case CFileList::ByFullPathAsc:
			case CFileList::ByFullPathDesc:
				result = CompareFileAttrPath()( left, right );
				break;
			case CFileList::ByFileNameAsc:
			case CFileList::ByFileNameDesc:
				result = TCompareNameExt()( left.GetPath(), right.GetPath() );
				break;
			case CFileList::BySizeAsc:
			case CFileList::BySizeDesc:
			case CFileList::FileSameSize:
			case CFileList::FileSameSizeAndDim:
				result = CompareFileAttrSize()( left, right );
				if ( Equal == result )
					if ( m_fileOrder == CFileList::FileSameSizeAndDim && m_compareImageDim )
						result = CompareImageDimensions()( left, right );

				// also ordonate by filepath (if secondary comparison not disabled)
				if ( result == Equal && m_useSecondaryComparison )
					result = CompareFileAttrPath()( left, right );
				break;
			case CFileList::ByDateAsc:
			case CFileList::ByDateDesc:
				result = CompareFileTime( left.GetLastModifTime(), right.GetLastModifTime() );
				break;
			case CFileList::ByDimensionAsc:
			case CFileList::ByDimensionDesc:
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
