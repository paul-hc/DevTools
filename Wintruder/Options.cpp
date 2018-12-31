
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
	: CRegistryOptions( reg::section_options, CRegistryOptions::SaveOnModify )
	, m_pAppSvc( pAppSvc )
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
	, m_autoUpdateRefresh( !m_hasUIPI )		// turn off auto-refresh since UIPI can cause deadlocks or severe delays
	, m_autoUpdateTimeout( 3 )
	, m_updateTarget( opt::CurrentWnd )
{
	ASSERT_PTR( m_pAppSvc );

	AddOption( MAKE_OPTION( &m_keepTopmost ), ID_TOP_MOST_CHECK );
	AddOption( MAKE_OPTION( &m_hideOnTrack ), ID_AUTO_HIDE_CHECK );
	AddOption( MAKE_OPTION( &m_autoHighlight ), ID_AUTO_HILIGHT_CHECK );
	AddOption( MAKE_OPTION( &m_ignoreHidden ), ID_IGNORE_HIDDEN_CHECK );
	AddOption( MAKE_OPTION( &m_ignoreDisabled ), ID_IGNORE_DISABLED_CHECK );
	AddOption( MAKE_OPTION( &m_displayZeroFlags ), ID_DISPLAY_ZERO_FLAGS_CHECK );
	AddOption( MAKE_ENUM_OPTION( &m_frameStyle ) );
	AddOption( MAKE_OPTION( &m_frameSize ) );
	AddOption( MAKE_ENUM_OPTION( &m_queryWndIcons ) );
	AddOption( MAKE_OPTION( &m_autoUpdate ), ID_AUTO_UPDATE_CHECK );
	AddOption( MAKE_OPTION( &m_autoUpdateRefresh ), ID_AUTO_UPDATE_REFRESH_CHECK );
	AddOption( MAKE_OPTION( &m_autoUpdateTimeout ) );
	AddOption( MAKE_ENUM_OPTION( &m_updateTarget ) );
}

void COptions::LoadAll( void )
{
	__super::LoadAll();

	if ( m_hasUIPI )
		ModifyOption( &m_autoUpdateRefresh, false );	// turn off auto-refresh since UIPI can cause deadlocks or severe delays
}

void COptions::PublishChangeEvent( void )
{
	m_pAppSvc->PublishEvent( app::OptionChanged );
}

void COptions::OnOptionChanged( const void* pDataMember )
{
	__super::OnOptionChanged( pDataMember );

	PublishChangeEvent();

	if ( pDataMember == &m_keepTopmost )
		ui::SetTopMost( AfxGetMainWnd()->GetSafeHwnd(), m_keepTopmost );
	else if ( pDataMember == &m_autoUpdate )
		m_pAppSvc->PublishEvent( app::ToggleAutoUpdate );	// main dialog will toggle the timer on/off
}

void COptions::OnUpdateOption( CCmdUI* pCmdUI )
{
	__super::OnUpdateOption( pCmdUI );

	switch ( pCmdUI->m_nID )
	{
		case ID_AUTO_UPDATE_REFRESH_CHECK:
			pCmdUI->Enable( !m_hasUIPI );					// disabled for Windows 8+
			break;
	}
}


// command handlers

BEGIN_MESSAGE_MAP( COptions, CRegistryOptions )
END_MESSAGE_MAP()
