
#include "stdafx.h"
#include "SystemTray.h"
#include "TrayIcon.h"
#include "ISystemTrayCallback.h"
#include "PopupDlgBase.h"
#include "WndUtils.h"
#include "WindowDebug.h"
#include "resource.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace sys_tray
{
	bool HasSystemTray( void )
	{
		return CSystemTray::Instance() != NULL;
	}

	bool ShowBalloonMessage( const std::tstring& text, const TCHAR* pTitle /*= NULL*/, app::MsgType msgType /*= app::Info*/, UINT timeoutSecs /*= 0*/ )
	{
		if ( CSystemTray* pSystemTray = CSystemTray::Instance() )
			if ( CTrayIcon* pMsgTrayIcon = pSystemTray->FindMessageIcon() )
				return pMsgTrayIcon->ShowBalloonTip( text, pTitle != NULL ? pTitle : pMsgTrayIcon->GetTooltipText().c_str(), msgType, timeoutSecs );

		return false;
	}

	bool HideBalloonMessage( void )
	{
		if ( CSystemTray* pSystemTray = CSystemTray::Instance() )
			if ( CTrayIcon* pMsgTrayIcon = pSystemTray->FindMessageIcon() )
				return pMsgTrayIcon->HideBalloonTip();

		return false;
	}
}


// CSystemTray implementation

CSystemTray* CSystemTray::s_pInstance = NULL;
const UINT CSystemTray::WM_TASKBARCREATED = ::RegisterWindowMessage( _T("TaskbarCreated") );
const UINT CSystemTray::WM_TRAYICONNOTIFY = ::RegisterWindowMessageA( "utl:WM_TRAYICONNOTIFY" );

CSystemTray::CSystemTray( void )
	: m_pOwnerCallback( NULL )
	, m_mainTrayIconId( 0 )
	, m_restoreShowCmd( SW_SHOWNORMAL )
{
	if ( NULL == s_pInstance )
		s_pInstance = this;
}

CSystemTray::~CSystemTray()
{
	if ( this == s_pInstance )
		s_pInstance = NULL;

	ASSERT( m_icons.empty() );		// should've been deleted in OnDestroy()
}

CWnd* CSystemTray::GetOwnerWnd( void ) const
{
	return m_pOwnerCallback != NULL ? m_pOwnerCallback->GetOwnerWnd() : NULL;
}

size_t CSystemTray::FindIconPos( UINT trayIconId ) const
{
	for ( size_t pos = 0; pos != m_icons.size(); ++pos )
		if ( trayIconId == m_icons[ pos ]->GetID() )
			return pos;

	return utl::npos;
}

CTrayIcon* CSystemTray::FindIcon( UINT trayIconId ) const
{
	size_t foundPos = FindIconPos( trayIconId );

	if ( utl::npos == foundPos )
		return NULL;

	return m_icons[ foundPos ];
}

CTrayIcon& CSystemTray::LookupIcon( UINT trayIconId ) const
{
	size_t foundPos = FindIconPos( trayIconId );
	ENSURE( foundPos != utl::npos );
	return *m_icons[ foundPos ];
}

CTrayIcon* CSystemTray::FindMessageIcon( void ) const
{
	CTrayIcon* pTrayIcon = FindIcon( IDR_APPLICATION );		// usually the auto-hide application icon

	if ( NULL == pTrayIcon )
		pTrayIcon = FindMainIcon();							// pick the main icon

	if ( NULL == pTrayIcon && !m_icons.empty() )
		pTrayIcon = m_icons.front();						// pick the first available tray icon

	return pTrayIcon;
}

CTrayIcon* CSystemTray::CreateTrayIcon( HICON hIcon, UINT trayIconId, const TCHAR* pIconTipText, bool visible /*= true*/ )
{
	ASSERT_NULL( FindIcon( trayIconId ) );

	CWnd* pPopupWnd = EnsurePopupWnd();
	ASSERT_PTR( pPopupWnd->GetSafeHwnd() );

	bool isMainIcon = visible && !utl::Any( m_icons, std::mem_fun( &CTrayIcon::IsMainIcon ) );
	CTrayIcon* pTrayIcon = new CTrayIcon( this, trayIconId, isMainIcon );

	if ( !pTrayIcon->Add( pPopupWnd, hIcon, pIconTipText, visible ) )
		TRACE( " ? CSystemTray::CreateTrayIcon( %d ) did not succeed (taskbar missing), will be installed delayed...\n", trayIconId );

	m_icons.push_back( pTrayIcon );

	if ( isMainIcon )
		m_mainTrayIconId = trayIconId;

	return pTrayIcon;
}

bool CSystemTray::DeleteTrayIcon( UINT trayIconId )
{
	size_t foundPos = FindIconPos( trayIconId );

	if ( utl::npos == foundPos )
	{
		ASSERT( false );
		return false;
	}

	return m_icons[ foundPos ]->Delete();
}

