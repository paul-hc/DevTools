
#include "stdafx.h"
#include "ImagesModel.h"
#include "FileAttr.h"
#include "FileAttrAlgorithms.h"
#include "ImageArchiveStg.h"
#include "Application_fwd.h"
#include "utl/ContainerUtilities.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/Timer.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImagesModel::CImagesModel( void )
{
}

CImagesModel::~CImagesModel()
{
	Clear();
}

void CImagesModel::Clear( void )
{
	utl::ClearOwningContainer( m_fileAttributes );

	ReleaseStorages();
	m_storagePaths.clear();
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

void CImagesModel::ReleaseStorages( void )
{
	CImageArchiveStg::Factory().ReleaseStorages( m_storagePaths );
}

void CImagesModel::Swap( CImagesModel& rImagesModel )
{
	m_fileAttributes.swap( rImagesModel.m_fileAttributes );
	m_storagePaths.swap( rImagesModel.m_storagePaths );
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

void CImagesModel::StoreBaselineSequence( void )
{
	for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
		m_fileAttributes[ pos ]->StoreBaselinePos( pos );
}

const CFileAttr* CImagesModel::FindFileAttrWithPath( const fs::CPath& filePath ) const
{
	return fattr::FindWithPath( m_fileAttributes, filePath );
}

bool CImagesModel::AddFileAttr( CFileAttr* pFileAttr )
{
	ASSERT_PTR( pFileAttr );
	ASSERT( !utl::Contains( m_fileAttributes, pFileAttr ) );		// add once?

	if ( const CFileAttr* pFoundExisting = FindFileAttrWithPath( pFileAttr->GetPath() ) )
	{
		pFoundExisting;
		delete pFileAttr;
		return false;
	}

	m_fileAttributes.push_back( pFileAttr );
	return true;
}

bool CImagesModel::AddStoragePath( const fs::CPath& storagePath )
{
	return utl::AddUnique( m_storagePaths, storagePath );
}


// file ordering implementation

void CImagesModel::OrderFileAttrs( fattr::Order fileOrder, ui::IProgressService* pProgressSvc )
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
			std::sort( m_fileAttributes.begin(), m_fileAttributes.end(), pred::MakeOrderByPtr( pred::CompareBaselinePos() ) );
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

void CImagesModel::FilterFileDuplicates( fattr::Order fileOrder, ui::IProgressService* pProgressSvc, bool compareImageDim /*= false*/ )
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

void CImagesModel::FilterCorruptFiles( ui::IProgressService* pProgressSvc )
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