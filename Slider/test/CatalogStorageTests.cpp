
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds

#include "CatalogStorageTests.h"
#include "CatalogStorageService.h"
#include "ICatalogStorage.h"
#include "AlbumDoc.h"
#include "SearchPattern.h"
#include "utl/AppTools.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileEnumerator.h"
#include "utl/RuntimeException.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImage.h"

#define new DEBUG_NEW


namespace ut
{
	CComPtr< ICatalogStorage > CreateArchiveStorageFile( const fs::CPath& docStgPath, const std::vector< fs::CPath >& srcImagePaths ) throws_( CException* )
	{
		CCatalogStorageService storageSvc;

		storageSvc.BuildFromSrcPaths( srcImagePaths );			// will create an ad-hoc album based on the transfer attributes, to save to "_Album.sld" stream

		CComPtr< ICatalogStorage > pCatalogStorage = CCatalogStorageFactory::CreateStorageObject();

		pCatalogStorage->CreateImageArchiveFile( docStgPath, &storageSvc );					// create the entire image archive: works internally in utl::ThrowMode
		return pCatalogStorage;
	}
}


CCatalogStorageTests::CCatalogStorageTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCatalogStorageTests& CCatalogStorageTests::Instance( void )
{
	static CCatalogStorageTests s_testCase;
	return s_testCase;
}

void CCatalogStorageTests::TestBuildImageArchive( void )
{
	const fs::CPath& imageDirPath = ut::GetStdImageDirPath();
	if ( imageDirPath.IsEmpty() )
		return;

	const fs::CPath docStgPath = ut::CTempFilePool::MakePoolDirPath().GetParentPath() / _T("ImageArchive.ias");
	size_t srcImageCount = 0;

	try
	{
		// create image archive storage
		{
			fs::CPathEnumerator srcFound( fs::TEnumFlags::Make( fs::EF_Recurse | fs::EF_ResolveShellLinks ) );
			fs::EnumFiles( &srcFound, imageDirPath, NULL );
			srcImageCount = srcFound.m_filePaths.size();

			CComPtr< ICatalogStorage > pCatalogStorage = ut::CreateArchiveStorageFile( docStgPath, srcFound.m_filePaths );
			ASSERT_PTR( pCatalogStorage );

			ENSURE( fs::IsValidStructuredStorage( docStgPath.GetPtr() ) );
		}

		_TestLoadImageArchive( docStgPath, srcImageCount );
	}
	catch ( CException* pExc )
	{
		ASSERT_EQUAL( _T(""), mfc::CRuntimeException::MessageOf( *pExc ) );
		pExc->Delete();
	}
}

void CCatalogStorageTests::TestAlbumSaveAs( void )
{
	CAlbumDoc albumDoc;
	fs::CPath imagesDirPath = ut::GetDestImagesDirPath();

	ASSERT( albumDoc.BuildAlbum( imagesDirPath ) );				// search for test images
	ASSERT( albumDoc.GetModel()->GetFileAttrCount() != 0 );		// has found files
}


void CCatalogStorageTests::_TestLoadImageArchive( const fs::CPath& docStgPath, size_t srcImageCount ) throws_( CException* )
{
	// catalog not yet opened for reading
	ASSERT_NULL( CCatalogStorageFactory::Instance()->FindStorage( docStgPath ) );

	// load the image archive storage
	{
		CComPtr< ICatalogStorage > pCatalogStorage = CCatalogStorageFactory::CreateStorageObject();

		ASSERT( pCatalogStorage->GetDocStorage()->OpenDocFile( docStgPath, STGM_READ ) );

		{
			ICatalogStorage* pOpenCatalogStorage = CCatalogStorageFactory::Instance()->FindStorage( docStgPath );
			ASSERT_PTR( pOpenCatalogStorage );
			ASSERT( pOpenCatalogStorage->GetDocStorage()->IsOpenForReading() );
		}

		std::auto_ptr< CAlbumDoc > pAlbumDoc = CAlbumDoc::LoadAlbumDocument( docStgPath );
		ASSERT_PTR( pAlbumDoc.get() );

		const CImagesModel* pImagesModel = &pAlbumDoc->GetModel()->GetImagesModel();
		ASSERT_EQUAL( srcImageCount, pImagesModel->GetFileAttrs().size() );

		const std::vector< CFileAttr* >& fileAttrs = pImagesModel->GetFileAttrs();

		for ( size_t i = 0; i != fileAttrs.size(); ++i )
			_TestAlbumFileAttr( pCatalogStorage, fileAttrs[ i ] );
	}

	// ensure catalog no longer opened for reading
	ASSERT_NULL( CCatalogStorageFactory::Instance()->FindStorage( docStgPath ) );
}

void CCatalogStorageTests::_TestAlbumFileAttr( ICatalogStorage* pCatalogStorage, const CFileAttr* pFileAttr ) throws_( CException* )
{
	ASSERT_PTR( pCatalogStorage );
	ASSERT( pCatalogStorage->GetDocStorage()->IsOpen() );
	ASSERT_PTR( pFileAttr );

	ASSERT( pFileAttr->GetPath().IsComplexPath() );
	ASSERT( pFileAttr->IsValid() );

	// test loading embedded images
	{
		UINT frameCount = CWicImage::LookupImageFileFrameCount( pFileAttr->GetPath() ).first;
		ASSERT( frameCount >= 1 );

		// verify each frame
		for ( UINT framePos = 0; framePos != frameCount; ++framePos )
		{
			fs::ImagePathKey frameKey( pFileAttr->GetPath(), framePos );

			ASSERT( !CWicImage::IsCorruptFrame( frameKey ) );

			std::auto_ptr< CWicImage > pFrameImage( CWicImage::CreateFromFile( frameKey, utl::ThrowMode ) );
			ASSERT_PTR( pFrameImage.get() );
		}
	}

	// test loading embedded thumbnails
	{
		std::auto_ptr< CCachedThumbBitmap > pThumbnail( pCatalogStorage->LoadThumbnail( pFileAttr->GetPath() ) );		// loaded, not cached
		ASSERT_PTR( pThumbnail.get() );
	}
}


void CCatalogStorageTests::Run( void )
{
	__super::Run();

	TestBuildImageArchive();
	TestAlbumSaveAs();
}


#endif //_DEBUG