bool CSystemTray::IsMinimizedToTray( const CWnd* pOwnerWnd )
{
	ASSERT_PTR( pOwnerWnd );

	bool isOwnerWnd = CSystemTray::Instance() != NULL && CSystemTray::Instance()->GetOwnerWnd() == pOwnerWnd;
	bool isMinimized = isOwnerWnd && HasFlag( pOwnerWnd->m_nFlags, WF_EX_MinimizedToTray );

	// consistent with visibility state?
	// note: dialogs/property-sheets are hidden when processing WM_DESTROY, so we avoid checking for visibility consistency
	ENSURE( !isOwnerWnd || is_a<CPopupDlgBase>( pOwnerWnd ) || isMinimized == !ui::IsVisible( pOwnerWnd->GetSafeHwnd() ) );

	return isMinimized;
}

bool CSystemTray::IsRestoreToMaximized( const CWnd* pOwnerWnd )
{
	bool isOwnerWnd = CSystemTray::Instance() != NULL && CSystemTray::Instance()->GetOwnerWnd() == pOwnerWnd;
	bool restoreToMaximized = isOwnerWnd && HasFlag( pOwnerWnd->m_nFlags, WF_EX_RestoreToMaximized );

	ENSURE( !isOwnerWnd || restoreToMaximized == (SW_SHOWMAXIMIZED == CSystemTray::Instance()->m_restoreShowCmd) );		// consistent with tray show cmd?

	return restoreToMaximized;
}

void CSystemTray::MinimizeOwnerWnd( bool restoreToMaximized /*= false*/ )
{
	CWnd* pOwnerWnd = GetOwnerWnd();
	ASSERT_PTR( pOwnerWnd->GetSafeHwnd() );

	bool wasMaximized = restoreToMaximized || pOwnerWnd->IsZoomed() != FALSE;

	m_restoreShowCmd = wasMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;		// store show state to be restored to

	SetFlag( pOwnerWnd->m_nFlags, WF_EX_MinimizedToTray );
	SetFlag( pOwnerWnd->m_nFlags, WF_EX_RestoreToMaximized, wasMaximized );

	pOwnerWnd->ShowWindow( SW_HIDE );					// IMP: hide window post-minimize so that it vanishes from the taskbar into the system tray

	OnOwnerWndStatusChanged();
}

void CSystemTray::RestoreOwnerWnd( void )
{
	CWnd* pOwnerWnd = GetOwnerWnd();
	ASSERT_PTR( pOwnerWnd->GetSafeHwnd() );

	pOwnerWnd->ShowWindow( m_restoreShowCmd );			// activate and display the window: restore or maximize depending of its show state when minimized

	SetFlag( pOwnerWnd->m_nFlags, WF_EX_MinimizedToTray, false );
	SetFlag( pOwnerWnd->m_nFlags, WF_EX_RestoreToMaximized, false );
	m_restoreShowCmd = SW_SHOWNORMAL;

	OnOwnerWndStatusChanged();
}

void CSystemTray::OnOwnerWndStatusChanged( void )
{
	bool isMinimized = IsMinimizedToTray( GetOwnerWnd() );

	if ( m_pOwnerCallback != NULL )
		if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
			pContextMenu->SetDefaultItem( isMinimized
				? static_cast<UINT>( -1 )					// prevent Restore on double-click, since we restore on single L-click
				: ID_APP_MINIMIZE
			);

	if ( isMinimized && m_mainTrayIconId != 0 )
		LookupIcon( m_mainTrayIconId ).SetTrayFocus();		// focus the tray icon when minimized
}


// message handlers

void CSystemTray::HandleDestroy( void )
{
	utl::for_each( m_icons, std::mem_fun( &CTrayIcon::Delete ) );
	m_icons.clear();
}

bool CSystemTray::HandleSysCommand( UINT sysCmdId )
{
	if ( m_pOwnerCallback != NULL )
		switch ( sysCmdId )
		{
			case SC_MAXIMIZE:
				m_restoreShowCmd = SW_SHOWMAXIMIZED;
				// fall-through
			case SC_RESTORE:
				RestoreOwnerWnd();
				return true;
			case SC_MINIMIZE:
				MinimizeOwnerWnd();
				return true;
		}

	return false;
}

bool CSystemTray::HandleTimer( UINT_PTR eventId )
{
	for ( std::vector< CTrayIcon* >::const_iterator itIcon = m_icons.begin(); itIcon != m_icons.end(); ++itIcon )
		if ( (*itIcon)->HandleTimer( eventId ) )
			return true;

	return false;
}

void CSystemTray::HandleSettingChange( UINT flags )
{
	if ( SPI_SETWORKAREA == flags )
		utl::for_each( m_icons, std::mem_fun( &CTrayIcon::InstallPending ) );
}

void CSystemTray::HandleExplorerRestart( void )
{	// called whenever the taskbar is created (eg after explorer crashes and restarts).
	utl::for_each( m_icons, std::mem_fun( &CTrayIcon::InstallPending ) );
}

