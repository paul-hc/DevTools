
#include "pch.h"
#include "TrayIcon.h"
#include "SystemTray.h"
#include "ISystemTrayCallback.h"
#include "ImageStore.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include "WindowDebug.h"
#include "utl/Algorithms.h"
#include "utl/StringUtilities.h"
#include <shellapi.h>		// Shell_NotifyIcon()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	UINT GetTooltipMaxLength( void ) { NOTIFYICONDATA niData; niData; return COUNT_OF( niData.szTip ); }
	bool IsValidBaloonTimeout( UINT timeoutSecs ) { return timeoutSecs >= 10 && timeoutSecs <= 30; }		// must be between 10 and 30 seconds
	bool IsValidBaloonInfoFlags( DWORD infoFlags ) { return NIIF_NONE == infoFlags || NIIF_INFO == infoFlags || NIIF_WARNING == infoFlags || NIIF_ERROR == infoFlags; }
}


// CTrayIcon implementation

const UINT CTrayIcon::s_tooltipMaxLength = hlp::GetTooltipMaxLength();

CTrayIcon::CTrayIcon( CSystemTray* pTray, UINT trayIconId, bool isMainIcon )
	: m_trayIconId( trayIconId )
	, m_pTray( pTray )
	, m_pOwnerCallback( m_pTray->GetOwnerCallback() )
	, m_createFlags( 0 )
	, m_isMainIcon( isMainIcon )
	, m_autoHide( false )
	, m_addPending( false )
	, m_visible( false )
	, m_tooltipVisible( false )
	, m_baloonVisible( false )
	, m_ignoreNextLDblClc( false )
	, m_autoHideFlags( 0 )
{
	utl::ZeroWinStruct( &m_niData );
}

CTrayIcon::~CTrayIcon()
{
}

bool CTrayIcon::Add( CWnd* pPopupWnd, HICON hIcon, const TCHAR* pIconTipText, bool visible )
{
	std::tstring iconTipText;

	if ( nullptr == hIcon )
		hIcon = CImageStoresSvc::Instance()->RetrieveIcon( CIconId( m_trayIconId ) )->GetSafeHandle();

	if ( pIconTipText != nullptr )
		iconTipText = pIconTipText;
	else
		iconTipText = str::Load( m_trayIconId );

	// setup the NOTIFYICONDATA structure
	m_niData.hWnd = pPopupWnd->GetSafeHwnd();										// this window will receieve tray notifications
	m_niData.uID = m_trayIconId;
	m_niData.hIcon = hIcon;
	m_niData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;				// NIF_SHOWTIP required with NOTIFYICON_VERSION_4 to display the standard tooltip
	m_niData.uCallbackMessage = CSystemTray::WM_TRAYICONNOTIFY;
	str::CopyTextToBuffer( m_niData.szTip, iconTipText.c_str(), s_tooltipMaxLength );	// tray only supports tooltip text of maximum s_tooltipMaxLength

	m_visible = visible;
	if ( !m_visible )
	{
		SetFlag( m_niData.uFlags, NIF_STATE );
		m_niData.dwState = m_niData.dwStateMask = NIS_HIDDEN;

	}

	m_autoHide = !m_visible;
	SetFlag( m_autoHideFlags, NIF_ICON | NIF_INFO, m_autoHide );	// auto-hide icon after showing the balloon, animation, etc

	m_createFlags = m_niData.uFlags;		// store in case we need to recreate in OnExplorerRestart
	return Notify_AddIcon();
}

bool CTrayIcon::Delete( void )
{
	StopAnimation();

	bool succeeded = Notify_DeleteIcon();

	delete this;
	return succeeded;
}

bool CTrayIcon::AddPending( void )
{
	if ( !m_addPending )
		return false;

	m_niData.uFlags = m_createFlags;	// reset the flags to what was used at creation

	if ( !Notify_AddIcon() )			// try and recreate the icon
		return false;					// it's STILL hidden, then have another go next time...

	m_addPending = false;
	return true;
}

bool CTrayIcon::NotifyTrayIcon( int notifyCode )
{
	return ::Shell_NotifyIcon( notifyCode, &m_niData ) != FALSE;
}

bool CTrayIcon::Notify_AddIcon( void )
{
	if ( !NotifyTrayIcon( NIM_ADD ) )
	{
		m_addPending = true;
		return false;
	}

	m_addPending = false;

	// each time we add an icon, we need to modify to shell version 4 (Vista+) behaviour for WM_TRAYICONNOTIFY notifications
	m_niData.uVersion = NOTIFYICON_VERSION_4;		// note: add NIF_SHOWTIP to indicate the standard tooltip should still be shown
	NotifyTrayIcon( NIM_SETVERSION );
	return true;
}

