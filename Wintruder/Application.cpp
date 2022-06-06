
#include "stdafx.h"
#include "Application.h"
#include "AppService.h"
#include "MainDialog.h"
#include "resource.h"
#include "utl/UI/CmdInfoStore.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/UI/WndUtils.h"
#include "wnd/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


static const CImageStore::CCmdAlias cmdAliases[] =
{
	{ CM_REFRESH, ID_REFRESH },
	{ CM_MOVE_WINDOW_UP, ID_SHUTTLE_UP },
	{ CM_MOVE_WINDOW_DOWN, ID_SHUTTLE_DOWN },
	{ CM_MOVE_WINDOW_TO_TOP, ID_SHUTTLE_TOP },
	{ CM_MOVE_WINDOW_TO_BOTTOM, ID_SHUTTLE_BOTTOM },
	{ CM_COPY_FORMATTED, ID_EDIT_COPY }
};


namespace app
{
	void DrillDownDetail( DetailPage detailPage )
	{
		GetMainDialog()->DrillDownDetail( detailPage );
	}
}


CApplication theApp;


CApplication::CApplication( void )
	: CBaseApp<CWinApp>()
{
	// use AFX_IDS_APP_TITLE="Wintruder" - use same app registry key for 32/64 bit executables

	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
	if ( !__super::InitInstance() )
		return FALSE;

	if ( proc::IsProcessElevated() )
		GetLogger().Log( _T("* RUN ELEVATED (ADMIN) *") );

	app::GetOptions()->LoadAll();
	ui::CCmdInfoStore::m_autoPopDuration = 30000;			// 30 sec popup display time for MFC tooltips

	CAboutBox::s_appIconId = IDD_MAIN_DIALOG;
	m_sharedAccel.Load( IDR_MAIN_SHARED_ACCEL );

	GetSharedImageStore()->RegisterToolbarImages( IDR_IMAGE_STRIP );
	GetSharedImageStore()->RegisterAliases( ARRAY_PAIR( cmdAliases ) );

	CBaseMainDialog::ParseCommandLine( __argc, __targv );

	CMainDialog mainDialog;
	m_pMainWnd = &mainDialog;
	mainDialog.DoModal();
	m_pMainWnd = NULL;

	app::GetOptions()->SaveAll();
	return FALSE;
}

int CApplication::ExitInstance( void )
{
	__super::ExitInstance();
	return 0;		// exit code
}

void CApplication::ShowAppPopups( bool show )
{
	if ( HWND hMainWnd = AfxGetMainWnd()->GetSafeHwnd() )
	{
		::ShowWindow( hMainWnd, show ? SW_SHOW : SW_HIDE );
		::ShowOwnedPopups( hMainWnd, show );
	}
}

CToolTipCtrl* CApplication::GetMainTooltip( void )
{
	if ( NULL == m_mainTooltip.m_hWnd )
	{
		ASSERT_PTR( AfxGetMainWnd()->GetSafeHwnd() );

		VERIFY( m_mainTooltip.Create( AfxGetMainWnd(), TTS_ALWAYSTIP ) );
		int delayTime = m_mainTooltip.GetDelayTime( TTDT_INITIAL );
		m_mainTooltip.SetDelayTime( TTDT_INITIAL, delayTime );
		m_mainTooltip.SetDelayTime( TTDT_RESHOW, delayTime / 5 );
		m_mainTooltip.SetDelayTime( TTDT_AUTOPOP, 30000 );
		m_mainTooltip.Activate( TRUE );
	}

	return &m_mainTooltip;
}

bool CApplication::RelayTooltipEvent( MSG* pMsg )
{
	if ( !IsWindow( m_mainTooltip.m_hWnd ) )
		return false;

	switch ( pMsg->message )
	{
		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_NCLBUTTONUP:
		case WM_NCRBUTTONUP:
		case WM_NCMBUTTONUP:
			m_mainTooltip.Activate( TRUE );
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			m_mainTooltip.Activate( FALSE );
			return true;
		default:
			return false;
	}

	MSG relayMsg = *pMsg;
	::ScreenToClient( relayMsg.hwnd, &relayMsg.pt );
	m_mainTooltip.RelayEvent( &relayMsg );
	return true;
}