bool CSystemTray::HandleTrayIconNotify( WPARAM wParam, LPARAM lParam )
{
	UINT msgNotifyCode = LOWORD( lParam );		// e.g. WM_CONTEXTMENU, WM_LBUTTONDBLCLK, NIN_SELECT, NIN_BALLOONSHOW, NIN_POPUPOPEN, etc
	UINT trayIconId = HIWORD( lParam );
	CPoint screenPos( GET_X_LPARAM( wParam ), GET_Y_LPARAM( wParam ) );

	dbg::TraceTrayNotifyCode( msgNotifyCode );

	if ( m_pOwnerCallback != NULL )
		if ( m_pOwnerCallback->OnTrayIconNotify( msgNotifyCode, trayIconId, screenPos ) )
			return true;			// event handled by the owner (skip default handling)

	return LookupIcon( trayIconId ).HandleTrayIconNotify( msgNotifyCode, screenPos );
}


// CSystemTrayWnd implementation

CSystemTrayWnd::CSystemTrayWnd( void )
	: CWnd()
	, CSystemTray()
{
}

CSystemTrayWnd::~CSystemTrayWnd()
{
	if ( m_hWnd != NULL )
		DestroyWindow();
}

CWnd* CSystemTrayWnd::EnsurePopupWnd( void )
{
	// create once an invisible top-level popup window
	if ( NULL == m_hWnd )
		if ( !CreateEx( 0, AfxRegisterWndClass( 0 ), _T("<HiddenTrayIconPopup>"), WS_POPUP, 0, 0, 0, 0, NULL, 0 ) )
			ASSERT( false );

	return this;
}


// message map

BEGIN_MESSAGE_MAP( CSystemTrayWnd, CWnd )
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SETTINGCHANGE()
	ON_REGISTERED_MESSAGE( CSystemTray::WM_TASKBARCREATED, OnExplorerRestart )
	ON_REGISTERED_MESSAGE( CSystemTray::WM_TRAYICONNOTIFY, OnTrayIconNotify )
END_MESSAGE_MAP()

void CSystemTrayWnd::OnDestroy( void )
{
	HandleDestroy();
	__super::OnDestroy();
}

void CSystemTrayWnd::OnTimer( UINT_PTR eventId )
{
	if ( !HandleTimer( eventId ) )
		__super::OnTimer( eventId );
}

void CSystemTrayWnd::OnSettingChange( UINT flags, LPCTSTR pSection )
{
	__super::OnSettingChange( flags, pSection );
	HandleSettingChange( flags );
}

LRESULT CSystemTrayWnd::OnExplorerRestart( WPARAM wParam, LPARAM lParam )
{
	// This is called whenever the taskbar is created (e.g. after explorer crashes and restarts).
	// WM_TASKBARCREATED message is only passed to TOP LEVEL windows.
	wParam, lParam;
	HandleExplorerRestart();
	return Default();
}

LRESULT CSystemTrayWnd::OnTrayIconNotify( WPARAM wParam, LPARAM lParam )
{
	return HandleTrayIconNotify( wParam, lParam ) ? 1L : 0L;
}


// CSystemTrayWndHook implementation

CSystemTrayWndHook::CSystemTrayWndHook( bool autoDelete /*= false*/ )
	: CWindowHook( autoDelete )
	, CSystemTray()
{
}

CSystemTrayWndHook::~CSystemTrayWndHook()
{
}

CWnd* CSystemTrayWndHook::GetPopupWnd( void )
{
	ASSERT_PTR( GetOwnerCallback() );
	return GetOwnerCallback()->GetOwnerWnd();
}

CWnd* CSystemTrayWndHook::EnsurePopupWnd( void )
{	// hooks the popup window
	CWnd* pPopupWnd = GetPopupWnd();

	if ( !IsHooked() )
		HookWindow( pPopupWnd->GetSafeHwnd() );

	return pPopupWnd;
}

LRESULT CSystemTrayWndHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
{
	switch ( message )
	{
		case WM_DESTROY:
			HandleDestroy();
			break;
		case WM_SYSCOMMAND:
			if ( HandleSysCommand( GET_SC_WPARAM( wParam ) ) )		// sysCmdId
				return 0L;
			break;
		case WM_TIMER:
			if ( HandleTimer( static_cast<UINT>( wParam ) ) )
				return 0L;
			break;
		case WM_SETTINGCHANGE:
			HandleSettingChange( static_cast<UINT>( wParam ) );
			break;
		default:
			if ( CSystemTray::WM_TASKBARCREATED == message )
				HandleExplorerRestart();
			else if ( CSystemTray::WM_TRAYICONNOTIFY == message )
				if ( HandleTrayIconNotify( wParam, lParam ) )
					return 1L;
	}

	return __super::WindowProc( message, wParam, lParam );
}
