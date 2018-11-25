
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
			UpdateData( DialogOutput );
			break;
	}
}

void COptionsPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_auTargetCombo.m_hWnd;

	DDX_Control( pDX, ID_FRAME_STYLE_COMBO, m_frameStyleCombo );
	DDX_Control( pDX, ID_FRAME_SIZE_EDIT, m_frameSizeEdit );
	DDX_Control( pDX, ID_QUERY_WND_ICONS_COMBO, m_queryWndIconsCombo );
	DDX_Control( pDX, ID_AUTO_UPDATE_TARGET_COMBO, m_auTargetCombo );
	DDX_Control( pDX, ID_AUTO_UPDATE_TIMEOUT_EDIT, m_auTimeoutEdit );

	if ( firstInit )
	{
		EnableToolTips( TRUE );

		ui::WriteComboItems( m_frameStyleCombo, opt::GetTags_FrameStyle().GetUiTags() );
		ui::WriteComboItems( m_queryWndIconsCombo, opt::GetTags_QueryWndIcons().GetUiTags() );
		ui::WriteComboItems( m_auTargetCombo, opt::GetTags_AutoUpdateTarget().GetUiTags() );

		m_frameSizeEdit.SetValidRange( Range< int >( 1, 50 ) );
		m_auTimeoutEdit.SetValidRange( Range< int >( 1, 60 ) );
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		ui::UpdateControlsUI( this );		// update check-boxes (check-state and enabling)

		m_frameSizeEdit.SetNumber( m_pOptions->m_frameSize );
		m_frameStyleCombo.SetCurSel( m_pOptions->m_frameStyle );
		m_queryWndIconsCombo.SetCurSel( m_pOptions->m_queryWndIcons );
		m_auTargetCombo.SetCurSel( m_pOptions->m_updateTarget );
		m_auTimeoutEdit.SetNumber( m_pOptions->m_autoUpdateTimeout );
	}

	__super::DoDataExchange( pDX );
}

BOOL COptionsPage::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pOptions->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( COptionsPage, CLayoutPropertyPage )
	ON_CBN_SELCHANGE( ID_FRAME_STYLE_COMBO, OnCbnSelchange_FrameStyle )
	ON_EN_CHANGE( ID_FRAME_SIZE_EDIT, OnEnChange_FrameSize )
	ON_CBN_SELCHANGE( ID_QUERY_WND_ICONS_COMBO, OnCbnSelchange_QueryWndIcons )
	ON_CBN_SELCHANGE( ID_AUTO_UPDATE_TARGET_COMBO, OnCbnSelchange_AutoUpdateTarget )
	ON_EN_CHANGE( ID_AUTO_UPDATE_TIMEOUT_EDIT, OnEnChange_AutoUpdateRate )
END_MESSAGE_MAP()

void COptionsPage::OnCbnSelchange_FrameStyle( void )
{
	m_pOptions->ModifyOption( &m_pOptions->m_frameStyle, static_cast< opt::FrameStyle >( m_frameStyleCombo.GetCurSel() ) );
}

void COptionsPage::OnEnChange_FrameSize( void )
{
	int frameSize;
	if ( m_frameSizeEdit.ParseNumber( &frameSize ) )
		m_pOptions->ModifyOption( &m_pOptions->m_frameSize, frameSize );
}

void COptionsPage::OnCbnSelchange_QueryWndIcons( void )
{
	m_pOptions->ModifyOption( &m_pOptions->m_queryWndIcons, static_cast< opt::QueryWndIcons >( m_queryWndIconsCombo.GetCurSel() ) );
}

void COptionsPage::OnCbnSelchange_AutoUpdateTarget( void )
{
	m_pOptions->ModifyOption( &m_pOptions->m_updateTarget, static_cast< opt::UpdateTarget >( m_auTargetCombo.GetCurSel() ) );
}

void COptionsPage::OnEnChange_AutoUpdateRate( void )
{
	int timeout;
	if ( m_auTimeoutEdit.ParseNumber( &timeout ) )
		if ( m_pOptions->ModifyOption( &m_pOptions->m_autoUpdateTimeout, timeout ) )
			app::GetMainDialog()->GetAutoUpdateTimer()->SetElapsed( m_pOptions->m_autoUpdateTimeout * 1000 );
}