BOOL CApplication::PreTranslateMessage( MSG* pMsg )
{
	if ( HWND hMainDlg = AfxGetMainWnd()->GetSafeHwnd() )
		if ( pMsg->hwnd == hMainDlg || ::IsChild( hMainDlg, pMsg->hwnd ) )
		{
			if ( m_mainTooltip.m_hWnd != NULL )
				RelayTooltipEvent( pMsg );

			if ( m_sharedAccel.Translate( pMsg, hMainDlg ) )
				return TRUE;
		}

	return __super::PreTranslateMessage( pMsg );
}

BOOL CApplication::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( app::GetOptions()->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp<CWinApp> )
	ON_COMMAND( ID_APP_EXIT, OnAppExit )
	ON_COMMAND( CM_REFRESH, CmRefresh )
	ON_COMMAND( CM_REFRESH_BRANCH, CmRefreshBranch )
	ON_COMMAND( CM_ACTIVATE_WINDOW, CmActivateWindow )
	ON_UPDATE_COMMAND_UI( CM_ACTIVATE_WINDOW, OnUpdateActivateWindow )
	ON_COMMAND( CM_SHOW_WINDOW, CmShowWindow )
	ON_UPDATE_COMMAND_UI( CM_SHOW_WINDOW, OnUpdateShowWindow )
	ON_COMMAND( CM_HIDE_WINDOW, CmHideWindow )
	ON_UPDATE_COMMAND_UI( CM_HIDE_WINDOW, OnUpdateHideWindow )
	ON_COMMAND( CM_TOGGLE_TOPMOST_WINDOW, CmToggleTopmostWindow )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_TOPMOST_WINDOW, OnUpdateTopmostWindow )
	ON_COMMAND( CM_TOGGLE_ENABLE_WINDOW, CmToggleEnableWindow )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_ENABLE_WINDOW, OnUpdateEnableWindow )
	ON_COMMAND_RANGE( CM_MOVE_WINDOW_UP, CM_MOVE_WINDOW_TO_BOTTOM, CmMoveWindow )
	ON_UPDATE_COMMAND_UI_RANGE( CM_MOVE_WINDOW_UP, CM_MOVE_WINDOW_TO_BOTTOM, OnUpdateMoveWindow )
	ON_COMMAND( CM_REDRAW_DESKTOP, CmRedrawDesktop )
END_MESSAGE_MAP()

void CApplication::OnAppExit( void )
{
	AfxGetMainWnd()->SendMessage( WM_SYSCOMMAND, SC_CLOSE );
}

void CApplication::CmRefresh( void )
{
	app::GetSvc().PublishEvent( app::RefreshWndTree );
}

void CApplication::CmRefreshBranch( void )
{
	app::GetSvc().PublishEvent( app::RefreshBranch );
}

void CApplication::CmActivateWindow( void )
{
	wnd::Activate( app::GetTargetWnd() );
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateActivateWindow( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( app::IsValidTargetWnd() );
}

void CApplication::CmShowWindow( void )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	wnd::ShowWindow( targetWnd, SW_SHOWNA );
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateShowWindow( CCmdUI* pCmdUI )
{
	bool enable = app::IsValidTargetWnd();
	pCmdUI->Enable( enable );
	ui::SetRadio( pCmdUI, enable && ui::IsVisible( app::GetTargetWnd() ) );
}

void CApplication::CmHideWindow( void )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	CScopedAttachThreadInput scopedThreadAccess( targetWnd );

	::ShowWindow( targetWnd, SW_HIDE );
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateHideWindow( CCmdUI* pCmdUI )
{
	bool enable = app::IsValidTargetWnd();
	pCmdUI->Enable( enable && ui::IsVisible( app::GetTargetWnd() ) );
	ui::SetRadio( pCmdUI, enable && !ui::IsVisible( app::GetTargetWnd() ) );
}

void CApplication::CmToggleTopmostWindow( void )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	ui::SetTopMost( targetWnd, !ui::IsTopMost( targetWnd ) );

	app::GetSvc().PublishEvent( app::RefreshWndTree );		// top z-order has changed
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateTopmostWindow( CCmdUI* pCmdUI )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	bool enable = targetWnd.IsValid() && ( !ui::IsChild( targetWnd ) || ::GetParent( targetWnd ) == ::GetDesktopWindow() );
	pCmdUI->Enable( enable );
	pCmdUI->SetCheck( enable && ui::IsTopMost( app::GetTargetWnd() ) );
}

