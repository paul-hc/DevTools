
#include "stdafx.h"
#include "Options.h"
#include "AppService.h"
#include "Observers.h"
#include "wnd/WndUtils.h"
#include "utl/EnumTags.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace opt
{
	const CEnumTags& GetTags_FrameStyle( void )
	{
		static const CEnumTags tags( _T("Entire window|Non-client area|Frame") );
		return tags;
	}

	const CEnumTags& GetTags_AutoUpdateTarget( void )
	{
		static const CEnumTags tags( _T("Current|At Mouse|Foreground|Active|Focus|Topmost|Topmost Non-Child|Topmost Visible") );
		return tags;
	}

	const CEnumTags& GetTags_QueryWndIcons( void )
	{
		static const CEnumTags tags( _T("None (avoid UIPI deadlocks)|Top Windows|All Windows") );
		return tags;
	}
}


namespace reg
{
	static const TCHAR section_options[] = _T("Options");
}


// COptions implementation

COptions::COptions( CAppService* pAppSvc )
	: CCmdTarget()
	, m_pAppSvc( pAppSvc )
	, m_regOptions( reg::section_options )
	, m_hasUIPI( wnd::HasUIPI() )
	, m_keepTopmost( false )
	, m_hideOnTrack( false )
	, m_autoHighlight( true )
	, m_ignoreHidden( true )
	, m_ignoreDisabled( false )
	, m_displayZeroFlags( false )
	, m_frameStyle( opt::NonClient )
	, m_frameSize( ::GetSystemMetrics( SM_CXFIXEDFRAME ) )
	, m_queryWndIcons( m_hasUIPI ? opt::NoWndIcons : opt::AllWndIcons )
	, m_autoUpdate( false )
	, m_autoUpdateRefresh( true )
	, m_autoUpdateTimeout( 3 )
	, m_updateTarget( opt::CurrentWnd )
{
	ASSERT_PTR( m_pAppSvc );

	m_regOptions.AddOption( MAKE_OPTION( &m_keepTopmost ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_hideOnTrack ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_autoHighlight ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_ignoreHidden ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_ignoreDisabled ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_displayZeroFlags ) );
	m_regOptions.AddOption( MAKE_ENUM_OPTION( &m_frameStyle ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_frameSize ) );
	m_regOptions.AddOption( MAKE_ENUM_OPTION( &m_queryWndIcons ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_autoUpdate ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_autoUpdateRefresh ) );
	m_regOptions.AddOption( MAKE_OPTION( &m_autoUpdateTimeout ) );
	m_regOptions.AddOption( MAKE_ENUM_OPTION( &m_updateTarget ) );
}

void COptions::Load( void )
{
	m_regOptions.LoadOptions();

	if ( m_hasUIPI )
		ModifyOption( &m_autoUpdateRefresh, false );		// disable auto-refresh since UIPI can cause deadlocks or severe delays
}

void COptions::Save( void ) const
{
	m_regOptions.SaveOptions();
}

void COptions::PublishChangeEvent( void )
{
	m_pAppSvc->PublishEvent( app::OptionChanged );
}


// command handlers

BEGIN_MESSAGE_MAP( COptions, CCmdTarget )
	ON_COMMAND( ID_TOP_MOST_CHECK, OnToggle_KeepTopmost )
	ON_UPDATE_COMMAND_UI( ID_TOP_MOST_CHECK, OnUpdate_KeepTopmost )
	ON_COMMAND( ID_AUTO_HIDE_CHECK, OnToggle_AutoHideCheck )
	ON_UPDATE_COMMAND_UI( ID_AUTO_HIDE_CHECK, OnUpdate_AutoHideCheck )
	ON_COMMAND( ID_AUTO_HILIGHT_CHECK, OnToggle_AutoHilightCheck )
	ON_UPDATE_COMMAND_UI( ID_AUTO_HILIGHT_CHECK, OnUpdate_AutoHilightCheck )
	ON_COMMAND( ID_IGNORE_HIDDEN_CHECK, OnToggle_IgnoreHidden )
	ON_UPDATE_COMMAND_UI( ID_IGNORE_HIDDEN_CHECK, OnUpdate_IgnoreHidden )
	ON_COMMAND( ID_IGNORE_DISABLED_CHECK, OnToggle_IgnoreDisabled )
	ON_UPDATE_COMMAND_UI( ID_IGNORE_DISABLED_CHECK, OnUpdate_IgnoreDisabled )
	ON_COMMAND( ID_DISPLAY_ZERO_FLAGS_CHECK, OnToggle_DisplayZeroFlags )
	ON_UPDATE_COMMAND_UI( ID_DISPLAY_ZERO_FLAGS_CHECK, OnUpdate_DisplayZeroFlags )
	ON_COMMAND( ID_AUTO_UPDATE_CHECK, OnToggle_AutoUpdate )
	ON_UPDATE_COMMAND_UI( ID_AUTO_UPDATE_CHECK, OnUpdate_AutoUpdate )
	ON_COMMAND( ID_AUTO_UPDATE_REFRESH_CHECK, OnToggle_AutoUpdateRefresh )
	ON_UPDATE_COMMAND_UI( ID_AUTO_UPDATE_REFRESH_CHECK, OnUpdate_AutoUpdateRefresh )
END_MESSAGE_MAP()

void COptions::OnToggle_KeepTopmost( void )
{
	ToggleOption( &m_keepTopmost );
	ui::SetTopMost( AfxGetMainWnd()->GetSafeHwnd(), m_keepTopmost );
}

void COptions::OnUpdate_KeepTopmost( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_keepTopmost );
}

void COptions::OnToggle_AutoHideCheck( void )
{
	ToggleOption( &m_hideOnTrack );
}

void COptions::OnUpdate_AutoHideCheck( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_hideOnTrack );
}

void COptions::OnToggle_AutoHilightCheck( void )
{
	ToggleOption( &m_autoHighlight );
}

void COptions::OnUpdate_AutoHilightCheck( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_autoHighlight );
}

void COptions::OnToggle_IgnoreHidden( void )
{
	ToggleOption( &m_ignoreHidden );
}

void COptions::OnUpdate_IgnoreHidden( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_ignoreHidden );
}

void COptions::OnToggle_IgnoreDisabled( void )
{
	ToggleOption( &m_ignoreDisabled );
}

void COptions::OnUpdate_IgnoreDisabled( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_ignoreDisabled );
}

void COptions::OnToggle_DisplayZeroFlags( void )
{
	ToggleOption( &m_displayZeroFlags );
}

void COptions::OnUpdate_DisplayZeroFlags( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_displayZeroFlags );
}

void COptions::OnToggle_AutoUpdate( void )
{
	ToggleOption( &m_autoUpdate );

	m_pAppSvc->PublishEvent( app::ToggleAutoUpdate );		// main dialog will toggle the timer on/off
}

void COptions::OnUpdate_AutoUpdate( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_autoUpdate );
}

void COptions::OnToggle_AutoUpdateRefresh( void )
{
	ToggleOption( &m_autoUpdateRefresh );
}

void COptions::OnUpdate_AutoUpdateRefresh( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_hasUIPI );							// enabled for Windows 7-
	pCmdUI->SetCheck( m_autoUpdateRefresh );
}
