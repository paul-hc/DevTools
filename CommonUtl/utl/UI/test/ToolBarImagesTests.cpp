
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ToolBarImagesTests.h"
#include "test/TestToolWnd.h"
#include <afxtoolbar.h>
#include <afxtoolbarimages.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CToolBarImagesTests::CToolBarImagesTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CToolBarImagesTests& CToolBarImagesTests::Instance( void )
{
	static CToolBarImagesTests s_testCase;
	return s_testCase;
}

void CToolBarImagesTests::TestToolBarImagesBitmap( ut::CTestDevice& rTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	rTestDev.DrawImages( CMFCToolBar::GetImages() );
}

void CToolBarImagesTests::TestToolBarImagesDetails( ut::CTestDevice& rTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	CMFCToolBarImages* pImages = CMFCToolBar::GetImages();
	int imageCount = pImages->GetCount();

	rTestDev.DrawImagesDetails( pImages );

	rTestDev.DrawTileCaption( str::Format( _T("%d total images"), imageCount ) );
}

void CToolBarImagesTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ) );
	testDev.SetSubTitle( _T("CToolBarImagesTests") );

	RUN_TESTDEV_1( TestToolBarImagesBitmap, testDev );
	RUN_TESTDEV_1( TestToolBarImagesDetails, testDev );
}


#endif //USE_UT
