
#include "stdafx.h"
#include "FindWndDialog.h"
#include "Application.h"
#include "AppService.h"
#include "resource.h"
#include "wnd/FlagRepository.h"
#include "wnd/WndSearchPattern.h"
#include "wnd/WndUtils.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("FindDialog");
}


CFindWndDialog::CFindWndDialog( CWndSearchPattern* pPattern, CWnd* pParent )
	: CLayoutDialog( IDD_FIND_WND_DIALOG, pParent )
	, m_pPattern( pPattern )
{
	m_regSection = reg::section;
	LoadDlgIcon( ID_EDIT_FIND );

	m_trackWndPicker.LoadTrackCursor( IDR_POINTER_CURSOR );
	m_trackWndPicker.SetTrackIconId( ID_TRANSPARENT );
	m_trackWndPicker.SetToolIconId( IDD_MAIN_DIALOG );

	m_resetButton.LoadMenu( IDR_CONTEXT_MENU, app::ResetSplitButton );
	m_resetButton.SetIconId( ID_RESET_DEFAULT );
}

CFindWndDialog::~CFindWndDialog()
{
}

void CFindWndDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_trackWndPicker.m_hWnd;

	DDX_Control( pDX, IDC_TRACK_TOOL_ICON, m_trackWndPicker );
	DDX_Control( pDX, IDC_WCLASS_COMBO, m_wndClassCombo );
	DDX_Control( pDX, IDC_RESET_DEFAULT, m_resetButton );

	if ( firstInit )
	{
		const std::vector< CFlagStore* >& specificStores = CStyleRepository::Instance().GetSpecificStores();
		for ( std::vector< CFlagStore* >::const_iterator itStore = specificStores.begin(); itStore != specificStores.end(); ++itStore )
			for ( std::vector< std::tstring >::const_iterator itWndClass = ( *itStore )->m_wndClasses.begin(); itWndClass != ( *itStore )->m_wndClasses.end(); ++itWndClass )
				m_wndClassCombo.AddString( itWndClass->c_str() );
	}

	ui::DDX_Text( pDX, IDC_WCLASS_COMBO, m_pPattern->m_wndClass );
	ui::DDX_Text( pDX, IDC_CAPTION_EDIT, m_pPattern->m_caption );
	ui::DDX_Int( pDX, IDC_ID_EDIT, m_pPattern->m_id, INT_MAX );
	ui::DDX_Bool( pDX, IDC_MATCH_CASE_CHECK, m_pPattern->m_matchCase );
	ui::DDX_Bool( pDX, IDC_MATCH_WHOLE_CHECK, m_pPattern->m_matchWhole );
	ui::DDX_Bool( pDX, IDC_REFRESH_NOW, m_pPattern->m_refreshNow );

	static const HWND nullHandle = NULL;
	ui::DDX_Handle( pDX, IDC_HANDLE_EDIT, m_pPattern->m_handle, &nullHandle );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		CheckRadioButton( IDC_FROM_BEGINING_RADIO, IDC_FROM_CURRENT_RADIO, m_pPattern->m_fromBeginning ? IDC_FROM_BEGINING_RADIO : IDC_FROM_CURRENT_RADIO );
		CheckRadioButton( IDC_DIR_FORWARD_RADIO, IDC_DIR_BACKWARD_RADIO, m_pPattern->m_forward ? IDC_DIR_FORWARD_RADIO : IDC_DIR_BACKWARD_RADIO );
	}
	else
	{
		m_pPattern->m_fromBeginning = IsDlgButtonChecked( IDC_FROM_BEGINING_RADIO ) != FALSE;
		m_pPattern->m_forward = IsDlgButtonChecked( IDC_DIR_FORWARD_RADIO ) != FALSE;
		m_pPattern->Save();
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFindWndDialog, CLayoutDialog )
	ON_TSN_ENDTRACKING( IDC_TRACK_TOOL_ICON, OnTsnEndTracking_WndPicker )
	ON_CONTROL_RANGE( IDC_HANDLE_EDIT, IDC_ID_EDIT, EN_KILLFOCUS, OnPatternFieldChanged )
	ON_CONTROL_RANGE( IDC_WCLASS_COMBO, IDC_WCLASS_COMBO, CBN_SELCHANGE, OnPatternFieldChanged )
	ON_BN_CLICKED( IDC_RESET_DEFAULT, OnResetDefault )
	ON_COMMAND( CM_COPY_TARGET_WND, OnCopyTargetWndHandle )
	ON_UPDATE_COMMAND_UI( CM_COPY_TARGET_WND, OnUpdateCopyTargetWndHandle )
	ON_COMMAND( CM_COPY_WND_CLASS, OnCopyWndClass )
	ON_UPDATE_COMMAND_UI( CM_COPY_WND_CLASS, OnUpdateCopyWndClass )
	ON_COMMAND( CM_COPY_WND_CAPTION, OnCopyWndCaption )
	ON_UPDATE_COMMAND_UI( CM_COPY_WND_CAPTION, OnUpdateCopyWndCaption )
	ON_COMMAND( CM_COPY_WND_IDENT, OnCopyWndIdent )
	ON_UPDATE_COMMAND_UI( CM_COPY_WND_IDENT, OnUpdateCopyWndIdent )
