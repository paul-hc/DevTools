
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/FileEnumerator.h"
#include "test/WicImageTests.h"
#include "test/TestToolWnd.h"
#include "WicImage.h"
#include "WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "CacheLoader.hxx"		// WaitPendingQueue()


CWicImageTests::CWicImageTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CWicImageTests& CWicImageTests::Instance( void )
{
	static CWicImageTests s_testCase;
	return s_testCase;
}

void CWicImageTests::DisplayMultiFrameImageStrip( ut::CTestDevice* pTestDev, const fs::CFlexPath& imagePath )
{
	CWicImage image;
	UINT framePos = 0;
	ASSERT( image.LoadFromFile( imagePath, framePos ) );

	do
	{
		pTestDev->DrawBitmap( image.GetWicBitmap(), image.GetBmpFmt().m_size );
		pTestDev->DrawTileCaption( str::Format( _T("%d:%s"), framePos + 1, imagePath.GetFilenamePtr() ) );
		++*pTestDev;
	}
	while ( image.IsValidFramePos( ++framePos ) && image.LoadFrame( framePos ) );

	pTestDev->Await();
	pTestDev->GotoNextStrip();
}

void CWicImageTests::TestIconMultiFrame_Scissors( ut::CTestDevice* pTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CWicImageTests::TestIconMultiFrame_Scissors") );
	DisplayMultiFrameImageStrip( pTestDev, imagePath );
}

void CWicImageTests::TestIconMultiFrame_RedBubbles( ut::CTestDevice* pTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( RedBubbles_ico );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CWicImageTests::TestIconMultiFrame_RedBubbles") );
	DisplayMultiFrameImageStrip( pTestDev, imagePath );
}

void CWicImageTests::TestImageCache( ut::CTestDevice* pTestDev )
{
	const fs::TDirPath& imageSrcPath = ut::GetImageSourceDirPath();
	if ( imageSrcPath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CWicImageTests::TestImageCache") );

	fs::CPathEnumerator imageEnum;
	fs::EnumFiles( &imageEnum, imageSrcPath, _T("*.*") );

	fs::SortPaths( imageEnum.m_filePaths );

	std::vector<fs::CPath>::const_iterator itFilePath = imageEnum.m_filePaths.begin();

	enum { MaxSize = 10u, N5 = 5u, N15 = 15u };

	// enqueue 5 images files
	std::vector<fs::TImagePathKey> imagePaths;
	for ( UINT count = 0; itFilePath != imageEnum.m_filePaths.end() && count != N5; ++itFilePath, ++count )
		imagePaths.push_back( fs::TImagePathKey( itFilePath->Get(), 0 ) );

	CWicImageCache cache( MaxSize );

	cache.Enqueue( imagePaths );
	cache.GetCache()->WaitPendingQueue();

	ASSERT_EQUAL( imagePaths.size(), cache.GetCount() );

	std::pair<CWicImage*, int> imagePair;

	for ( size_t i = 0; i != imagePaths.size(); ++i )
	{
		imagePair = cache.Acquire( imagePaths[ i ] );
		ASSERT_PTR( imagePair.first );
		ASSERT_EQUAL( fs::cache::CacheHit, imagePair.second );
		pTestDev->DrawBitmap( imagePair.first->GetWicBitmap(), CSize( 150, 150 ) );
		pTestDev->DrawTileCaption( imagePair.first->GetImagePath().GetFilename() );
		++*pTestDev;
	}

	// acquire 1 more image file
	ASSERT_EQUAL( fs::cache::Load, cache.Acquire( fs::TImagePathKey( itFilePath->Get(), 0 ) ).second );
	++itFilePath;

	// enqueue 15 more images files
	imagePaths.clear();
	for ( UINT count = 0; itFilePath != imageEnum.m_filePaths.end() && count != N15; ++itFilePath, ++count )
		imagePaths.push_back( fs::TImagePathKey( itFilePath->Get(), 0 ) );

	cache.Enqueue( imagePaths );
	cache.GetCache()->WaitPendingQueue();

	ASSERT( cache.GetCount() <= MaxSize );			// ensure it doesn't overflow MaxSize

	pTestDev->Await();
	pTestDev->GotoNextStrip();
}

void CWicImageTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ) );

	RUN_TEST1( TestIconMultiFrame_Scissors, &testDev );
	RUN_TEST1( TestIconMultiFrame_RedBubbles, &testDev );
	RUN_TEST1( TestImageCache, &testDev );
}


#endif //USE_UT
