#ifndef BaseMainFrameWndEx_hxx
#define BaseMainFrameWndEx_hxx
#pragma once

#include "resource.h"
#include <afxtoolbar.h>


// CBaseFrameWnd template code

template< typename BaseFrameWnd >
CBaseMainFrameWndEx<BaseFrameWnd>::~CBaseMainFrameWndEx()
{
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseMainFrameWndEx, BaseFrameWnd, TBaseClass )
	ON_COMMAND( ID_WINDOW_MANAGER, OnWindowManager )
	ON_COMMAND( ID_VIEW_CUSTOMIZE, OnViewCustomize )
	ON_REGISTERED_MESSAGE( AFX_WM_CREATETOOLBAR, OnToolbarCreateNew )
END_MESSAGE_MAP()

template< typename BaseFrameWnd >
void CBaseMainFrameWndEx<BaseFrameWnd>::OnWindowManager( void )
{
	if ( CMDIFrameWndEx* pMdiFrameWnd = dynamic_cast<CMDIFrameWndEx*>( this ) )
		pMdiFrameWnd->ShowWindowsDialog();
}

template< typename BaseFrameWnd >
void CBaseMainFrameWndEx<BaseFrameWnd>::OnViewCustomize( void )
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog( this, TRUE /* scan menus */ );

	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
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
