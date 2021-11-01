
#include "stdafx.h"
#include "ImageFileEnumerator.h"
#include "SearchPattern.h"
#include "FileAttr.h"
#include "ICatalogStorage.h"
#include "AlbumDoc.h"
#include "Application_fwd.h"
#include "utl/ContainerUtilities.h"
#include "utl/RuntimeException.h"
#include "utl/ErrorHandler.h"
#include "utl/UI/ImagingWic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImageFileEnumerator::CImageFileEnumerator( IEnumerator* pProgressEnum /*= NULL*/ )
	: fs::CPathEnumerator( fs::TEnumFlags::Make( fs::EF_Recurse | fs::EF_IgnoreHiddenNodes | fs::EF_ResolveShellLinks ), pProgressEnum )
	, m_issueStore( _T("Searching for images") )
	, m_pCurrPattern( NULL )
{
	m_foundImages.SetUseIndexing();			// optimize lookup performance when searching a large number of image files
}

CImageFileEnumerator::~CImageFileEnumerator()
{
}

void CImageFileEnumerator::Search( const std::vector< CSearchPattern* >& searchPatterns ) throws_( CException*, CUserAbortedException )
{
	CWaitCursor wait;
	CScopedErrorHandling scopedThrow( CCatalogStorageFactory::Instance(), utl::ThrowMode );		// report storage sharing violations, etc

	for ( std::vector< CSearchPattern* >::const_iterator itPattern = searchPatterns.begin(); itPattern != searchPatterns.end(); )
	{
		m_pCurrPattern = *itPattern;

		try
		{
			const size_t oldFoundSize = m_foundImages.GetFileAttrs().size();

			if ( m_pChainEnum != NULL && m_pCurrPattern->IsDirPath() )
				m_pChainEnum->AddFoundSubDir( m_pCurrPattern->GetFilePath().GetPtr() );		// progress only: advance stage to the root directory

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
	m_pCurrPattern = NULL;
}

void CImageFileEnumerator::Search( const CSearchPattern& searchPattern ) throws_( CException*, CUserAbortedException )
{
	std::vector< CSearchPattern* > searchPatterns( 1, const_cast< CSearchPattern* >( &searchPattern ) );
	Search( searchPatterns );
}

void CImageFileEnumerator::SearchCatalogStorage( const fs::CPath& docStgPath ) throws_( CException*, CUserAbortedException )
{
	if ( !docStgPath.FileExist() )
		AfxThrowFileException( CFileException::fileNotFound, -1, docStgPath.GetPtr() );			// storage file path does not exist

	CWaitCursor wait;
	CScopedErrorHandling scopedThrow( CCatalogStorageFactory::Instance(), utl::ThrowMode );		// report storage sharing violations, etc

	m_issueStore.Reset( _T("Querying storage for images") );

	AddFoundFile( docStgPath.GetPtr() );
}

void CImageFileEnumerator::SwapFoundImages( CImagesModel& rImagesModel )
{
	m_foundImages.StoreBaselineSequence();			// keep track of the original found order
	m_foundImages.Swap( rImagesModel );
}

bool CImageFileEnumerator::PassFilter( const CFileAttr& fileAttr ) const
{
	if ( !PassFileFilter( fileAttr.GetFileSize() ) )
		return false;

	if ( m_pCurrPattern != NULL )
		if ( CSearchPattern::AutoDropNumFormat == m_pCurrPattern->GetSearchMode() )
			if ( !CSearchPattern::IsNumFileName( fileAttr.GetPath().GetPtr() ) )
				return false;

	return true;
}

bool CImageFileEnumerator::Push( CFileAttr* pFileAttr )
{
	if ( !PassFilter( *pFileAttr ) )
	{
		delete pFileAttr;
		return false;
	}

	if ( !m_foundImages.AddFileAttr( pFileAttr ) )
		return false;			// found duplicate image path (could happen with multiple embedded albums referencing the same image)

	if ( m_pChainEnum != NULL )
		m_pChainEnum->AddFoundFile( pFileAttr->GetPath().GetPtr() );

//Sleep( 100 );			// debug progress bar
	return true;
}

void CImageFileEnumerator::PushMany( const std::vector< CFileAttr* >& fileAttrs )
{
	// go via filtering
	for ( std::vector< CFileAttr* >::const_iterator itFileAttr = fileAttrs.begin(); itFileAttr != fileAttrs.end() && !MustStop(); ++itFileAttr )
		Push( *itFileAttr );
}

void CImageFileEnumerator::OnAddFileInfo( const CFileFind& foundFile )
{
	fs::CPath filePath = foundFile.GetFilePath().GetString();

	if ( app::IsAlbumFile( filePath.GetPtr() ) )		// found a catalog storage?
	{
		if ( CanRecurse() )		// treat found storages as sub-directories
			AddFoundFile( filePath.GetPtr() );
	}
	else
		Push( new CFileAttr( foundFile ) );
}

void CImageFileEnumerator::AddFoundFile( const TCHAR* pFilePath )
{
	fs::CPath filePath( pFilePath );

	if ( app::IsAlbumFile( filePath.GetPtr() ) )
	{
		// found an album (slide file or compound image catalog storage): load its metadata as found images
		// note: we need to load as CAlbumDoc since its document schema may be older (backwards compatibility)

		std::auto_ptr< CAlbumDoc > pAlbumDoc = CAlbumDoc::LoadAlbumDocument( filePath );
		if ( pAlbumDoc.get() != NULL )
		{
			AddFoundSubDir( filePath.GetPtr() );							// an album counts as a sub-directory

			std::vector< CFileAttr* > albumFileAttrs;
			pAlbumDoc->RefModel()->SwapFileAttrs( albumFileAttrs );			// take ownership of found image attributes

			if ( !albumFileAttrs.empty() )
			{
				PushMany( albumFileAttrs );

				if ( app::IsCatalogFile( filePath.GetPtr() ) )
					m_foundImages.AddStoragePath( filePath );
				else
				{	// augment embedded catalog storages in a .sld
					std::vector< fs::CPath > subStoragePaths;
					pAlbumDoc->GetModel()->QueryEmbeddedStorages( subStoragePaths );

					for ( std::vector< fs::CPath >::const_iterator itStoragePath = subStoragePaths.begin(); itStoragePath != subStoragePaths.end(); ++itStoragePath )
						m_foundImages.AddStoragePath( *itStoragePath );
				}
			}
		}
	}
	else if ( wic::IsValidFileImageFormat( filePath.GetPtr() ) && filePath.FileExist() )
		Push( new CFileAttr( filePath ) );
}