bool CTrayIcon::Notify_DeleteIcon( void )
{
	if ( !NotifyTrayIcon( NIM_DELETE ) )
		return false;

	m_niData.hIcon = nullptr;
	ClearFlag( m_niData.uFlags, NIF_ICON );
	return true;
}

bool CTrayIcon::Notify_ModifyState( DWORD stateFlag, bool on )
{
	::SetFlag( m_niData.uFlags, NIF_STATE );		// preserve the other existing flags so that we don't loose the tooltip, etc
	::SetFlag( m_niData.dwState, stateFlag, on );
	m_niData.dwStateMask = stateFlag;

	return NotifyTrayIcon( NIM_MODIFY );
}

void CTrayIcon::PostNotify( PrivateNotify notifyCode, const CPoint& screenPos )
{
	m_pTray->GetPopupWnd()->PostMessage(
		CSystemTray::WM_TRAYICONNOTIFY,
		MAKEWPARAM( screenPos.x, screenPos.y ),
		MAKELPARAM( notifyCode, m_trayIconId )
	);
}


// icon manipulation

bool CTrayIcon::SetIcon( HICON hIcon )
{
	SetFlag( m_niData.uFlags, NIF_ICON );
	m_niData.hIcon = hIcon;				// the shell tray will own a copy of the icon, unless NIS_SHAREDICON state is used

	return NotifyTrayIcon( NIM_MODIFY );
}

bool CTrayIcon::SetIcon( const CIconId& iconResId )
{
	const CIcon* pIcon = CImageStoresSvc::Instance()->RetrieveIcon( iconResId );

	return pIcon != nullptr && SetIcon( pIcon->GetHandle() );
}

bool CTrayIcon::SetVisible( bool visible /*= true*/ )
{
	if ( m_visible == visible )
		return false;		// no change

	if ( !Notify_ModifyState( NIS_HIDDEN, !visible ) )
		return false;

	m_visible = visible;
	return true;
}

void CTrayIcon::SetTrayFocus( void )
{	// added by Michael Dunn [Nov 1999]; Microsoft's Win 2K UI guidelines say you should do this after the user dismisses the icon's context menu.
	NotifyTrayIcon( NIM_SETFOCUS );		// sets the focus to the tray icon
}

bool CTrayIcon::SetTooltipText( const TCHAR* pIconTipText )
{
	SetFlag( m_niData.uFlags, NIF_TIP );
	str::CopyTextToBuffer( m_niData.szTip, pIconTipText, s_tooltipMaxLength );

	return NotifyTrayIcon( NIM_MODIFY );
}

bool CTrayIcon::ShowBalloonTip( const std::tstring& text, const TCHAR* pTitle /*= nullptr*/, app::MsgType msgType /*= app::Info*/, UINT timeoutSecs /*= 0*/ )
{
	//TRACE( _T("CTrayIcon::ShowBalloonTip( %s ) {...\n"), str::Clamp( text, 24 ).c_str() );

	bool hasText = !text.empty() || !str::IsEmpty( pTitle );

	if ( m_baloonVisible && hasText )			// overlapping balloon?
		if ( nullptr == m_pBaloonPending.get() )
		{	// delayed transaction: wait for the NIN_BALLOONHIDE notification
			m_pBaloonPending.reset( new CBaloonData( text, pTitle, msgType, timeoutSecs ) );
			DoHideBalloonTip();
			return true;
		}

	m_pAnimation.reset();						// stop animation without auto-hiding

	if ( hasText && !m_visible )				// on-demand: showing baloon on hidden icon?
		SetVisible();							// auto-show hidden icon to display baloon

	DWORD infoFlag = ToInfoFlag( msgType, &timeoutSecs );
	return DoShowBalloonTip( text, pTitle, infoFlag, timeoutSecs );
}

bool CTrayIcon::DoShowBalloonTip( const std::tstring& text, const TCHAR* pTitle, DWORD infoFlag, UINT timeoutSecs )
{
	ASSERT( hlp::IsValidBaloonInfoFlags( infoFlag ) );		// info icon must be valid
	ASSERT( hlp::IsValidBaloonTimeout( timeoutSecs ) );		// timeout must be between 10 and 30 seconds

	SetFlag( m_niData.uFlags, NIF_INFO );
	//ClearFlag( m_niData.uFlags, NIF_ICON );	// PHC: protect existing icon from vanishing - note: it no longer vanishes since the HICON is owned externally
	m_niData.dwInfoFlags = infoFlag;
	m_niData.uTimeout = timeoutSecs * 1000;		// convert time to ms

	str::CopyTextToBuffer( m_niData.szInfo, text.c_str(), BalloonTextMaxLength );
	str::CopyTextToBuffer( m_niData.szInfoTitle, pTitle, BalloonTitleMaxLength );

	bool success = NotifyTrayIcon( NIM_MODIFY );

	if ( text.empty() )							// hid the balloon?
		m_baloonVisible = false;				// proactively mark balloon as hidden, since for longer non-cooperative transactions NIN_BALLOONHIDE won't be received in time

	m_niData.szInfo[ 0 ] = _T('\0');			// zero-out the balloon text string so that later operations won't redisplay the balloon
	return success;
}

