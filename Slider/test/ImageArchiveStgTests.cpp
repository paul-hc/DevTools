
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ImageArchiveStgTests.h"
#include "ImageStorageService.h"
#include "ImageArchiveStg.h"
#include "AlbumDoc.h"
#include "utl/AppTools.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileEnumerator.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImage.h"

#define new DEBUG_NEW


namespace ut
{
	void CreateArchiveStorageFile( CImageArchiveStg* pImageStorage, const fs::CPath& docStgPath, const std::vector< fs::CPath >& srcImagePaths ) throws_( CException* )
	{
		ASSERT_PTR( pImageStorage );
		ASSERT( !pImageStorage->IsOpen() );

		CImageStorageService storageSvc;
		storageSvc.BuildFromSrcPaths( srcImagePaths );

		utl::COwningContainer< std::vector< CFileAttr* > > albumFileAttrs;
		storageSvc.MakeAlbumFileAttrs( albumFileAttrs );		// clone album file attributes before they get altered

		// create the archive core content
		pImageStorage->CreateImageArchive( docStgPath, &storageSvc );

		CAlbumDoc albumDoc;

		albumDoc.RefModel()->SwapFileAttrs( albumFileAttrs );
		if ( !storageSvc.IsEmpty() )
			albumDoc.m_slideData.SetCurrentIndex( 0 );

		albumDoc.SaveAlbumToArchiveStg( docStgPath );
	}

	std::auto_ptr< CImagesModel > LoadArchiveStorageAlbum( const fs::CPath& docStgPath )
	{
		ASSERT_PTR( CImageArchiveStg::Factory()->FindStorage( docStgPath ) );		// the archive storage must be open for reading

		std::auto_ptr< CImagesModel > pImagesModel;
		CAlbumDoc albumDoc;

		if ( CAlbumDoc::Succeeded == albumDoc.LoadArchiveStorage( docStgPath ) )
		{
			pImagesModel.reset( new CImagesModel() );
			pImagesModel->Swap( albumDoc.RefModel()->RefImagesModel() );
		}

		return pImagesModel;
	}
}


CImageArchiveStgTests::CImageArchiveStgTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CImageArchiveStgTests& CImageArchiveStgTests::Instance( void )
{
	static CImageArchiveStgTests s_testCase;
	return s_testCase;
}

void CImageArchiveStgTests::TestBuildImageArchive( void )
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
			fs::CEnumerator srcFound;
			fs::EnumFiles( &srcFound, imageDirPath, NULL, Deep );
			srcImageCount = srcFound.m_filePaths.size();

			CImageArchiveStg imageStorage;
			ut::CreateArchiveStorageFile( &imageStorage, docStgPath, srcFound.m_filePaths );

			ENSURE( fs::CStructuredStorage::IsValidDocFile( docStgPath.GetPtr() ) );
		}

		_TestLoadImageArchive( docStgPath, srcImageCount );
	}
	catch ( CException* pExc )
	{
		app::TraceException( pExc );
		ASSERT_EQUAL( _T(""), mfc::CRuntimeException::MessageOf( *pExc ) );
		pExc->Delete();
	}
}

void CImageArchiveStgTests::_TestLoadImageArchive( const fs::CPath& docStgPath, size_t srcImageCount ) throws_( CException* )
{
	// load the image archive storage
	{
		CImageArchiveStg imageStorage;
		ASSERT( imageStorage.OpenDocFile( docStgPath.GetPtr(), STGM_READ ) );

		std::auto_ptr< CImagesModel > pImagesModel = ut::LoadArchiveStorageAlbum( docStgPath );
		ASSERT_PTR( pImagesModel.get() );
		ASSERT_EQUAL( srcImageCount, pImagesModel->GetFileAttrs().size() );

		const std::vector< CFileAttr* >& fileAttrs = pImagesModel->GetFileAttrs();

		for ( size_t i = 0; i != fileAttrs.size(); ++i )
			_TestAlbumFileAttr( &imageStorage, fileAttrs[ i ] );
	}
}

void CImageArchiveStgTests::_TestAlbumFileAttr( CImageArchiveStg* pImageStorage, const CFileAttr* pFileAttr ) throws_( CException* )
{
	ASSERT_PTR( pImageStorage );
	ASSERT( pImageStorage->IsOpen() );
	ASSERT_PTR( pFileAttr );

	ASSERT( pFileAttr->GetPath().IsComplexPath() );
	ASSERT( pFileAttr->IsValid() );

	// test loading embedded images
	{
		UINT imageFrameCount = CWicImage::LookupImageFileFrameCount( pFileAttr->GetPath() ).first;

		// verify each frame
		for ( UINT framePos = 0; framePos != imageFrameCount; ++framePos )
			if ( framePos != pFileAttr->GetPathKey().second )
			{
				fs::ImagePathKey frameKey( pFileAttr->GetPath(), framePos );

				ASSERT( !CWicImage::IsCorruptFrame( frameKey ) );

				std::auto_ptr< CWicImage > pFrameImage( CWicImage::CreateFromFile( frameKey, true ) );
				ASSERT_PTR( pFrameImage.get() );
			}
	}

	// test loading embedded thumbnails
	{
		std::auto_ptr< CCachedThumbBitmap > pThumbnail( pImageStorage->LoadThumbnail( pFileAttr->GetPath() ) );		// loaded, not cached
		ASSERT_PTR( pThumbnail.get() );
	}
}


void CImageArchiveStgTests::Run( void )
{
	__super::Run();

	TestBuildImageArchive();
}


#endif //_DEBUG
