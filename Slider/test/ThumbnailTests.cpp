
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "ThumbnailTests.h"
#include "Application.h"
#include "utl/ContainerOwnership.h"
#include "utl/FileEnumerator.h"
#include "utl/StructuredStorage.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/test/TestToolWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static CThumbnailer* GetThumbnailer( void )
	{
		static std::auto_ptr<CThumbnailer> pThumbnailer;
		if ( nullptr == pThumbnailer.get() )
		{
			pThumbnailer.reset( new CThumbnailer() );
			app::GetApp()->GetSharedResources().AddAutoPtr( &pThumbnailer );			// auto-reset in ExitInstance()
		}
		return pThumbnailer.get();
	}

	bool SaveThumbnailToFiles( IWICBitmapSource* pThumbBitmap, const TCHAR* pSrcFnameExt )
	{
		if ( CThumbnailTests::GetThumbSaveDirPath().IsEmpty() )
			return false;

		static const TCHAR* extensions[] = { _T(".bmp"), _T(".jpg"), _T(".png"), _T(".tif"), _T(".gif") };

		for ( UINT i = 0; i != COUNT_OF( extensions ); ++i )
		{
			fs::CPath destFilePath = CThumbnailTests::GetThumbSaveDirPath() / str::Format( _T("thumb %s_%d%s"), pSrcFnameExt, i + 1, extensions[ i ] );
			if ( !wic::SaveBitmapToFile( pThumbBitmap, destFilePath.GetPtr() ) )
				return false;
		}
		return true;
	}

	bool SaveThumbnailToDocStorage( IWICBitmapSource* pThumbBitmap, const TCHAR* pSrcFnameExt )
	{
		if ( CThumbnailTests::GetThumbSaveDirPath().IsEmpty() )
			return false;

		static const TCHAR* extensions[] = { _T(".bmp"), _T(".jpg"), _T(".png"), _T(".tif"), _T(".gif") };

		fs::CStructuredStorage docStg;
		VERIFY( docStg.CreateDocFile( CThumbnailTests::GetThumbSaveDirPath() / _T("myThumbs.stg") ) );

		for ( UINT i = 0; i != COUNT_OF( extensions ); ++i )
		{
			fs::CPathParts parts;
			parts.m_fname = str::Format( _T("thumb %s_%d"), pSrcFnameExt, i + 1 );
			parts.m_ext = extensions[ i ];

			const GUID& containerFormatId = wic::CBitmapOrigin::FindContainerFormatId( parts.m_ext.c_str() );
			CComPtr<IStream> pThumbStream = docStg.CreateStream( parts.MakePath().GetPtr() );
			
			if ( !wic::SaveBitmapToStream( pThumbBitmap, pThumbStream, containerFormatId ) )
				return false;
		}
		return true;
	}
}


CThumbnailTests::CThumbnailTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CThumbnailTests& CThumbnailTests::Instance( void )
{
	static CThumbnailTests s_testCase;
	return s_testCase;
}

const fs::TDirPath& CThumbnailTests::GetThumbSaveDirPath( void )
{
	static fs::TDirPath dirPath = ut::GetDestImagesDirPath() / fs::TDirPath( _T("thumbnails") );
	if ( !dirPath.IsEmpty() && !fs::CreateDirPath( dirPath.GetPtr() ) )
	{
		TRACE( _T("\n * Cannot create the local save directory for thumbs: %s\n"), dirPath.GetPtr() );
		dirPath.Clear();
	}
	return dirPath;
}

void CThumbnailTests::DrawThumbs( ut::CTestDevice* pTestDev, const std::vector<TBitmapPathPair>& thumbs )
{
	pTestDev->ResetOrigin();

	CDC memDC;
	if ( !memDC.CreateCompatibleDC( pTestDev->GetDC() ) )
		return;

	for ( size_t i = 0; i != thumbs.size(); ++i )
	{
		CBitmap* pBitmap = thumbs[ i ].first;
		CBitmapInfo bmpInfo( *pBitmap );
		ASSERT( bmpInfo.IsDibSection() );

		pTestDev->DrawBitmap( *pBitmap, ut::GetThumbnailer()->GetBoundsSize(), &memDC );
		pTestDev->DrawTileCaption( thumbs[ i ].second );

		if ( !pTestDev->CascadeNextTile() )
			break;
	}

	pTestDev->Await();
	pTestDev->GotoNextStrip();
}

