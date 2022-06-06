
#include "stdafx.h"
#include "SystemTray.h"
#include "ISystemTrayCallback.h"
#include "WndUtils.h"
#include "MenuUtilities.h"
#include "WindowDebug.h"
#include "utl/ContainerUtilities.h"
#include "resource.h"
#include <shellapi.h>		// Shell_NotifyIcon()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	void CopyTextToBuffer( TCHAR* pDestBuffer, const TCHAR* pText, size_t bufferSize )
	{
		if ( pText != NULL )
			_tcsncpy( pDestBuffer, pText, bufferSize - 1 );
		else
			pDestBuffer[ 0 ] = _T('\0');
	}

	UINT GetTooltipMaxLength( void ) { NOTIFYICONDATA niData; return COUNT_OF( niData.szTip ); }
	bool IsValidBaloonTimeout( UINT timeoutSecs ) { return timeoutSecs >= 10 && timeoutSecs <= 30; }		// must be between 10 and 30 seconds
	bool IsValidBaloonInfoFlags( DWORD infoFlags ) { return NIIF_NONE == infoFlags || NIIF_INFO == infoFlags || NIIF_WARNING == infoFlags || NIIF_ERROR == infoFlags; }
}


// CTrayIconAnimation class

class CTrayIconAnimation
{
public:
	CTrayIconAnimation( CSystemTray* pSystemTray, double durationSecs, UINT stepDelayMiliSecs )
		: m_pSystemTray( pSystemTray )
		, m_pWnd( m_pSystemTray->GetPopupWnd() )
		, m_hIconOrig( m_pSystemTray->GetIcon() )
		, m_durationSecs( durationSecs )
		, m_eventId( m_pWnd->SetTimer( AnimationEventId, stepDelayMiliSecs, NULL ) )
		, m_imageCount( m_pSystemTray->GetAnimImageList().GetImageCount() )
		, m_imagePos( 0 )
	{
		ASSERT( m_eventId != 0 );
	}

	~CTrayIconAnimation()
	{
		ASSERT_PTR( m_pWnd->GetSafeHwnd() );
		if ( m_eventId != 0 )
			m_pWnd->KillTimer( m_eventId );

		if ( m_hIconOrig != NULL )
			m_pSystemTray->SetIcon( m_hIconOrig );
	}

	bool HandleTimerEvent( UINT_PTR eventId )
	{
		if ( eventId != m_eventId )
			return false;

		ASSERT( m_imagePos < m_imageCount );

		HICON hIcon = m_pSystemTray->GetAnimImageList().ExtractIcon( static_cast<int>( m_imagePos ) );

		m_pSystemTray->SetIcon( hIcon );
		::DestroyIcon( hIcon );
		++m_imagePos %= m_imageCount;
		return true;
	}

	bool IsTimeout( void ) const { return m_timer.ElapsedSeconds() > m_durationSecs; }
private:
	enum { AnimationEventId = 4567 };

	CSystemTray* m_pSystemTray;
	CWnd* m_pWnd;
	const HICON m_hIconOrig;
	const double m_durationSecs;
	const UINT_PTR m_eventId;
	const size_t m_imageCount;
	size_t m_imagePos;
	CTimer m_timer;
};


// CSystemTray implementation

CSystemTray* CSystemTray::s_pInstance = NULL;
const UINT CSystemTray::s_tooltipMaxLength = hlp::GetTooltipMaxLength();
const UINT CSystemTray::WM_TASKBARCREATED = ::RegisterWindowMessage( _T("TaskbarCreated") );
const UINT CSystemTray::WM_TRAYICONNOTIFY = ::RegisterWindowMessageA( "utl:WM_TRAYICONNOTIFY" );

CSystemTray::CSystemTray( void )
	: m_pOwnerCallback( NULL )
	, m_iconAdded( false )
	, m_iconHidden( false )
	, m_showIconPending( false )
	, m_tooltipVisible( false )
	, m_baloonVisible( false )
	, m_ignoreNextLDblClc( false )
	, m_autoHideFlags( 0 )
	, m_mainTrayIconId( 0 )
	, m_mainFlags( 0 )
{
	utl::ZeroWinStruct( &m_niData );

	if ( NULL == s_pInstance )
		s_pInstance = this;
}

