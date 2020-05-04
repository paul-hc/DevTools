
#include "stdafx.h"
#include "ImageFileEnumerator.h"
#include "SearchSpec.h"
#include "FileAttr.h"
#include "ImageArchiveStg.h"
#include "Application.h"
#include "utl/ContainerUtilities.h"
#include "utl/RuntimeException.h"
#include "utl/ThrowMode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const Range< size_t > CImageFileEnumerator::s_allFileSizesRange( 0, UINT_MAX );

CImageFileEnumerator::CImageFileEnumerator( IEnumerator* pProgressEnum /*= NULL*/ )
	: fs::CEnumerator( pProgressEnum )
	, m_fileSizeRange( s_allFileSizesRange )
	, m_issueStore( _T("Searching for images") )
	, m_pCurrSpec( NULL )
{
}

CImageFileEnumerator::~CImageFileEnumerator()
{
}

void CImageFileEnumerator::Search( const std::vector< CSearchSpec* >& searchSpecs ) throws_( CException*, CUserAbortedException )
{
	CWaitCursor wait;
	CPushThrowMode pushThrow( &CImageArchiveStg::Factory(), true );			// report storage sharing violations, etc

	for ( std::vector< CSearchSpec* >::const_iterator itSpec = searchSpecs.begin(); itSpec != searchSpecs.end(); )
	{
		m_pCurrSpec = *itSpec;
		try
		{
			const size_t oldFoundSize = m_foundImages.GetFileAttrs().size();
			m_pCurrSpec->EnumImageFiles( this );

			if ( oldFoundSize == m_foundImages.GetFileAttrs().size() )	// no new matching files found
				m_issueStore.AddIssue( str::Format( _T("No matching images found in: %s"), m_pCurrSpec->GetFilePath().GetPtr() ) );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_CANCELTRYCONTINUE | MB_ICONEXCLAMATION | MB_DEFBUTTON3 ) )
			{
				case IDCANCEL:
				case IDABORT:
					m_foundImages.ReleaseStorages();
					throw new mfc::CUserAbortedException;
				case IDTRYAGAIN:
				case IDRETRY:	continue;
				case IDCONTINUE:
				case IDIGNORE:	break;
			}
		}
		++itSpec;
	}
}

void CImageFileEnumerator::Search( const CSearchSpec& searchSpec ) throws_( CException*, CUserAbortedException )
{
	std::vector< CSearchSpec* > searchSpecs( 1, const_cast< CSearchSpec* >( &searchSpec ) );
	Search( searchSpecs );
}

void CImageFileEnumerator::SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException*, CUserAbortedException )
{
	if ( !stgDocPath.FileExist() )
		AfxThrowFileException( CFileException::fileNotFound, -1, stgDocPath.GetPtr() );		// storage file path does not exist

	CWaitCursor wait;
	CPushThrowMode pushThrow( &CImageArchiveStg::Factory(), true );			// report storage sharing violations, etc

	m_issueStore.Reset( _T("Querying storage for images") );

	AddFoundFile( stgDocPath.GetPtr() );
}

void CImageFileEnumerator::SwapFoundImages( CImagesModel& rImagesModel )
{
	m_foundImages.StoreBaselineSequence();			// keep track of the original found order
	m_foundImages.Swap( rImagesModel );
}

bool CImageFileEnumerator::PassFilter( const CFileAttr& fileAttr ) const
{
	if ( !m_fileSizeRange.Contains( fileAttr.GetFileSize() ) )
		return false;

	if ( m_pCurrSpec != NULL )
		if ( CSearchSpec::AutoDropNumFormat == m_pCurrSpec->GetSearchMode() )
			if ( !CSearchSpec::IsNumFileName( fileAttr.GetPath().GetPtr() ) )
				return false;

	return true;
}

void CImageFileEnumerator::Push( CFileAttr* pFileAttr )
{
	if ( !PassFilter( *pFileAttr ) )
		return;

	m_foundImages.AddFileAttr( pFileAttr );

	if ( m_pChainEnum != NULL )
		m_pChainEnum->AddFoundFile( pFileAttr->GetPath().GetPtr() );

//Sleep( 100 );			// debug progress bar
}

void CImageFileEnumerator::PushMany( const std::vector< CFileAttr* >& fileAttrs )
{
	// go via filtering
	for ( std::vector< CFileAttr* >::const_iterator itFileAttr = fileAttrs.begin(); itFileAttr != fileAttrs.end() && !MustStop(); ++itFileAttr )
		Push( *itFileAttr );
}

void CImageFileEnumerator::AddFoundFile( const TCHAR* pFilePath )
{
	fs::CPath filePath( pFilePath );

	if ( app::IsImageArchiveDoc( filePath.GetPtr() ) )
	{
		// found a compound image storage: load its metadata as found images
		if ( CImageArchiveStg::Factory().VerifyPassword( filePath ) )
		{
			AddFoundSubDir( filePath.GetPtr() );		// a storage counts as a sub-directory

			std::vector< CFileAttr* > archiveImageAttrs;
			CImageArchiveStg::Factory().LoadImagesMetadata( archiveImageAttrs, filePath );

			if ( !archiveImageAttrs.empty() )
			{
				PushMany( archiveImageAttrs );
				m_foundImages.AddStoragePath( filePath );
			}
		}
	}
	else if ( filePath.FileExist() )
		Push( new CFileAttr( filePath ) );
}

void CImageFileEnumerator::AddFile( const CFileFind& foundFile )
{
	fs::CPath filePath = foundFile.GetFilePath().GetString();

	if ( CImageArchiveStg::HasImageArchiveExt( filePath.GetPtr() ) )
		AddFoundFile( filePath.GetPtr() );
	else
		Push( new CFileAttr( foundFile ) );
}

bool CImageFileEnumerator::MustStop( void ) const
{
	return m_foundImages.GetFileAttrs().size() >= m_maxFiles;
}