DWORD CTrayIcon::ToInfoFlag( app::MsgType msgType, UINT* pOutTimeoutSecs /*= nullptr*/ )
{
	DWORD infoFlag = NIIF_NONE;
	UINT timeoutSecs = 10;

	switch ( msgType )
	{
		case app::Error:
			infoFlag = NIIF_ERROR;
			timeoutSecs = 30;
			break;
		case app::Warning:
			infoFlag = NIIF_WARNING;
			timeoutSecs = 20;
			break;
		case app::Info:
			infoFlag = NIIF_INFO;
			timeoutSecs = 10;
			break;
	}

	if ( pOutTimeoutSecs != nullptr && 0 == *pOutTimeoutSecs )
		*pOutTimeoutSecs = timeoutSecs;

	return infoFlag;
}

bool CTrayIcon::AutoHideIcon( void )
{
	bool hidIcon = false;

	if ( m_autoHide && m_visible )		// must auto-hide icon?
	{
		if ( !IsAnimating() )			// prevent hiding the animating icon (due to balloon async notifications)
			hidIcon = SetVisible( false );

		m_baloonVisible = false;
	}

	if ( hidIcon )
		if ( CWnd* pOwnerWnd = m_pTray->GetOwnerWnd() )
			if ( CWnd::GetForegroundWindow() != pOwnerWnd )
				pOwnerWnd->SetForegroundWindow();

	return hidIcon;
}

std::tstring CTrayIcon::FormatState( void ) const
{
	std::tstring text = str::Format( _T("trayIconId=%d"), m_trayIconId );
	static const TCHAR s_sep[] = _T(" ");

	if ( m_isMainIcon )
		stream::Tag( text, _T("MainIcon"), s_sep );

	if ( m_autoHide )		// must auto-hide icon?
		stream::Tag( text, _T("auto-hide"), s_sep );

	if ( m_addPending )
		stream::Tag( text, _T("addPending"), s_sep );

	if ( !m_visible )
		stream::Tag( text, _T("hidden"), s_sep );

	if ( m_tooltipVisible )
		stream::Tag( text, _T("tooltipVisible"), s_sep );

	if ( m_baloonVisible )
		stream::Tag( text, _T("baloonVisible"), s_sep );

	return text;
}

void CTrayIcon::Animate( double durationSecs, UINT stepDelayMiliSecs /*= 100*/ )
{
	ASSERT_PTR( m_animImageList.GetSafeHandle() );

	if ( m_baloonVisible )
		HideBalloonTip();

	if ( !m_visible )			// on-demand: animating a hidden icon?  note: if hiding the balloon, the icon will be auto-shown later while animating
		SetVisible();			// auto-show hidden icon to display animation

	m_pAnimation.reset( new CTrayIcon::CAnimation( this, durationSecs, stepDelayMiliSecs ) );
}

bool CTrayIcon::StopAnimation( void )
{
	if ( !IsAnimating() )
		return false;

	m_pAnimation.reset();

	if ( m_autoHide )
		SetVisible( false );

	return true;
}

bool CTrayIcon::HandleTimer( UINT_PTR eventId )
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

