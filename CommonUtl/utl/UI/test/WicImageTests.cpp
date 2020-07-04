
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "utl/FileEnumerator.h"
#include "test/WicImageTests.h"
#include "test/TestToolWnd.h"
#include "WicImage.h"
#include "WicImageCache.h"

#define new DEBUG_NEW

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

void CWicImageTests::TestImage( ut::CTestDevice* pTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	CWicImage image;
	UINT framePos = 0;
	ASSERT( image.LoadFromFile( imagePath, framePos ) );

	do
	{
		pTestDev->DrawBitmap( image.GetWicBitmap(), image.GetBmpFmt().m_size );
		++*pTestDev;
	}
	while ( image.IsValidFramePos( ++framePos ) && image.LoadFrame( framePos ) );

	pTestDev->GotoNextStrip();
}

void CWicImageTests::TestImageCache( ut::CTestDevice* pTestDev )
{
	if ( ut::GetImageSourceDirPath().IsEmpty() )
		return;

	fs::CEnumerator imageEnum;
	fs::EnumFiles( &imageEnum, ut::GetImageSourceDirPath(), _T("*.*") );

	fs::SortPaths( imageEnum.m_filePaths );

	std::vector< fs::CPath >::const_iterator itFilePath = imageEnum.m_filePaths.begin();

	enum { MaxSize = 10u, N5 = 5u, N15 = 15u };

	// enqueue 5 images files
	std::vector< fs::ImagePathKey > imagePaths;
	for ( UINT count = 0; itFilePath != imageEnum.m_filePaths.end() && count != N5; ++itFilePath, ++count )
		imagePaths.push_back( fs::ImagePathKey( itFilePath->Get(), 0 ) );

	CWicImageCache cache( MaxSize );

	cache.Enqueue( imagePaths );
	cache.GetCache()->WaitPendingQueue();

	ASSERT_EQUAL( imagePaths.size(), cache.GetCount() );

	std::pair< CWicImage*, int > imagePair;

	for ( size_t i = 0; i != imagePaths.size(); ++i )
	{
		imagePair = cache.Acquire( imagePaths[ i ] );
		ASSERT_PTR( imagePair.first );
		ASSERT_EQUAL( fs::cache::CacheHit, imagePair.second );
		pTestDev->DrawBitmap( imagePair.first->GetWicBitmap(), CSize( 150, 150 ) );
		++*pTestDev;
	}

	// acquire 1 more image file
	ASSERT_EQUAL( fs::cache::Load, cache.Acquire( fs::ImagePathKey( itFilePath->Get(), 0 ) ).second );
	++itFilePath;

	// enqueue 15 more images files
	imagePaths.clear();
	for ( UINT count = 0; itFilePath != imageEnum.m_filePaths.end() && count != N15; ++itFilePath, ++count )
		imagePaths.push_back( fs::ImagePathKey( itFilePath->Get(), 0 ) );

	cache.Enqueue( imagePaths );
	cache.GetCache()->WaitPendingQueue();

	ASSERT( cache.GetCount() <= MaxSize );			// ensure it doesn't overflow MaxSize
	Sleep( 250 );
}

void CWicImageTests::Run( void )
{
	__super::Run();

	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ), ut::TileRight );
	testDev.GotoOrigin();

	TestImage( &testDev );
	TestImageCache( &testDev );
}


#endif //_DEBUG
