
#include "stdafx.h"
#include "ImageFileEnumerator.h"
#include "SearchPattern.h"
#include "FileAttr.h"
#include "ImageArchiveStg.h"
#include "AlbumDoc.h"
#include "Application_fwd.h"
#include "utl/ContainerUtilities.h"
#include "utl/RuntimeException.h"
#include "utl/ErrorHandler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const Range< size_t > CImageFileEnumerator::s_allFileSizesRange( 0, UINT_MAX );

CImageFileEnumerator::CImageFileEnumerator( IEnumerator* pProgressEnum /*= NULL*/ )
	: fs::CEnumerator( pProgressEnum )
	, m_fileSizeRange( s_allFileSizesRange )
	, m_issueStore( _T("Searching for images") )
	, m_pCurrPattern( NULL )
{
}

CImageFileEnumerator::~CImageFileEnumerator()
{
}

void CImageFileEnumerator::Search( const std::vector< CSearchPattern* >& searchPatterns ) throws_( CException*, CUserAbortedException )
{
	CWaitCursor wait;
	CScopedErrorHandling scopedThrow( CImageArchiveStg::Factory(), utl::ThrowMode );		// report storage sharing violations, etc

	for ( std::vector< CSearchPattern* >::const_iterator itPattern = searchPatterns.begin(); itPattern != searchPatterns.end(); )
	{
		m_pCurrPattern = *itPattern;
		try
		{
			const size_t oldFoundSize = m_foundImages.GetFileAttrs().size();

			if ( m_pChainEnum != NULL && m_pCurrPattern->IsDirPath() )
				m_pChainEnum->AddFoundSubDir( m_pCurrPattern->GetFilePath().GetPtr() );	// progress only: advance stage to the root directory

			m_pCurrPattern->EnumImageFiles( this );

			if ( oldFoundSize == m_foundImages.GetFileAttrs().size() )	// no new matching files found
				m_issueStore.AddIssue( str::Format( _T("No matching images found in: %s"), m_pCurrPattern->GetFilePath().GetPtr() ) );
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
		++itPattern;
	}

	UniquifyAll();
}

void CImageFileEnumerator::Search( const CSearchPattern& searchPattern ) throws_( CException*, CUserAbortedException )
{
	std::vector< CSearchPattern* > searchPatterns( 1, const_cast< CSearchPattern* >( &searchPattern ) );
	Search( searchPatterns );
}

void CImageFileEnumerator::SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException*, CUserAbortedException )
{
	if ( !stgDocPath.FileExist() )
		AfxThrowFileException( CFileException::fileNotFound, -1, stgDocPath.GetPtr() );		// storage file path does not exist

	CWaitCursor wait;
	CScopedErrorHandling scopedThrow( CImageArchiveStg::Factory(), utl::ThrowMode );		// report storage sharing violations, etc

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

	if ( m_pCurrPattern != NULL )
		if ( CSearchPattern::AutoDropNumFormat == m_pCurrPattern->GetSearchMode() )
			if ( !CSearchPattern::IsNumFileName( fileAttr.GetPath().GetPtr() ) )
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
		CAlbumDoc archiveDoc;
		if ( CAlbumDoc::Succeeded == archiveDoc.LoadArchiveStorage( filePath ) )	// note: we need to load as CAlbumDoc since its document schema may be older (backwards compatibility)
		{
			AddFoundSubDir( filePath.GetPtr() );							// a storage counts as a sub-directory

			std::vector< CFileAttr* > archiveImageAttrs;
			archiveDoc.RefModel()->SwapFileAttrs( archiveImageAttrs );		// take ownership of found image attributes

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
