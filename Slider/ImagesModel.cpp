
#include "stdafx.h"
#include "ImagesModel.h"
#include "FileAttr.h"
#include "FileAttrAlgorithms.h"
#include "Application_fwd.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/FileSystem.h"
#include "utl/PathMap.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/Timer.h"
#include "utl/IProgressService.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImagesModel::CImagesModel( void )
{
}

CImagesModel::CImagesModel( const CImagesModel& right )
{
	operator=( right );
}

CImagesModel::~CImagesModel()
{
	Clear();
}

CImagesModel& CImagesModel::operator=( const CImagesModel& right )
{
	if ( &right != this )
	{
		utl::CloneOwningContainerObjects( m_fileAttributes, right.m_fileAttributes );
		m_storagePaths = right.m_storagePaths;
	}
	return *this;
}

void CImagesModel::Clear( void )
{
	utl::ClearOwningContainer( m_fileAttributes );
	m_storagePaths.clear();

	if ( m_pIndexer.get() != NULL )
		m_pIndexer->Clear();
}

void CImagesModel::StoreBaselineSequence( void )
{
	fattr::StoreBaselineSequence( m_fileAttributes );
}

void CImagesModel::Swap( CImagesModel& rImagesModel )
{
	m_fileAttributes.swap( rImagesModel.m_fileAttributes );
	m_storagePaths.swap( rImagesModel.m_storagePaths );
}

void CImagesModel::SetUseIndexing( bool useIndexing /*= true*/ )
{
	ASSERT( IsEmpty() );
	m_pIndexer.reset( useIndexing ? new fs::CPathIndex< fs::CFlexPath >() : NULL );
}

void CImagesModel::Stream( CArchive& archive )
{
	serial::StreamOwningPtrs( archive, m_fileAttributes );
	serial::SerializeValues( archive, m_storagePaths );

	std::vector< int > displaySequence;				// display indexes that points to a baseline position in m_fileAttributes

	if ( archive.IsLoading() )
	{
		serial::SerializeValues( archive, displaySequence );

		if ( !displaySequence.empty() )
		{
			ASSERT( displaySequence.size() == m_fileAttributes.size() );

			// basically un-persist CFileAttr::m_baselinePos
			for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
				m_fileAttributes[ pos ]->StoreBaselinePos( displaySequence[ pos ] );
		}
		else
		{	// backwards compatibility: displaySequence was saved as CAlbumModel::m_customOrder, but only for m_fileOrder == CustomOrder
			StoreBaselineSequence();		// assume current positions in m_fileAttributes as BASELINE
		}
	}
	else
	{
		fattr::QueryDisplayIndexSequence( &displaySequence, m_fileAttributes );
		serial::SerializeValues( archive, displaySequence );		// basically persist CFileAttr::m_baselinePos
	}
}

bool CImagesModel::ContainsFileAttr( const fs::CFlexPath& filePath ) const
{
	if ( m_pIndexer.get() != NULL )
		return m_pIndexer->Contains( filePath );

	return FindPosFileAttr( filePath ) != utl::npos;
}

size_t CImagesModel::FindPosFileAttr( const fs::CFlexPath& filePath ) const
{
	return fattr::FindPosWithPath( m_fileAttributes, filePath );
}

const CFileAttr* CImagesModel::FindFileAttr( const fs::CFlexPath& filePath ) const
{
	return fattr::FindWithPath( m_fileAttributes, filePath );
}

bool CImagesModel::AddFileAttr( CFileAttr* pFileAttr )
{
	ASSERT_PTR( pFileAttr );
	//REQUIRE( !utl::Contains( m_fileAttributes, pFileAttr ) );		// added once?

	bool exists = m_pIndexer.get() != NULL
		? !m_pIndexer->Register( pFileAttr->GetPath() )
		: ( FindPosFileAttr( pFileAttr->GetPath() ) != utl::npos );

	if ( exists )
	{
		delete pFileAttr;
		return false;
	}

	m_fileAttributes.push_back( pFileAttr );
	return true;
}

std::auto_ptr<CFileAttr> CImagesModel::RemoveFileAttrAt( size_t pos )
{
	REQUIRE( pos < m_fileAttributes.size() );

	std::auto_ptr<CFileAttr> pRemovedFileAttr( m_fileAttributes[ pos ] );

	m_fileAttributes.erase( m_fileAttributes.begin() + pos );
	return pRemovedFileAttr;
}