CSystemTray::~CSystemTray()
{
	if ( this == s_pInstance )
		s_pInstance = NULL;

	ASSERT( !m_iconAdded );		// should've been removed in OnDestroy()
	ASSERT( !IsAnimating() );
}

bool CSystemTray::CreateTrayIcon( HICON hIcon, UINT trayIconId, const TCHAR* pIconTipText, bool iconHidden /*= false*/ )
{
	CWnd* pPopupWnd = EnsurePopupWnd();
	ASSERT_PTR( pPopupWnd->GetSafeHwnd() );

	// setup the NOTIFYICONDATA structure
	m_niData.hWnd = pPopupWnd->GetSafeHwnd();										// this window will receieve tray notifications
	m_niData.uID = trayIconId;
	m_niData.hIcon = hIcon;
	m_niData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;				// NIF_SHOWTIP required with NOTIFYICON_VERSION_4 to display the standard tooltip
	m_niData.uCallbackMessage = WM_TRAYICONNOTIFY;
	hlp::CopyTextToBuffer( m_niData.szTip, pIconTipText, s_tooltipMaxLength );		// tray only supports tooltip text of maximum s_tooltipMaxLength

	m_iconHidden = iconHidden;

	if ( m_iconHidden )
	{
		SetFlag( m_niData.uFlags, NIF_STATE );
		m_niData.dwState = NIS_HIDDEN;
		m_niData.dwStateMask = NIS_HIDDEN;
	}

	if ( 0 == m_mainTrayIconId )
	{	// store main icon data in case we need to recreate in OnExplorerRestart
		m_mainTrayIconId = trayIconId;
		m_mainFlags = m_niData.uFlags;
	}

	bool success = Notify_AddIcon();
	m_showIconPending = !success;
	return success;
}

bool CSystemTray::CreateTrayIcon( UINT trayIconResId, const TCHAR* pIconTipText, bool iconHidden /*= false*/ )
{
	HICON hIcon = AfxGetApp()->LoadIcon( trayIconResId );

	if ( NULL == hIcon )
		hIcon = ::LoadIcon( NULL, IDI_ERROR );

	bool result = CreateTrayIcon( hIcon, trayIconResId, pIconTipText, iconHidden );
	::DestroyIcon( hIcon );
	return result;
}

bool CSystemTray::NotifyTrayIcon( int notifyCode )
{
	return ::Shell_NotifyIcon( notifyCode, &m_niData ) != FALSE;
}

bool CSystemTray::Notify_AddIcon( void )
{
	if ( !NotifyTrayIcon( NIM_ADD ) )
		return false;

	m_iconAdded = true;

	// each time we add an icon, we need to modify to shell version 4 (Vista+) behaviour for WM_TRAYICONNOTIFY notifications
	m_niData.uVersion = NOTIFYICON_VERSION_4;		// note: add NIF_SHOWTIP to indicate the standard tooltip should still be shown
	NotifyTrayIcon( NIM_SETVERSION );
	return true;
}

bool CSystemTray::Notify_DeleteIcon( void )
{
	if ( !NotifyTrayIcon( NIM_DELETE ) )
		return false;

	m_iconAdded = m_iconHidden = false;
	return true;
}

bool CSystemTray::Notify_ModifyState( DWORD stateFlag, bool on )
{
	::SetFlag( m_niData.uFlags, NIF_STATE );		// preserve the other existing flags so that we don't loose the tooltip, etc
	m_niData.dwState = 0;
	m_niData.dwStateMask = stateFlag;

	::SetFlag( m_niData.dwState, stateFlag, on );

	return NotifyTrayIcon( NIM_MODIFY );
}

bool CSystemTray::IsMinimizedToTray( CWnd* pPopupWnd )
{
	ASSERT_PTR( pPopupWnd->GetSafeHwnd() );
	return !ui::IsVisible( pPopupWnd->GetSafeHwnd() );
}