END_MESSAGE_MAP()

void CFindWndDialog::OnCancel( void )
{
	UpdateData( DialogSaveChanges );			// save changes anyway
	EndDialog( IDCANCEL );
}

void CFindWndDialog::OnTsnEndTracking_WndPicker( void )
{
	if ( CTrackStatic::Commit == m_trackWndPicker.GetTrackingResult() )
	{
		m_pPattern->m_handle = m_trackWndPicker.GetSelectedWnd();
		UpdateData( DialogOutput );
	}
}

void CFindWndDialog::OnPatternFieldChanged( UINT ctrlId )
{
	ctrlId;
	UpdateData( DialogSaveChanges );
}

void CFindWndDialog::OnResetDefault( void )
{
	*m_pPattern = CWndSearchPattern();
	UpdateData( DialogOutput );
}

void CFindWndDialog::OnCopyTargetWndHandle( void )
{
	UpdateData( DialogSaveChanges );
	m_pPattern->m_handle = app::GetTargetWnd();
	UpdateData( DialogOutput );
}

void CFindWndDialog::OnUpdateCopyTargetWndHandle( CCmdUI* pCmdUI )
{
	CWndSpot& targetWnd = app::GetTargetWnd();
	bool enable = targetWnd.IsValid();

	pCmdUI->Enable( enable );
	pCmdUI->SetCheck( enable && m_pPattern->m_handle == targetWnd.m_hWnd );

	if ( pCmdUI->m_pMenu != NULL )
	{
		static const std::tstring baseItemText = ui::GetMenuItemText( pCmdUI->m_pMenu, pCmdUI->m_nID );
		std::tostringstream itemText;
		itemText << baseItemText;
		if ( enable )
			itemText << _T('\t') << wnd::FormatWindowHandle( targetWnd );
		pCmdUI->SetText( itemText.str().c_str() );
	}
}

void CFindWndDialog::OnCopyWndClass( void )
{
	UpdateData( DialogSaveChanges );
	m_pPattern->m_wndClass = wc::GetDisplayClassName( m_pPattern->m_handle );
	UpdateData( DialogOutput );
}

void CFindWndDialog::OnUpdateCopyWndClass( CCmdUI* pCmdUI )
{
	bool enable = false;
	if ( ui::IsValidWindow( m_pPattern->m_handle ) )
		enable = true;

	pCmdUI->Enable( enable );

	if ( pCmdUI->m_pMenu != NULL )
	{
		static const std::tstring baseItemText = ui::GetMenuItemText( pCmdUI->m_pMenu, pCmdUI->m_nID );
		std::tostringstream itemText;
		itemText << baseItemText;
		if ( enable )
		{
			std::tstring wndClass = wc::GetDisplayClassName( m_pPattern->m_handle );
			if ( !wndClass.empty() )
				itemText << _T('\t') << wndClass;
		}
		pCmdUI->SetText( itemText.str().c_str() );
	}
}

void CFindWndDialog::OnCopyWndCaption( void )
{
	UpdateData( DialogSaveChanges );
	m_pPattern->m_caption = wnd::GetWindowText( m_pPattern->m_handle );
	UpdateData( DialogOutput );
}

void CFindWndDialog::OnUpdateCopyWndCaption( CCmdUI* pCmdUI )
{
	bool enable = false;
	if ( ui::IsValidWindow( m_pPattern->m_handle ) )
		enable = true;

	pCmdUI->Enable( enable );

	if ( pCmdUI->m_pMenu != NULL )
	{
		static const std::tstring baseItemText = ui::GetMenuItemText( pCmdUI->m_pMenu, pCmdUI->m_nID );
		std::tostringstream itemText;
		itemText << baseItemText;
		if ( enable )
		{
			std::tstring caption = wnd::GetWindowText( m_pPattern->m_handle );
			if ( !caption.empty() )
				itemText << _T('\t') << caption;
		}
		pCmdUI->SetText( itemText.str().c_str() );
	}
}

void CFindWndDialog::OnCopyWndIdent( void )
{
	UpdateData( DialogSaveChanges );
	m_pPattern->m_id = ::GetDlgCtrlID( m_pPattern->m_handle );
	UpdateData( DialogOutput );
}

void CFindWndDialog::OnUpdateCopyWndIdent( CCmdUI* pCmdUI )
{
	bool enable = false;
	if ( ui::IsValidWindow( m_pPattern->m_handle ) )
		enable = ui::IsChild( m_pPattern->m_handle );

	pCmdUI->Enable( enable );

	if ( pCmdUI->m_pMenu != NULL )
	{
		static const std::tstring baseItemText = ui::GetMenuItemText( pCmdUI->m_pMenu, pCmdUI->m_nID );
		std::tostringstream itemText;
		itemText << baseItemText;
		if ( enable )
			itemText << _T('\t') << ::GetDlgCtrlID( m_pPattern->m_handle );

		pCmdUI->SetText( itemText.str().c_str() );
	}
}