bool CImagesModel::AddStoragePath( const fs::TStgDocPath& storagePath )
{
	return utl::AddUnique( m_storagePaths, storagePath );
}

void CImagesModel::ClearInvalidStoragePaths( void )
{
	// backwards compatibility: delete dangling embedded storages
	for ( std::vector< fs::TStgDocPath >::iterator itStoragePath = m_storagePaths.begin(); itStoragePath != m_storagePaths.end(); )
		if ( !fs::IsValidStructuredStorage( itStoragePath->GetPtr() ) )
			itStoragePath = m_storagePaths.erase( itStoragePath );
		else
			++itStoragePath;
}


// file ordering implementation

void CImagesModel::OrderFileAttrs( fattr::Order fileOrder, utl::IProgressService* pProgressSvc )
{
	switch ( fileOrder )
	{
		case fattr::CustomOrder:
			ASSERT( false );		// should be done in CAlbumModel, where the original order is maintained
			return;
		case fattr::FilterFileSameSize:
		case fattr::FilterFileSameSizeAndDim:
			FilterFileDuplicates( fileOrder, pProgressSvc );
			if ( fattr::FilterFileSameSizeAndDim == fileOrder )
			{
				pProgressSvc->GetHeader()->SetStageLabel( _T("Sort Images by Dimensions") );
				std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByPtr( pred::CompareFileAttr( fileOrder, true ) ) );

				FilterFileDuplicates( fileOrder, pProgressSvc, true );
			}
			return;
		case fattr::FilterCorruptedFiles:
			FilterCorruptFiles( pProgressSvc );
			return;
	}

	// do standard ordering
	switch ( fileOrder )
	{
		case fattr::OriginalOrder:
			std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByPtr( pred::TCompareBaselinePos() ) );
			break;
		case fattr::Shuffle:
		case fattr::ShuffleSameSeed:
			// assume ::srand was already called by the client
			std::random_shuffle( m_fileAttributes.begin(), m_fileAttributes.end() );
			break;
		default:
			std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByPtr( pred::CompareFileAttr( fileOrder, false ) ) );
			break;
	}
}

void CImagesModel::FilterFileDuplicates( fattr::Order fileOrder, utl::IProgressService* pProgressSvc, bool compareImageDim /*= false*/ )
{	// removes files with single occurences according to m_fileOrder criteria, leaving only multiple occurences (e.g. files with the same size)
	size_t count = m_fileAttributes.size();

	pProgressSvc->AdvanceStage( str::Format( _T("Filter Duplicate Images by %s"), compareImageDim ? _T("Dimensions") : _T("File Size") ) );
	pProgressSvc->SetBoundedProgressCount( count );

	pred::CompareFileAttr compare( fileOrder, compareImageDim );
	compare.m_useSecondaryComparison = false;		// turn on the accurate == comparisons

	for ( size_t i = 0; i < count; )
	{
		size_t j = i;

		while ( j < count - 1 && pred::Equal == compare( m_fileAttributes[ j + 1 ], m_fileAttributes[ j ] ) )
			j++;

		pProgressSvc->AdvanceItem( m_fileAttributes[ i ]->GetPath().Get() );
		if ( j == i )
		{
			m_fileAttributes.erase( m_fileAttributes.begin() + i );
			--count;
		}
		else
			i = j + 1;
	}
}

void CImagesModel::FilterCorruptFiles( utl::IProgressService* pProgressSvc )
{	// removes all invalid/non-existing files from this file list
	pProgressSvc->AdvanceStage( _T("Checking for corrupted image files") );
	pProgressSvc->SetBoundedProgressCount( m_fileAttributes.size() );

	app::LogEvent( _T("---------- Search for corrupt images ----------") );

	CTimer timer;

	for ( std::vector< CFileAttr* >::iterator itFileAttr = m_fileAttributes.begin(); itFileAttr != m_fileAttributes.end(); )
	{
		pProgressSvc->AdvanceItem( ( *itFileAttr )->GetPath().Get() );

		if ( CWicImage::IsCorruptFile( ( *itFileAttr )->GetPath() ) )
		{
			app::LogEvent( _T("Found corrupt image file: %s"), ( *itFileAttr )->GetPath().GetPtr() );
			++itFileAttr;
		}
		else
			itFileAttr = m_fileAttributes.erase( itFileAttr );
	}

	app::LogEvent( _T("---------- End of search, elapsed %.2f seconds ----------"), timer.ElapsedSeconds() );
}