void CSystemTray::MinimizeToTray( CWnd* pPopupWnd )
{
	ASSERT_PTR( pPopupWnd->GetSafeHwnd() );
	pPopupWnd->ShowWindow( SW_HIDE );				// IMP: hide window post-minimize so that it vanishes from the taskbar into the system tray
}

void CSystemTray::RestoreFromTray( CWnd* pPopupWnd )
{
	ASSERT_PTR( pPopupWnd->GetSafeHwnd() );
	pPopupWnd->ShowWindow( SW_NORMAL );		// activate and display the window
}

void CSystemTray::MinimizePopupWnd( void )
{
	ASSERT_PTR( m_pOwnerCallback );
	CSystemTray::MinimizeToTray( GetPopupWnd() );

	if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
		pContextMenu->SetDefaultItem( (UINT)-1 );		// prevent Restore on double-click, since we restore on single L-click
}

void CSystemTray::RestorePopupWnd( void )
{
	ASSERT_PTR( m_pOwnerCallback );
	CSystemTray::RestoreFromTray( GetPopupWnd() );

	if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
		pContextMenu->SetDefaultItem( ID_APP_MINIMIZE );
}

bool CSystemTray::SetTooltipText( const TCHAR* pIconTipText )
{
	SetFlag( m_niData.uFlags, NIF_TIP );
	hlp::CopyTextToBuffer( m_niData.szTip, pIconTipText, s_tooltipMaxLength );

	return NotifyTrayIcon( NIM_MODIFY );
}

bool CSystemTray::ShowBalloonTip( const TCHAR text[], const TCHAR* pTitle /*= NULL*/, DWORD infoFlag /*= NIIF_NONE*/, UINT timeoutSecs /*= 10*/ )
{
	ASSERT( hlp::IsValidBaloonInfoFlags( infoFlag ) );		// info icon must be valid
	ASSERT( hlp::IsValidBaloonTimeout( timeoutSecs ) );		// timeout must be between 10 and 30 seconds
	ASSERT( HasIcon() );

	bool autoShowIcon = !str::IsEmpty( text ) && !IsIconVisible();		// showing baloon on hidden icon?

	if ( autoShowIcon )
		SetIconVisible();						// auto-show icon to display baloon

	SetFlag( m_niData.uFlags, NIF_INFO );
	m_niData.dwInfoFlags = infoFlag;
	m_niData.uTimeout = timeoutSecs * 1000;		// convert time to ms

	hlp::CopyTextToBuffer( m_niData.szInfo, text, BalloonTextMaxLength );
	hlp::CopyTextToBuffer( m_niData.szInfoTitle, pTitle, BalloonTitleMaxLength );

	bool success = NotifyTrayIcon( NIM_MODIFY );

	m_niData.szInfo[ 0 ] = _T('\0');			// zero-out the balloon text string so that later operations won't redisplay the balloon

	if ( success && autoShowIcon )
		SetFlag( m_autoHideFlags, NIF_INFO );		// store the balloon flag to auto-hide icon

	return success;
}


// icon manipulation

bool CSystemTray::SetIcon( HICON hIcon )
{
	SetFlag( m_niData.uFlags, NIF_ICON );
	m_niData.hIcon = hIcon;				// the shell tray will own a copy of the icon, unless NIS_SHAREDICON state is used

	return NotifyTrayIcon( NIM_MODIFY );
}

bool CSystemTray::LoadResIcon( UINT iconResId )
{
	if ( HICON hIcon = (HICON)::LoadImage( AfxGetResourceHandle(), MAKEINTRESOURCE( iconResId ), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR ) )
	{
		bool success = SetIcon( hIcon );
		::DestroyIcon( hIcon );

		return success;
	}

	return false;
}

bool CSystemTray::AddIcon( void )
{
	if ( m_iconAdded )
		RemoveIcon();

	SetFlag( m_niData.uFlags, NIF_ICON );

	if ( Notify_AddIcon() )
		m_iconHidden = m_showIconPending = false;
	else
		m_showIconPending = true;

	return m_iconAdded;
}

bool CSystemTray::RemoveIcon( void )
{
	m_showIconPending = false;

	if ( m_iconAdded )
	{
		SetFlag( m_niData.uFlags, NIF_ICON, false );
		return Notify_DeleteIcon();
	}

	return false;
}

