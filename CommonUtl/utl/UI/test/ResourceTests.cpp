
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ResourceTests.h"
#include "MenuUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CResourceTests::CResourceTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CResourceTests& CResourceTests::Instance( void )
{
	static CResourceTests s_testCase;
	return s_testCase;
}

void CResourceTests::TestDeepPopupIndex( void )
{
	ui::CPopupIndexPath popupIndexPath( 1 );

	ASSERT_EQUAL( 1, popupIndexPath.GetDepth() );
	ASSERT_EQUAL( 1, popupIndexPath.GetPopupIndexAt( 0 ) );

	popupIndexPath = ui::CPopupIndexPath( 1, 3 );
	ASSERT_EQUAL( 2, popupIndexPath.GetDepth() );
	ASSERT_EQUAL( 1, popupIndexPath.GetPopupIndexAt( 0 ) );
	ASSERT_EQUAL( 3, popupIndexPath.GetPopupIndexAt( 1 ) );

	popupIndexPath = ui::CPopupIndexPath( 1, 3, 5 );
	ASSERT_EQUAL( 3, popupIndexPath.GetDepth() );
	ASSERT_EQUAL( 1, popupIndexPath.GetPopupIndexAt( 0 ) );
	ASSERT_EQUAL( 3, popupIndexPath.GetPopupIndexAt( 1 ) );
	ASSERT_EQUAL( 5, popupIndexPath.GetPopupIndexAt( 2 ) );

	popupIndexPath = ui::CPopupIndexPath( 1, 3, 5, 7 );
	ASSERT_EQUAL( 4, popupIndexPath.GetDepth() );
	ASSERT_EQUAL( 1, popupIndexPath.GetPopupIndexAt( 0 ) );
	ASSERT_EQUAL( 3, popupIndexPath.GetPopupIndexAt( 1 ) );
	ASSERT_EQUAL( 5, popupIndexPath.GetPopupIndexAt( 2 ) );
	ASSERT_EQUAL( 7, popupIndexPath.GetPopupIndexAt( 3 ) );

	popupIndexPath = ui::CPopupIndexPath( 22, 33, 44, 55 );
	ASSERT_EQUAL( 4, popupIndexPath.GetDepth() );
	ASSERT_EQUAL( 22, popupIndexPath.GetPopupIndexAt( 0 ) );
	ASSERT_EQUAL( 33, popupIndexPath.GetPopupIndexAt( 1 ) );
	ASSERT_EQUAL( 44, popupIndexPath.GetPopupIndexAt( 2 ) );
	ASSERT_EQUAL( 55, popupIndexPath.GetPopupIndexAt( 3 ) );
}

void CResourceTests::TestLoadPopupMenu( void )
{
	ui::UseMenuImages menuImages = ui::NoMenuImages;
	CMenu contextMenu;
	std::tstring popupText;

	// depth 1
	ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::TestPopup, menuImages, &popupText );
	ASSERT_EQUAL( _T("<test-popup L0>"), popupText );

		// depth 2
		ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 0 ), menuImages, &popupText );
		ASSERT_EQUAL( _T("Popup L1.A [of L0]"), popupText );
			// depth 3
			ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 0, 0 ), menuImages, &popupText );
			ASSERT_EQUAL( _T("Popup L2.A [of Popup L1.A]"), popupText );
				// depth 4
				ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 0, 0, 0 ), menuImages, &popupText );
				ASSERT_EQUAL( _T("Popup L3.A [of Popup L2.A]"), popupText );

		// depth 2
		ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 1 ), menuImages, &popupText );
		ASSERT_EQUAL( _T("Popup L1.B [of L0]"), popupText );
			// depth 3
			ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 1, 1 ), menuImages, &popupText );
			ASSERT_EQUAL( _T("Popup L2.B [of Popup L1.B]"), popupText );
				// depth 4
				ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::TestPopup, 1, 1, 1 ), menuImages, &popupText );
				ASSERT_EQUAL( _T("Popup L3.B [of Popup L2.B]"), popupText );
}


void CResourceTests::Run( void )
{
	RUN_TEST( TestDeepPopupIndex );
	RUN_TEST( TestLoadPopupMenu );
}


#endif //USE_UT