void CThumbnailTests::TestThumbConversion( void )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Flamingos_jpg );
	if ( !imagePath.IsEmpty() )
		if ( CCachedThumbBitmap* pThumbBitmap = ut::GetThumbnailer()->AcquireThumbnail( imagePath ) )
		{
			ut::SaveThumbnailToFiles( pThumbBitmap->GetWicBitmap(), imagePath.GetFilenamePtr() );
			ut::SaveThumbnailToDocStorage( pThumbBitmap->GetWicBitmap(), imagePath.GetFilenamePtr() );
		}
}

void CThumbnailTests::TestImageThumbs( ut::CTestDevice* pTestDev )
{
	const fs::TDirPath& imageSrcPath = ut::GetStdImageDirPath();
	if ( imageSrcPath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CThumbnailTests::TestImageThumbs") );

	fs::CPathEnumerator imageEnum( fs::EF_Recurse );
	fs::EnumFiles( &imageEnum, imageSrcPath, _T("*.*") );

	fs::SortPaths( imageEnum.m_filePaths );

	std::vector<TBitmapPathPair> thumbs;
	thumbs.reserve( MaxImageFiles );

	shell::CWinExplorer explorer;
	UINT count = 0;
	for ( std::vector<fs::CPath>::const_iterator itFilePath = imageEnum.m_filePaths.begin(); itFilePath != imageEnum.m_filePaths.end() && count != MaxImageFiles; ++itFilePath, ++count )
	{
		if ( CComPtr<IShellItem> pShellItem = explorer.FindShellItem( *itFilePath ) )
			if ( HBITMAP hThumbBitmap = explorer.ExtractThumbnail( pShellItem, ut::GetThumbnailer()->GetBoundsSize(), SIIGBF_BIGGERSIZEOK ) )		// SIIGBF_RESIZETOFIT, SIIGBF_BIGGERSIZEOK
			{
				thumbs.push_back( TBitmapPathPair( new CBitmap(), itFilePath->GetFilename() ) );
				thumbs.back().first->Attach( hThumbBitmap );
				continue;
			}

		// or do forced extraction:
		// HBITMAP hThumbBitmap = explorer.ExtractThumbnail( *itFilePath, ut::GetThumbnailer()->GetBoundsSize() )

		ASSERT( false );		// failed thumbnail extraction?
	}

	DrawThumbs( pTestDev, thumbs );
	utl::ClearOwningMapKeys( thumbs );
}

void CThumbnailTests::TestThumbnailCache( ut::CTestDevice* pTestDev )
{
	const fs::TDirPath& imageSrcPath = ut::GetStdImageDirPath();
	if ( imageSrcPath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CThumbnailTests::TestThumbnailCache") );

	fs::CPathEnumerator imageEnum( fs::EF_Recurse );
	fs::EnumFiles( &imageEnum, imageSrcPath, _T("*.*") );

	fs::SortPaths( imageEnum.m_filePaths );

	std::vector<TBitmapPathPair> thumbs;
	thumbs.reserve( MaxImageFiles );

	shell::CWinExplorer explorer;
	UINT count = 0;
	for ( std::vector<fs::CPath>::const_iterator itFilePath = imageEnum.m_filePaths.begin(); itFilePath != imageEnum.m_filePaths.end() && count != MaxImageFiles; ++itFilePath, ++count )
	{
		if ( CCachedThumbBitmap* pThumbBitmap = ut::GetThumbnailer()->AcquireThumbnail( fs::ToFlexPath( *itFilePath ) ) )
		{
			thumbs.push_back( TBitmapPathPair( pThumbBitmap, itFilePath->GetFilename() ) );
			continue;
		}

		ASSERT( false );		// failed thumbnail extraction?
	}

	DrawThumbs( pTestDev, thumbs );
	// thumbs are owned by the cache, don't delete them
}

void CThumbnailTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd() );

	RUN_TEST( TestThumbConversion );
	RUN_TEST1( TestImageThumbs, &testDev );
	RUN_TEST1( TestThumbnailCache, &testDev );
}


#endif //USE_UT
