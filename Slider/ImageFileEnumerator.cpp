
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

CImageFileEnumerator::CImageFileEnumerator( IEnumerator* pProgressCallback /*= NULL*/ )
	: fs::CEnumerator( pProgressCallback )
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

void CImageFileEnumerator::SwapResults( std::vector< CFileAttr >& rFileAttrs, std::vector< fs::CPath >* pArchiveStgPaths /*= NULL*/ )
{
	rFileAttrs.swap( m_fileAttrs );

	if ( pArchiveStgPaths != NULL )
		pArchiveStgPaths->swap( m_archiveStgPaths );
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

void CImageFileEnumerator::Push( const CFileAttr& fileAttr )
{
	if ( !PassFilter( fileAttr ) )
		return;

	m_fileAttrs.push_back( fileAttr );

	if ( m_pChainEnum != NULL )
		m_pChainEnum->AddFoundFile( fileAttr.GetPath().GetPtr() );

//Sleep( 100 );			// debug progress bar
}

void CImageFileEnumerator::PushMany( const std::vector< CFileAttr >& fileAttrs )
{
	m_fileAttrs.reserve( std::min( m_fileAttrs.size() + fileAttrs.size(), m_maxFiles ) );

	for ( std::vector< CFileAttr >::const_iterator itFileAttr = fileAttrs.begin(); itFileAttr != fileAttrs.end() && !MustStop(); ++itFileAttr )
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
	else
		Push( CFileAttr( foundFile ) );
}

bool CImageFileEnumerator::MustStop( void ) const
{
	return m_fileAttrs.size() >= m_maxFiles;
}


// CImagesProgressCallback implementation

const std::tstring CImagesProgressCallback::s_searching = _T("Searching for Image Files");

CImagesProgressCallback::CImagesProgressCallback( CWnd* pParent, const std::tstring& operationLabel /*= s_searching*/ )
	: m_dlg( operationLabel, CProgressDialog::StageLabelCount )
{
	if ( m_dlg.Create( _T("Image Files"), pParent ) )
	{
		m_dlg.SetStageLabel( _T("Search directory") );
		m_dlg.SetItemLabel( _T("Found image") );
		m_dlg.SetMarqueeProgress();
	}
}

CImagesProgressCallback::~CImagesProgressCallback()
{
	m_dlg.DestroyWindow();
}

void CImagesProgressCallback::Section_OrderImageFiles( size_t fileCount )
{
	m_dlg.SetOperationLabel( _T("Order Images") );
	m_dlg.ShowStage( false );
	m_dlg.SetItemLabel( _T("Compare image file attributes") );

	GetCallback()->SetProgressItemCount( fileCount );
}

void CImagesProgressCallback::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	GetCallback()->AdvanceItem( pFilePath );
}

bool CImagesProgressCallback::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	GetCallback()->AdvanceStage( pSubDirPath );
	return true;
}
