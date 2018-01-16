
#include "stdafx.h"
#include "Options.h"
#include "wnd/WndUtils.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_options[] = _T("Options");
	static const TCHAR entry_keepTopmost[] = _T("KeepTopmost");
	static const TCHAR entry_hideOnTrack[] = _T("HideOnTrack");
	static const TCHAR entry_autoHighlight[] = _T("AutoHighlight");
	static const TCHAR entry_ignoreHidden[] = _T("IgnoreHidden");
	static const TCHAR entry_ignoreDisabled[] = _T("IgnoreDisabled");
	static const TCHAR entry_displayZeroFlags[] = _T("DisplayZeroFlags");
	static const TCHAR entry_frameStyle[] = _T("FrameStyle");
	static const TCHAR entry_frameSize[] = _T("FrameSize");
	static const TCHAR entry_queryWndIcons[] = _T("QueryWndIcons");
	static const TCHAR entry_autoUpdate[] = _T("AutoUpdate");
	static const TCHAR entry_autoUpdateRefresh[] = _T("AutoUpdateRefresh");
	static const TCHAR entry_autoUpdateTimeout[] = _T("AutoUpdateTimeout");
	static const TCHAR entry_updateTarget[] = _T("UpdateTarget");
}


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


// COptions implementation

COptions::COptions( void )
	: m_keepTopmost( false )
	, m_hideOnTrack( false )
	, m_autoHighlight( true )
	, m_ignoreHidden( true )
	, m_ignoreDisabled( false )
	, m_displayZeroFlags( false )
	, m_frameStyle( opt::NonClient )
	, m_frameSize( ::GetSystemMetrics( SM_CXFIXEDFRAME ) )
	, m_queryWndIcons( opt::AllWndIcons )
	, m_autoUpdate( false )
	, m_autoUpdateRefresh( true )
	, m_autoUpdateTimeout( 3 )
	, m_updateTarget( opt::CurrentWnd )
{
	if ( wnd::HasUIPI() )
		m_queryWndIcons = opt::NoWndIcons;
}

void COptions::Load( void )
{
	CWinApp* pApp = AfxGetApp();

	m_keepTopmost = pApp->GetProfileInt( reg::section_options, reg::entry_keepTopmost, m_keepTopmost ) != FALSE;
	m_hideOnTrack = pApp->GetProfileInt( reg::section_options, reg::entry_hideOnTrack, m_hideOnTrack ) != FALSE;
	m_autoHighlight = pApp->GetProfileInt( reg::section_options, reg::entry_autoHighlight, m_autoHighlight ) != FALSE;
	m_ignoreHidden = pApp->GetProfileInt( reg::section_options, reg::entry_ignoreHidden, m_ignoreHidden ) != FALSE;
	m_ignoreDisabled = pApp->GetProfileInt( reg::section_options, reg::entry_ignoreDisabled, m_ignoreDisabled ) != FALSE;
	m_displayZeroFlags = pApp->GetProfileInt( reg::section_options, reg::entry_displayZeroFlags, m_displayZeroFlags ) != FALSE;
	m_frameStyle = static_cast< opt::FrameStyle >( pApp->GetProfileInt( reg::section_options, reg::entry_frameStyle, m_frameStyle ) );
	m_frameSize = pApp->GetProfileInt( reg::section_options, reg::entry_frameSize, m_frameSize );
	m_queryWndIcons = static_cast< opt::QueryWndIcons >( pApp->GetProfileInt( reg::section_options, reg::entry_queryWndIcons, m_queryWndIcons ) );
	m_autoUpdate = pApp->GetProfileInt( reg::section_options, reg::entry_autoUpdate, m_autoUpdate ) != FALSE;
	m_autoUpdateRefresh = pApp->GetProfileInt( reg::section_options, reg::entry_autoUpdateRefresh, m_autoUpdateRefresh ) != FALSE;
	m_autoUpdateTimeout = pApp->GetProfileInt( reg::section_options, reg::entry_autoUpdateTimeout, m_autoUpdateTimeout );
	m_updateTarget = (opt::UpdateTarget)pApp->GetProfileInt( reg::section_options, reg::entry_updateTarget, m_updateTarget );

	if ( wnd::HasUIPI() )
	{
		// disable auto-refresh since UIPI can cause deadlocks or severe delays
		m_autoUpdateRefresh = false;
		pApp->WriteProfileInt( reg::section_options, reg::entry_autoUpdateRefresh, m_autoUpdateRefresh );
	}
}

void COptions::Save( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section_options, reg::entry_keepTopmost, m_keepTopmost );
	pApp->WriteProfileInt( reg::section_options, reg::entry_hideOnTrack, m_hideOnTrack );
	pApp->WriteProfileInt( reg::section_options, reg::entry_autoHighlight, m_autoHighlight );
	pApp->WriteProfileInt( reg::section_options, reg::entry_ignoreHidden, m_ignoreHidden );
	pApp->WriteProfileInt( reg::section_options, reg::entry_ignoreDisabled, m_ignoreDisabled );
	pApp->WriteProfileInt( reg::section_options, reg::entry_displayZeroFlags, m_displayZeroFlags );
	pApp->WriteProfileInt( reg::section_options, reg::entry_frameStyle, m_frameStyle );
	pApp->WriteProfileInt( reg::section_options, reg::entry_frameSize, m_frameSize );
	pApp->WriteProfileInt( reg::section_options, reg::entry_queryWndIcons, m_queryWndIcons );
	pApp->WriteProfileInt( reg::section_options, reg::entry_autoUpdate, m_autoUpdate );
	pApp->WriteProfileInt( reg::section_options, reg::entry_autoUpdateRefresh, m_autoUpdateRefresh );
	pApp->WriteProfileInt( reg::section_options, reg::entry_autoUpdateTimeout, m_autoUpdateTimeout );
	pApp->WriteProfileInt( reg::section_options, reg::entry_updateTarget, m_updateTarget );
}
