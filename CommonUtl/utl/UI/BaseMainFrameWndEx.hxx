#ifndef BaseMainFrameWndEx_hxx
#define BaseMainFrameWndEx_hxx
#pragma once

#include "MenuUtilities.h"
#include "ControlBar_fwd.h"		// for mfc::FrameToolbarStyle (required in the final main frame)
#include "resource.h"


// CBaseFrameWnd template code

template< typename BaseFrameWnd >
CBaseMainFrameWndEx<BaseFrameWnd>::~CBaseMainFrameWndEx()
{
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseMainFrameWndEx, BaseFrameWnd, TBaseClass )
	ON_WM_INITMENUPOPUP()
	ON_COMMAND( ID_WINDOW_MANAGER, OnWindowManager )
	ON_REGISTERED_MESSAGE( AFX_WM_CREATETOOLBAR, OnToolbarCreateNew )
END_MESSAGE_MAP()

template< typename BaseFrameWnd >
void CBaseMainFrameWndEx<BaseFrameWnd>::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	if ( !isSysMenu )
	{
		ui::ReplaceMenuItemWithPopup( pPopupMenu, ID_APPLOOK_POPUP, IDR_STD_POPUPS_MENU, ui::AppLookPopup );
	}

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

template< typename BaseFrameWnd >
void CBaseMainFrameWndEx<BaseFrameWnd>::OnWindowManager( void )
{
	if ( CMDIFrameWndEx* pMdiFrameWnd = dynamic_cast<CMDIFrameWndEx*>( this ) )
		pMdiFrameWnd->ShowWindowsDialog();
}

template< typename BaseFrameWnd >
LRESULT CBaseMainFrameWndEx<BaseFrameWnd>::OnToolbarCreateNew( WPARAM wParam, LPARAM lParam )
{
	LRESULT result = __super::OnToolbarCreateNew( wParam, lParam );

	if ( result != 0 )
	{
		CMFCToolBar* pUserToolbar = (CMFCToolBar*)result;
		ASSERT_PTR( pUserToolbar );

		CString customizeLabel;
		VERIFY( customizeLabel.LoadString( ID_VIEW_CUSTOMIZE ) );

		pUserToolbar->EnableCustomizeButton( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel );
	}

	return result;
}


#endif // BaseMainFrameWndEx_hxx
