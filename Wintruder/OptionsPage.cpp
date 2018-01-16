
#include "stdafx.h"
#include "OptionsPage.h"
#include "AppService.h"
#include "MainDialog.h"
#include "Application.h"
#include "wnd/WndUtils.h"
#include "utl/EnumTags.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


COptionsPage::COptionsPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_PAGE )
	, m_pOptions( app::GetOptions() )
{
	app::GetSvc().AddObserver( this );
}

COptionsPage::~COptionsPage()
{
	app::GetSvc().RemoveObserver( this );
}

void COptionsPage::OnAppEvent( app::Event appEvent )
{
	switch ( appEvent )
	{
		case app::OptionChanged:
			m_pOptions->Save();			// save right away so that new app instances read the current state
			break;
	}
}

void COptionsPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_autoUpdateTargetCombo.m_hWnd;

	ui::DDX_EnumCombo( pDX, ID_FRAME_STYLE_COMBO, m_frameStyleCombo, m_pOptions->m_frameStyle, opt::GetTags_FrameStyle() );
	ui::DDX_EnumCombo( pDX, ID_QUERY_WND_ICONS_COMBO, m_queryWndIconsCombo, m_pOptions->m_queryWndIcons, opt::GetTags_QueryWndIcons() );
	ui::DDX_EnumCombo( pDX, ID_AUTO_UPDATE_TARGET_COMBO, m_autoUpdateTargetCombo, m_pOptions->m_updateTarget, opt::GetTags_AutoUpdateTarget() );

	if ( firstInit )
	{
		EnableToolTips( TRUE );
		ui::SetSpinRange( this, ID_FRAME_SIZE_SPIN, 1, 50 );
		ui::SetSpinRange( this, ID_AUTO_UPDATE_TIMEOUT_SPIN, 1, 50 );
		ui::EnableControl( m_hWnd, ID_AUTO_UPDATE_REFRESH_CHECK, !wnd::HasUIPI() );		// enabled for Windows 7-
	}

	ui::DDX_Bool( pDX, ID_TOP_MOST_CHECK, m_pOptions->m_keepTopmost );
	ui::DDX_Bool( pDX, ID_AUTO_HIDE_CHECK, m_pOptions->m_hideOnTrack );
	ui::DDX_Bool( pDX, ID_AUTO_HILIGHT_CHECK, m_pOptions->m_autoHighlight );
	ui::DDX_Number( pDX, ID_FRAME_SIZE_EDIT, m_pOptions->m_frameSize );

	ui::DDX_Bool( pDX, ID_IGNORE_DISABLED_CHECK, m_pOptions->m_ignoreDisabled );
	ui::DDX_Bool( pDX, ID_IGNORE_HIDDEN_CHECK, m_pOptions->m_ignoreHidden );
	ui::DDX_Bool( pDX, ID_DISPLAY_ZERO_FLAGS_CHECK, m_pOptions->m_displayZeroFlags );

	ui::DDX_Bool( pDX, ID_AUTO_UPDATE_CHECK, m_pOptions->m_autoUpdate );
	ui::DDX_Bool( pDX, ID_AUTO_UPDATE_REFRESH_CHECK, m_pOptions->m_autoUpdateRefresh );
	ui::DDX_Number( pDX, ID_AUTO_UPDATE_TIMEOUT_EDIT, m_pOptions->m_autoUpdateTimeout );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		m_pOptions->Save();			// save right away so that new app instances read the current state

	CLayoutPropertyPage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( COptionsPage, CLayoutPropertyPage )
	ON_COMMAND_RANGE( ID_TOP_MOST_CHECK, ID_AUTO_UPDATE_TARGET_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( ID_FRAME_STYLE_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( ID_QUERY_WND_ICONS_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( ID_AUTO_UPDATE_TARGET_COMBO, OnFieldModified )
	ON_EN_CHANGE( ID_AUTO_UPDATE_TIMEOUT_EDIT, OnEnChange_AutoUpdateRate )
END_MESSAGE_MAP()

void COptionsPage::OnFieldModified( void )
{
	UpdateData( DialogSaveChanges );
}

void COptionsPage::OnFieldModified( UINT ctrlId )
{
	OnFieldModified();

	switch ( ctrlId )
	{
		case ID_TOP_MOST_CHECK:
			ui::SetTopMost( AfxGetMainWnd()->GetSafeHwnd(), m_pOptions->m_keepTopmost );
			break;
		case ID_AUTO_UPDATE_CHECK:
			app::GetMainDialog()->GetAutoUpdateTimer()->SetStarted( m_pOptions->m_autoUpdate );
			break;
	}
}

void COptionsPage::OnEnChange_AutoUpdateRate( void )
{
	if ( m_autoUpdateTargetCombo.m_hWnd != NULL )		// subclassed
	{
		OnFieldModified();
		app::GetMainDialog()->GetAutoUpdateTimer()->SetElapsed( m_pOptions->m_autoUpdateTimeout * 1000 );
	}
}

void COptionsPage::OnSelChange_AutoUpdateTarget( void )
{
	OnFieldModified();
}
