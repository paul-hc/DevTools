
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

CImageFileEnumerator::CImageFileEnumerator( void )
	: m_maxFileCount( UINT_MAX )
	, m_fileSizeRange( s_allFileSizesRange )
	, m_issueStore( _T("Searching for images") )
	, m_pProgress( new app::CScopedProgress( 0, 100, 1, m_issueStore.GetHeader().c_str() ) )
	, m_pCurrSpec( NULL )
{
}

CImageFileEnumerator::~CImageFileEnumerator()
{
}

void CImageFileEnumerator::Search( const std::vector< CSearchSpec* >& searchSpecs ) throws_( CException* )
{
	CWaitCursor wait;
	CPushThrowMode pushThrow( &CImageArchiveStg::Factory(), true );			// report storage sharing violations, etc

	for ( std::vector< CSearchSpec* >::const_iterator itSpec = searchSpecs.begin(); itSpec != searchSpecs.end(); )
	{
		m_pCurrSpec = *itSpec;
		try
		{
			const size_t oldFoundSize = m_fileAttrs.size();
			m_pCurrSpec->EnumImageFiles( this );

			if ( oldFoundSize == m_fileAttrs.size() )		// no new matching files found
				m_issueStore.AddIssue( str::Format( _T("No matching images found in: %s"), m_pCurrSpec->GetFilePath().GetPtr() ) );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_CANCELTRYCONTINUE | MB_ICONEXCLAMATION | MB_DEFBUTTON3 ) )
			{
				case IDCANCEL:
				case IDABORT:
					CImageArchiveStg::Factory().ReleaseStorages( m_archiveStgPaths );
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

void CImageFileEnumerator::Search( const CSearchSpec& searchSpec ) throws_( CException* )
{
	std::vector< CSearchSpec* > searchSpecs( 1, const_cast< CSearchSpec* >( &searchSpec ) );
	Search( searchSpecs );
}

void CImageFileEnumerator::SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException* )
{
	if ( !stgDocPath.FileExist() )
		AfxThrowFileException( CFileException::fileNotFound, -1, stgDocPath.GetPtr() );		// storage file path does not exist

	CWaitCursor wait;
	CPushThrowMode pushThrow( &CImageArchiveStg::Factory(), true );			// report storage sharing violations, etc

	m_issueStore.Reset( _T("Querying storage for images") );

	AddFoundFile( stgDocPath.GetPtr() );
}

void CImageFileEnumerator::SwapResults( std::vector< CFileAttr >& rFileAttrs, std::vector< fs::CPath >* pArchiveStgPaths /*= NULL*/ )
{
	rFileAttrs.swap( m_fileAttrs );

	if ( pArchiveStgPaths != NULL )
		pArchiveStgPaths->swap( m_archiveStgPaths );
}

bool CImageFileEnumerator::PassFilter( const CFileFind& foundFile ) const
{
	if ( ( m_fileAttrs.size() + 1 ) > m_maxFileCount )
		return false;

	size_t fileLen = static_cast< size_t >( foundFile.GetLength() );
	if ( !m_fileSizeRange.Contains( fileLen ) )
		return false;

	if ( m_pCurrSpec != NULL )
		if ( CSearchSpec::AutoDropNumFormat == m_pCurrSpec->GetSearchMode() )
			if ( !CSearchSpec::IsNumFileName( foundFile.GetFilePath() ) )
				return false;

	return true;
}

void CImageFileEnumerator::Push( const CFileAttr& fileAttr )
{
	m_fileAttrs.push_back( fileAttr );
	m_pProgress->StepIt();
	//Sleep( 100 );			// debug progress bar
}

void CImageFileEnumerator::PushMany( const std::vector< CFileAttr >& fileAttrs )
{
	m_fileAttrs.insert( m_fileAttrs.end(), fileAttrs.begin(), fileAttrs.end() );
	m_pProgress->OffsetPos( static_cast< int >( fileAttrs.size() ) );
}

bool CImageFileEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
{
	pSubDirPath;
	return true;
}

void CImageFileEnumerator::AddFoundFile( const TCHAR* pFilePath )
{
	fs::CPath filePath( pFilePath );

	if ( app::IsImageArchiveDoc( filePath.GetPtr() ) )
	{
		// found a compound image storage: load it's metadata as found images
		if ( CImageArchiveStg::Factory().VerifyPassword( filePath ) )
		{
			std::vector< CFileAttr > archiveImageAttrs;
			CImageArchiveStg::Factory().LoadImagesMetadata( archiveImageAttrs, filePath );

			if ( !archiveImageAttrs.empty() )
			{
				PushMany( archiveImageAttrs );
				m_archiveStgPaths.push_back( filePath );
			}
		}
	}
	else if ( filePath.FileExist() )
		Push( CFileAttr( filePath ) );
}

void CImageFileEnumerator::AddFile( const CFileFind& foundFile )
{
	fs::CPath filePath( foundFile.GetFilePath().GetString() );

	if ( CImageArchiveStg::HasImageArchiveExt( filePath.GetPtr() ) )
		AddFoundFile( filePath.GetPtr() );
	else if ( PassFilter( foundFile ) )
		Push( CFileAttr( foundFile ) );
}
