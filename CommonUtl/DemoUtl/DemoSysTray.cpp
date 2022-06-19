
#include "stdafx.h"
#include "DemoSysTray.h"
#include "Application.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Dialog_fwd.h"
#include "utl/UI/SystemTray.h"
#include "utl/UI/TrayIcon.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDemoSysTray::CDemoSysTray( CWnd* pOwner )
	: CCmdTarget()
	, m_pOwner( pOwner )
	, m_pSysTray( CSystemTray::Instance() )
	, m_pMsgIcon( m_pSysTray->FindIcon( IDR_MSG_TRAY ) )
	, m_pAppIcon( m_pSysTray->FindIcon( IDR_MAINFRAME ) )
{
	ASSERT_PTR( m_pSysTray );
	ASSERT_PTR( m_pMsgIcon );
	ASSERT_PTR( m_pAppIcon );
}

CDemoSysTray::~CDemoSysTray()
{
}

const CDemoSysTray::CMsg& CDemoSysTray::GetNextMessage( void )
{
	static const CMsg s_messages[] =
	{
		{ _T("Knuth is an organist and a composer."), app::Info },
		{ _T("Knuth received a scholarship in physics to the Case Institute of Technology (now part of Case Western Reserve University)\nin Cleveland, Ohio, enrolling in 1956."), app::Warning },
		{ _T("Just before publishing the first volume of The Art of Computer Programming, Knuth left Caltech to accept employment with the Institute for Defense Analyses' Communications Research Division"), app::Error }
	};
	static size_t s_pos;

	return s_messages[ s_pos++ % COUNT_OF( s_messages ) ];
}


// command handlers

BEGIN_MESSAGE_MAP( CDemoSysTray, CCmdTarget )
	ON_COMMAND( ID_MSG_TRAY_SHOW_NEXT_BALLOON, On_MsgTray_ShowNextBalloon )
	ON_UPDATE_COMMAND_UI( ID_MSG_TRAY_SHOW_NEXT_BALLOON, OnUpdate_MsgTray_ShowNextBalloon )
	ON_COMMAND( ID_MSG_TRAY_HIDE_BALLOON, On_MsgTray_HideBalloon )
	ON_UPDATE_COMMAND_UI( ID_MSG_TRAY_HIDE_BALLOON, OnUpdate_MsgTray_HideBalloon )
	ON_COMMAND( ID_MSG_TRAY_ANIMATE, On_MsgTray_Animate )
	ON_UPDATE_COMMAND_UI( ID_MSG_TRAY_ANIMATE, OnUpdate_MsgTray_Animate )

	ON_COMMAND( ID_APP_TRAY_SHOW_BALLOON, On_AppTray_ShowBalloon )
	ON_UPDATE_COMMAND_UI( ID_APP_TRAY_SHOW_BALLOON, OnUpdate_AppTray_ShowBalloon )
	ON_COMMAND( ID_APP_TRAY_TOGGLE_TRAY_ICON, On_AppTray_ToggleTrayIcon )
	ON_UPDATE_COMMAND_UI( ID_APP_TRAY_TOGGLE_TRAY_ICON, OnUpdate_AppTray_ToggleTrayIcon )
	ON_COMMAND( ID_APP_TRAY_FOCUS_TRAY_ICON, On_AppTray_FocusTrayIcon )
	ON_UPDATE_COMMAND_UI( ID_APP_TRAY_FOCUS_TRAY_ICON, OnUpdate_AppTray_FocusTrayIcon )
	ON_COMMAND( ID_APP_TRAY_ANIMATE, On_AppTray_Animate )
	ON_UPDATE_COMMAND_UI( ID_APP_TRAY_ANIMATE, OnUpdate_AppTray_Animate )
END_MESSAGE_MAP()

void CDemoSysTray::On_MsgTray_ShowNextBalloon( void )
{
	const CMsg& msg = GetNextMessage();
	m_pMsgIcon->ShowBalloonTip( msg.m_text, _T("Message Balloon"), msg.m_type );
}

void CDemoSysTray::OnUpdate_MsgTray_ShowNextBalloon( CCmdUI* pCmdUI )
{
	pCmdUI;
}

void CDemoSysTray::On_MsgTray_HideBalloon( void )
{
	m_pMsgIcon->HideBalloonTip();
}

void CDemoSysTray::OnUpdate_MsgTray_HideBalloon( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pMsgIcon->IsVisible() && m_pMsgIcon->IsBalloonTipVisible() );
}

void CDemoSysTray::On_MsgTray_Animate( void )
{
	if ( !m_pMsgIcon->IsAnimating() )
		m_pMsgIcon->Animate( 10.0 );
	else
		m_pMsgIcon->StopAnimation();
}

void CDemoSysTray::OnUpdate_MsgTray_Animate( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pMsgIcon->CanAnimate() );
	pCmdUI->SetCheck( m_pMsgIcon->IsAnimating() );
}

void CDemoSysTray::On_AppTray_ShowBalloon( void )
{
	const CMsg& msg = GetNextMessage();
	m_pAppIcon->ShowBalloonTip( msg.m_text, _T("Demo UTL Balloon"), msg.m_type );
}

void CDemoSysTray::OnUpdate_AppTray_ShowBalloon( CCmdUI* pCmdUI )
{
	pCmdUI;
}

void CDemoSysTray::On_AppTray_ToggleTrayIcon( void )
{
	m_pAppIcon->SetVisible( !m_pAppIcon->IsVisible() );
}

void CDemoSysTray::OnUpdate_AppTray_ToggleTrayIcon( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_pAppIcon->IsVisible() );
}

void CDemoSysTray::On_AppTray_FocusTrayIcon( void )
{
	m_pAppIcon->SetTrayFocus();
}

void CDemoSysTray::OnUpdate_AppTray_FocusTrayIcon( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pAppIcon->IsVisible() );
}

void CDemoSysTray::On_AppTray_Animate( void )
{
	if ( !m_pAppIcon->IsAnimating() )
		m_pAppIcon->Animate( 10.0 );
	else
		m_pAppIcon->StopAnimation();
}

void CDemoSysTray::OnUpdate_AppTray_Animate( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pAppIcon->CanAnimate() );
	pCmdUI->SetCheck( m_pAppIcon->IsAnimating() );
}
