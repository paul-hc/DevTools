
#include "stdafx.h"
#include "ThumbnailTests.h"
#include "Application.h"
#include "utl/StructuredStorage.h"
#include "utl/Thumbnailer.h"
#include "utl/ut/TestToolWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG


namespace ut
{
	static CThumbnailer* GetThumbnailer( void )
	{
		static std::auto_ptr< CThumbnailer > pThumbnailer;
		if ( NULL == pThumbnailer.get() )
		{
			pThumbnailer.reset( new CThumbnailer );
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
		VERIFY( docStg.Create( path::Combine( CThumbnailTests::GetThumbSaveDirPath().GetPtr(), _T("myThumbs.stg") ).c_str() ) );

		for ( UINT i = 0; i != COUNT_OF( extensions ); ++i )
		{
			fs::CPathParts parts;
			parts.m_fname = str::Format( _T("thumb %s_%d"), pSrcFnameExt, i + 1 );
			parts.m_ext = extensions[ i ];

			const GUID& containerFormatId = wic::CBitmapOrigin::FindContainerFormatId( parts.m_ext.c_str() );
			CComPtr< IStream > pThumbStream = docStg.CreateStream( parts.MakePath().c_str() );
			
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
	static CThumbnailTests testCase;
	return testCase;
}

const fs::CPath& CThumbnailTests::GetThumbSaveDirPath( void )
{
	static fs::CPath dirPath = GetTestImagesDirPath() / fs::CPath( _T("thumbnails") );
	if ( !dirPath.IsEmpty() && !fs::CreateDirPath( dirPath.GetPtr() ) )
	{
		TRACE( _T("\n * Cannot create the local save directory for thumbs: %s\n"), dirPath.GetPtr() );
		dirPath.Clear();
	}
	return dirPath;
}

void CThumbnailTests::DrawThumbs( const std::vector< CBitmap* >& thumbs )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd(), ut::TileRight );
	testDev.GotoOrigin();

	CDC memDC;
	if ( !memDC.CreateCompatibleDC( testDev.GetDC() ) )
		return;

	for ( size_t i = 0; i != thumbs.size(); ++i )
	{
		CBitmap* pBitmap = thumbs[ i ];
		CBitmapInfo bmpInfo( *pBitmap );
		ASSERT( bmpInfo.IsDibSection() );

		testDev.DrawBitmap( *pBitmap, ut::GetThumbnailer()->GetBoundsSize(), &memDC );
		if ( !testDev.CascadeNextTile() )
			break;
	}
}

void CThumbnailTests::TestThumbConversion( void )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Flamingos_jpg );
	if ( !imagePath.IsEmpty() )
		if ( CCachedThumbBitmap* pThumbBitmap = ut::GetThumbnailer()->AcquireThumbnail( imagePath ) )
		{
			ut::SaveThumbnailToFiles( pThumbBitmap->GetWicBitmap(), imagePath.GetNameExt() );
			ut::SaveThumbnailToDocStorage( pThumbBitmap->GetWicBitmap(), imagePath.GetNameExt() );
		}
}

void CThumbnailTests::TestImageThumbs( void )
{
	if ( GetImageSourceDirPath().IsEmpty() )
		return;

	fs::CPathEnumerator imageEnum;
	fs::EnumFiles( &imageEnum, GetImageSourceDirPath().GetPtr(), _T("*.*") );

	std::vector< CBitmap* > thumbs;
	thumbs.reserve( MaxImageFiles );

	shell::CWinExplorer explorer;
	UINT count = 0;
	for ( fs::TPathSet::const_iterator itFilePath = imageEnum.m_filePaths.begin(); itFilePath != imageEnum.m_filePaths.end() && count != MaxImageFiles; ++itFilePath, ++count )
	{
		if ( CComPtr< IShellItem > pShellItem = explorer.FindShellItem( *itFilePath ) )
			if ( HBITMAP hThumbBitmap = explorer.ExtractThumbnail( pShellItem, ut::GetThumbnailer()->GetBoundsSize(), SIIGBF_BIGGERSIZEOK ) )		// SIIGBF_RESIZETOFIT, SIIGBF_BIGGERSIZEOK
			{
				thumbs.push_back( new CBitmap );
				thumbs.back()->Attach( hThumbBitmap );
			}

		// or do forced extraction:
		// HBITMAP hThumbBitmap = explorer.ExtractThumbnail( *itFilePath, ut::GetThumbnailer()->GetBoundsSize() )
	}

	DrawThumbs( thumbs );
	utl::ClearOwningContainer( thumbs );
}

void CThumbnailTests::TestThumbnailCache( void )
{
	if ( GetImageSourceDirPath().IsEmpty() )
		return;

	fs::CPathEnumerator imageEnum;
	fs::EnumFiles( &imageEnum, GetImageSourceDirPath().GetPtr(), _T("*.*") );

	std::vector< CBitmap* > thumbs;
	thumbs.reserve( MaxImageFiles );

	shell::CWinExplorer explorer;
	UINT count = 0;
	for ( fs::TPathSet::const_iterator itFilePath = imageEnum.m_filePaths.begin(); itFilePath != imageEnum.m_filePaths.end() && count != MaxImageFiles; ++itFilePath, ++count )
		if ( CCachedThumbBitmap* pThumbBitmap = ut::GetThumbnailer()->AcquireThumbnail( fs::ToFlexPath( *itFilePath ) ) )
			thumbs.push_back( pThumbBitmap );

	DrawThumbs( thumbs );
	// thumbs are owned by the cache, don't delete them
}

void CThumbnailTests::Run( void )
{
	__super::Run();

	TestThumbConversion();
//	TestImageThumbs();
	TestThumbnailCache();

	::Sleep( 200 );				// display results for a little while
}


#endif //_DEBUG