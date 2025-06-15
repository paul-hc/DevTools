#ifndef BaseMainFrameWndEx_hxx
#define BaseMainFrameWndEx_hxx
#pragma once

#include "MenuUtilities.h"
#include "ControlBar_fwd.h"		// for mfc::FrameToolbarStyle (required in the final main frame)
#include "BaseApp.h"
#include "resource.h"


// CBaseFrameWnd template code

template< typename BaseFrameWndT >
CBaseMainFrameWndEx<BaseFrameWndT>::~CBaseMainFrameWndEx()
{
}

template< typename BaseFrameWndT >
BOOL CBaseMainFrameWndEx<BaseFrameWndT>::PreCreateWindow( CREATESTRUCT& cs )
{
	if ( !str::IsEmpty( cs.lpszName ) )		// called twice during frame creation: skip fist time when cs.lpszName is nullptr
	{
		const std::tstring& appNameSuffix = DYNAMIC_CAST_BASE_APP( GetAppNameSuffix() );

		if ( !appNameSuffix.empty() && -1 == m_strTitle.Find( appNameSuffix.c_str() ) )
		{
			m_strTitle += appNameSuffix.c_str();		// append ' [NN-bit]' suffix
			cs.lpszName = m_strTitle.GetString();
		}
	}

	return __super::PreCreateWindow( cs );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseMainFrameWndEx, BaseFrameWndT, TBaseClass )
	ON_WM_INITMENUPOPUP()
	ON_COMMAND( ID_WINDOW_MANAGER, OnWindowManager )
	ON_REGISTERED_MESSAGE( AFX_WM_CREATETOOLBAR, OnToolbarCreateNew )
END_MESSAGE_MAP()

template< typename BaseFrameWndT >
void CBaseMainFrameWndEx<BaseFrameWndT>::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	if ( !isSysMenu )
	{
		ui::ReplaceMenuItemWithPopup( pPopupMenu, ID_APPLOOK_POPUP, IDR_STD_POPUPS_MENU, ui::AppLookPopup );
	}

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

template< typename BaseFrameWndT >
void CBaseMainFrameWndEx<BaseFrameWndT>::OnWindowManager( void )
{
	if ( CMDIFrameWndEx* pMdiFrameWnd = dynamic_cast<CMDIFrameWndEx*>( this ) )
		pMdiFrameWnd->ShowWindowsDialog();
}

template< typename BaseFrameWndT >
LRESULT CBaseMainFrameWndEx<BaseFrameWndT>::OnToolbarCreateNew( WPARAM wParam, LPARAM lParam )
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
