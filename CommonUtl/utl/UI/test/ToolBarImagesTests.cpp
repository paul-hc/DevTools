
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

void CToolBarImagesTests::TestToolBarImagesBitmap( ut::CTestDevice* pTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CToolBarImagesTests::TestToolBarImagesBitmap") );
	pTestDev->DrawHeadline( _T("CMFCToolBar::GetImages()") );

	pTestDev->DrawImages( CMFCToolBar::GetImages() );

	pTestDev->Await( 1000 );
}

void CToolBarImagesTests::TestToolBarImagesDetails( ut::CTestDevice* pTestDev )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Scissors_ico );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CToolBarImagesTests::TestToolBarImagesDetails") );

	CMFCToolBarImages* pImages = CMFCToolBar::GetImages();
	int imageCount = pImages->GetCount();

	pTestDev->DrawImagesDetails( pImages );

	pTestDev->DrawTileCaption( str::Format( _T("%d total images"), imageCount ) );

	pTestDev->Await( 1000 );
	pTestDev->GotoNextStrip();
}

void CToolBarImagesTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ) );

	RUN_TEST1( TestToolBarImagesBitmap, &testDev );
	RUN_TEST1( TestToolBarImagesDetails, &testDev );
}


#endif //USE_UT