void CApplication::CmToggleEnableWindow( void )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	ui::EnableWindow( targetWnd, ui::IsDisabled( targetWnd ) );
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateEnableWindow( CCmdUI* pCmdUI )
{
	bool enable = false, isEnabled = false;
	const CWndSpot& targetWnd = app::GetTargetWnd();
	if ( targetWnd.IsValid() )
	{
		enable = true;
		isEnabled = !ui::IsDisabled( targetWnd );
	}

	pCmdUI->Enable( enable );
	//pCmdUI->SetCheck( enable && isEnabled );		/* just a toggle button */

	if ( pCmdUI->m_pMenu != NULL )
	{
		enum Part { Enable, Disable, Suffix };
		static std::vector< std::tstring > parts;
		if ( parts.empty() )
			str::Split( parts, ui::GetMenuItemText( *pCmdUI->m_pMenu, pCmdUI->m_nID ).c_str(), _T("|") );

		std::tstring itemText; itemText.reserve( 128 );
		itemText = parts[ isEnabled ? Disable : Enable ];
		itemText += parts[ Suffix ];
		pCmdUI->SetText( itemText.c_str() );
	}
}

void CApplication::CmMoveWindow( UINT cmdId )
{
	switch ( cmdId )
	{
		case CM_MOVE_WINDOW_UP:
			ui::BringWndUp( app::GetTargetWnd() );
			break;
		case CM_MOVE_WINDOW_TO_TOP:
			ui::BringWndToTop( app::GetTargetWnd() );
			break;
		case CM_MOVE_WINDOW_DOWN:
			ui::BringWndDown( app::GetTargetWnd() );
			break;
		case CM_MOVE_WINDOW_TO_BOTTOM:
			ui::BringWndToBottom( app::GetTargetWnd() );
			break;
		default:
			ASSERT( FALSE );
	}

	app::GetSvc().PublishEvent( app::RefreshSiblings /*app::RefreshWndTree*/ );		// z-order has changed
	app::GetSvc().PublishEvent( app::WndStateChanged );
}

void CApplication::OnUpdateMoveWindow( CCmdUI* pCmdUI )
{
	bool enable = false, atBound = false;

	const CWndSpot& targetWnd = app::GetTargetWnd();
	if ( targetWnd.IsValid() && !targetWnd.IsDesktopWnd() )
	{
		enable = true;
		switch ( pCmdUI->m_nID )
		{
			case CM_MOVE_WINDOW_UP:
			case CM_MOVE_WINDOW_TO_TOP:
				atBound = ::GetWindow( targetWnd, GW_HWNDFIRST ) == targetWnd.m_hWnd;
				break;
			case CM_MOVE_WINDOW_DOWN:
			case CM_MOVE_WINDOW_TO_BOTTOM:
				atBound = ::GetWindow( targetWnd, GW_HWNDLAST ) == targetWnd.m_hWnd;
				break;
			default:
				ASSERT( FALSE );
		}
	}

	pCmdUI->Enable( enable && !atBound );
	pCmdUI->SetCheck( enable && atBound );
}

void CApplication::CmRedrawDesktop( void )
{
	ui::RedrawDesktop();
}