bool CSystemTray::SetIconVisible( bool visible /*= true*/ )
{
	ASSERT( m_iconAdded );

	if ( IsIconVisible() == visible )
		return false;		// no change

	if ( visible )
	{
		if ( !m_iconAdded )
			return AddIcon();

		if ( m_iconHidden )
			if ( Notify_ModifyState( NIS_HIDDEN, false ) )
			{
				m_iconHidden = false;
				return true;
			}
	}
	else
	{
		if ( !m_iconHidden )
			if ( Notify_ModifyState( NIS_HIDDEN, true ) )
			{
				m_iconHidden = true;
				return true;
			}
	}

	return false;
}

void CSystemTray::SetTrayFocus( void )
{
	// added by Michael Dunn [Nov 1999]
	// Sets the focus to the tray icon.
	// Microsoft's Win 2K UI guidelines say you should do this after the user dismisses the icon's context menu.
	NotifyTrayIcon( NIM_SETFOCUS );
}

void CSystemTray::InstallIconPending( void )
{
	// is the icon display pending, and it's not been set as "hidden"?
	if ( m_showIconPending && !m_iconHidden )
	{
		m_niData.uFlags = m_mainFlags;		// reset the flags to what was used at creation

		if ( Notify_AddIcon() )					// try and recreate the icon
			m_showIconPending = false;

		// if it's STILL hidden, then have another go next time...
	}
}


void CSystemTray::Animate( UINT stepDelayMiliSecs, double durationSecs )
{
	ASSERT_PTR( m_animImageList.GetSafeHandle() );
	m_pAnimation.reset( new CTrayIconAnimation( this, durationSecs, stepDelayMiliSecs ) );
}

bool CSystemTray::StopAnimation( void )
{
	if ( !IsAnimating() )
		return false;

	m_pAnimation.reset();
	return true;
}

// message handlers

void CSystemTray::HandleDestroy( void )
{
	StopAnimation();
	RemoveIcon();
}

bool CSystemTray::HandleTimer( UINT_PTR eventId )
{
	if ( IsAnimating() )
		if ( m_pAnimation->HandleTimerEvent( eventId ) )
		{
			if ( m_pAnimation->IsTimeout() )
				StopAnimation();

			return true;
		}

	return false;
}

void CSystemTray::HandleSettingChange( UINT flags )
{
	if ( SPI_SETWORKAREA == flags )
		if ( IsIconVisible() )
			InstallIconPending( true );
}

void CSystemTray::HandleExplorerRestart( void )
{
	// called whenever the taskbar is created (eg after explorer crashes and restarts).
	InstallIconPending( true );
}