bool CTrayIcon::HandleTrayIconNotify( UINT msgNotifyCode, const CPoint& screenPos )
{
	dbg::CScopedTrayIconDiagnostics scopedDiagnostics( this, msgNotifyCode );

	// event not handled by the owner: fallback to default implementation
	switch ( msgNotifyCode )
	{
		case WM_CONTEXTMENU:
			if ( m_isMainIcon && m_pOwnerCallback != nullptr )
				if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
					{
						pOwnerWnd->SetForegroundWindow();
						ui::TrackPopupMenu( *pContextMenu, pOwnerWnd, screenPos, TPM_RIGHTBUTTON );
						return true;
					}
					else
						pOwnerWnd->SendMessage( WM_CONTEXTMENU, (WPARAM)m_pTray->GetPopupWnd()->GetSafeHwnd(), MAKELPARAM( screenPos.x, screenPos.y ) );

			break;
		case WM_LBUTTONDBLCLK:
			if ( m_ignoreNextLDblClc )
				m_ignoreNextLDblClc = false;
			else if ( m_isMainIcon && m_pOwnerCallback != nullptr )
				if ( CMenu* pContextMenu = m_pOwnerCallback->GetTrayIconContextMenu() )
					if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					{
						UINT cmdId = pContextMenu->GetDefaultItem( GMDI_GOINTOPOPUPS );
						if ( cmdId != -1 )
						{
							pOwnerWnd->SetForegroundWindow();
							ui::SendCommand( pOwnerWnd->GetSafeHwnd(), cmdId, BN_CLICKED, m_pTray->GetPopupWnd()->GetSafeHwnd() );
							return true;
						}
					}

			break;
		case NIN_SELECT:
			if ( m_isMainIcon && m_pOwnerCallback != nullptr )
				if ( CWnd* pOwnerWnd = m_pOwnerCallback->GetOwnerWnd() )
					if ( CSystemTray::IsMinimizedToTray( pOwnerWnd ) )
					{	// Restore on L-Click
						pOwnerWnd->SetForegroundWindow();
						m_pTray->RestoreOwnerWnd();
						m_ignoreNextLDblClc = true;		// prevent minimizing again if WM_LBUTTONDBLCLK gets received next
						return true;
					}

			break;
		case NIN_POPUPOPEN:
			m_tooltipVisible = true;
			break;
		case NIN_POPUPCLOSE:
			if ( HasFlag( m_autoHideFlags, NIF_TIP ) )			// must auto-hide icon?
				SetVisible( false );

			m_tooltipVisible = false;
			break;
		case NIN_BALLOONSHOW:
			m_baloonVisible = true;
			break;
		case NIN_BALLOONHIDE:
			if ( m_pBaloonPending.get() != nullptr )
			{
				PostNotify( _NIN_BalloonPendingShow, screenPos );
				return true;
			}
			// fall-through
		case NIN_BALLOONTIMEOUT:
		case NIN_BALLOONUSERCLICK:
			if ( nullptr == m_pBaloonPending.get() )
				AutoHideIcon();
			break;
		case _NIN_BalloonPendingShow:
			if ( m_pBaloonPending.get() != nullptr )
			{
				m_pBaloonPending->ShowBalloon( this );
				m_pBaloonPending.reset();
			}
			else
				ASSERT( false );
			return true;
	}

	return false;		// event not handled
}


// CTrayIcon::CAnimation class

CTrayIcon::CAnimation::CAnimation( CTrayIcon* pTrayIcon, double durationSecs, UINT stepDelayMiliSecs )
	: m_pTrayIcon( pTrayIcon )
	, m_pWnd( m_pTrayIcon->GetTray()->GetPopupWnd() )
	, m_hIconOrig( m_pTrayIcon->GetIcon() )
	, m_durationSecs( durationSecs )
	, m_eventId( m_pWnd->SetTimer( AnimationEventId, stepDelayMiliSecs, nullptr ) )
	, m_imageCount( m_pTrayIcon->GetAnimImageList().GetImageCount() )
	, m_imagePos( 0 )
{
	ASSERT( m_eventId != 0 );
}

CTrayIcon::CAnimation::~CAnimation()
{
	ASSERT_PTR( m_pWnd->GetSafeHwnd() );

	if ( m_eventId != 0 )
		m_pWnd->KillTimer( m_eventId );

	if ( m_hIconOrig != nullptr )
		m_pTrayIcon->SetIcon( m_hIconOrig );
}

bool CTrayIcon::CAnimation::HandleTimerEvent( UINT_PTR eventId )
{
	if ( eventId != m_eventId )
		return false;

	ASSERT( m_imagePos < m_imageCount );

	CIcon icon( gdi::ExtractIcon( m_pTrayIcon->GetAnimImageList(), m_imagePos ) );

	m_pTrayIcon->SetIcon( icon.GetHandle() );
	++m_imagePos %= m_imageCount;
	return true;
}


// CScopedTrayIconBalloon implementation

CScopedTrayIconBalloon::CScopedTrayIconBalloon( CTrayIcon* pTrayIcon /*= nullptr*/, const std::tstring& text, const TCHAR* pTitle /*= nullptr*/,
												app::MsgType msgType /*= app::Info*/, UINT timeoutSecs /*= 30*/ )
	: m_pTrayIcon( pTrayIcon != nullptr ? pTrayIcon : CSystemTray::Instance()->FindMainIcon() )
{
	if ( m_pTrayIcon != nullptr )
		m_pTrayIcon->ShowBalloonTip( text, pTitle != nullptr ? pTitle : m_pTrayIcon->GetTooltipText().c_str(), msgType, timeoutSecs );
}

CScopedTrayIconBalloon::~CScopedTrayIconBalloon()
{
	if ( m_pTrayIcon != nullptr && m_pTrayIcon->IsBalloonTipVisible() )
		m_pTrayIcon->HideBalloonTip();
}