bool CSystemTray::HandleTrayIconNotify( WPARAM wParam, LPARAM lParam )
{
	UINT msgNotifyCode = LOWORD( lParam );		// e.g. WM_CONTEXTMENU, WM_LBUTTONDBLCLK, NIN_SELECT, NIN_BALLOONSHOW, NIN_POPUPOPEN, etc
	UINT iconId = HIWORD( lParam );
	CPoint screenPos( GET_X_LPARAM( wParam ), GET_Y_LPARAM( wParam ) );

	//dbg::TraceTrayNotifyCode( msgNotifyCode );

	if ( m_pOwnerCallback != NULL )
		if ( m_pOwnerCallback->OnTrayIconNotify( msgNotifyCode, iconId, screenPos ) )
			return true;			// event handled by the owner (skip default handling)

	// event not handled by the owner: fallback to default implementation
	switch ( msgNotifyCode )
	{
		case WM_CONTEXTMENU:
			if ( m_mainTrayIconId == iconId && m_pOwnerCallback != NULL )
				if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
					{
						pOwnerWnd->SetForegroundWindow();
						ui::TrackPopupMenu( *pContextMenu, pOwnerWnd, screenPos, TPM_RIGHTBUTTON );
						return true;
					}
					else
						pOwnerWnd->SendMessage( WM_CONTEXTMENU, (WPARAM)GetPopupWnd()->GetSafeHwnd(), MAKELPARAM( screenPos.x, screenPos.y ) );

			break;
		case WM_LBUTTONDBLCLK:
			if ( m_ignoreNextLDblClc )
				m_ignoreNextLDblClc = false;
			else if ( m_mainTrayIconId == iconId && m_pOwnerCallback != NULL )
				if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
					if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					{
						UINT cmdId = pContextMenu->GetDefaultItem( GMDI_GOINTOPOPUPS );
						if ( cmdId != -1 )
						{
							pOwnerWnd->SetForegroundWindow();
							ui::SendCommand( pOwnerWnd->GetSafeHwnd(), cmdId, BN_CLICKED, GetPopupWnd()->GetSafeHwnd() );
							return true;
						}
					}

			break;
		case NIN_SELECT:
			if ( m_mainTrayIconId == iconId && m_pOwnerCallback != NULL )
				if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					if ( IsMinimizedToTray( pOwnerWnd ) )
					{	// Restore on L-Click
						pOwnerWnd->SetForegroundWindow();
						RestorePopupWnd();
						m_ignoreNextLDblClc = true;		// prevent minimizing again if WM_LBUTTONDBLCLK gets received next
						return true;
					}

			break;
		case NIN_POPUPOPEN:
			m_tooltipVisible = true;
			break;
		case NIN_POPUPCLOSE:
			if ( HasFlag( m_autoHideFlags, NIF_TIP ) )		// must auto-hide icon?
				SetIconVisible( false );

			m_tooltipVisible = false;
			break;
		case NIN_BALLOONSHOW:
			m_baloonVisible = true;
			break;
		case NIN_BALLOONHIDE:
		case NIN_BALLOONTIMEOUT:
			if ( HasFlag( m_autoHideFlags, NIF_INFO ) )		// must auto-hide icon?
				SetIconVisible( false );

			m_baloonVisible = false;
			break;
	}

	return false;		// event not handled
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
		if ( !CreateEx( 0, AfxRegisterWndClass( 0 ), _T("<TrayIconPopup>"), WS_POPUP, 0, 0, 0, 0, NULL, 0 ) )
			ASSERT( false );

	return this;
}

bool CSystemTrayWnd::SetNotifyWnd( CWnd* pNotifyWnd )
{
	ASSERT( ::IsWindow( pNotifyWnd->GetSafeHwnd() ) );

	RefIconData().hWnd = pNotifyWnd->GetSafeHwnd();
	//RefIconData().uFlags = 0;

	return NotifyTrayIcon( NIM_MODIFY );
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
	// This is called whenever the taskbar is created (eg after explorer crashes and restarts).
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
		case WM_TIMER:
			if ( HandleTimer( static_cast<UINT>( wParam ) ) )
				return 0;
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


// CSystemTraySingleton implementation

UINT CSystemTraySingleton::s_appIconId = 1;
std::tstring CSystemTraySingleton::s_appTipText;

CSystemTraySingleton::CSystemTraySingleton( CWnd* pMainWnd, UINT appIconId /*= s_appIconId*/, const TCHAR* pAppTipText /*= s_appTipText.c_str()*/ )
	: CSystemTrayWndHook( true )
	, m_pMainWnd( pMainWnd )
{
	Construct( appIconId, pAppTipText );
}

void CSystemTraySingleton::Construct( UINT appIconId, const TCHAR* pAppTipText )
{
	ASSERT( Instance() == this );
	ASSERT_PTR( m_pMainWnd->GetSafeHwnd() );			// must be created to use

	SetOwnerCallback( this );
	HookWindow( m_pMainWnd->GetSafeHwnd() );
	CreateTrayIcon( appIconId, pAppTipText, true );		// create a hidden tray app icon
}

void CSystemTraySingleton::StoreAppInfo( UINT appIconId, const std::tstring& appTipText )
{
	s_appIconId = appIconId;
	s_appTipText = appTipText;
}

bool CSystemTraySingleton::OnTrayIconNotify( UINT msgNotifyCode, UINT iconId, const CPoint& screenPos )
{
	msgNotifyCode, iconId, screenPos;
	return false;
}
