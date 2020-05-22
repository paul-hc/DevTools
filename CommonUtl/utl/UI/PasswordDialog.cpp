
#include "stdafx.h"
#include "PasswordDialog.h"
#include "LayoutEngine.h"
#include "Path.h"
#include "CmdUpdate.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR entry_showPassword[] = _T("ShowPassword");
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_DOCUMENT_NAME_STATIC, SizeX },
		{ IDC_PASSWORD_EDIT, SizeX },
		{ IDC_CONFIRM_PASSWORD_EDIT, SizeX },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CPasswordDialog::CPasswordDialog( CWnd* pParentWnd, const fs::CPath* pDocPath /*= NULL*/ )
	: CLayoutDialog( IDD_PASSWORD_DIALOG, pParentWnd )
	, m_mode( EditMode )
{
	m_regSection = _T("PasswordDialog");
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	GetLayoutEngine().DisableResizeVertically();
	LoadDlgIcon( IDD_PASSWORD_DIALOG );

	if ( pDocPath != NULL )
		m_documentLabel = str::Format( _T("Document: %s"), pDocPath->GetNameExt() );
}

void CPasswordDialog::RecreateEditCtrls( void )
{
	ASSERT( EditMode == m_mode );

	bool showPassword = IsDlgButtonChecked( IDC_SHOW_PASSWORD_CHECK ) != FALSE;
	DWORD editStyle = m_passwordEdit.GetStyle();

	if ( !utl::ModifyValue( editStyle, MakeFlag( editStyle, ES_PASSWORD, !showPassword ) ) )
		return;

	ui::RecreateControl( &m_passwordEdit, editStyle );
	ui::RecreateControl( &m_confirmPasswordEdit, m_confirmPasswordEdit.GetStyle() );		// just to keep identical looking fonts (since SetFont fails for ES_PASSWORD)

	static const UINT s_ctrlIds[] = { IDC_CONFIRM_PASSWORD_STATIC, IDC_CONFIRM_PASSWORD_EDIT };
	ui::ShowControls( m_hWnd, ARRAY_PAIR( s_ctrlIds ), !showPassword );
}

void CPasswordDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_passwordEdit.m_hWnd;

	m_passwordEdit.DDX_Text( pDX, m_password, IDC_PASSWORD_EDIT );
	DDX_Control( pDX, IDC_CONFIRM_PASSWORD_EDIT, m_confirmPasswordEdit );

	if ( firstInit )
	{
		ui::SetDlgItemText( this, IDC_DOCUMENT_NAME_STATIC, m_documentLabel );

		if ( EditMode == m_mode )
		{
			bool showPassword = AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_showPassword, false ) != false;

			CheckDlgButton( IDC_SHOW_PASSWORD_CHECK, showPassword );
			RecreateEditCtrls();
			m_confirmPasswordEdit.SetText( m_password );		// auto-synchronize confirmation edit
		}
		else
		{
			static const UINT s_hiddenCtrlIds[] = { IDC_CONFIRM_PASSWORD_STATIC, IDC_CONFIRM_PASSWORD_EDIT, IDC_SHOW_PASSWORD_CHECK };
			ui::ShowControls( m_hWnd, ARRAY_PAIR( s_hiddenCtrlIds ), false );
		}
	}

	ui::DDX_Text( pDX, IDC_PASSWORD_EDIT, m_password );

	if ( EditMode == m_mode && IsDlgButtonChecked( IDC_SHOW_PASSWORD_CHECK ) )
		m_confirmPasswordEdit.SetText( m_password );		// auto-synchronize confirmation edit

	ui::UpdateDlgItemUI( this, IDOK );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CPasswordDialog, CLayoutDialog )
	ON_UPDATE_COMMAND_UI( IDOK, OnUpdateOK )
	ON_EN_CHANGE( IDC_PASSWORD_EDIT, OnModified )
	ON_EN_CHANGE( IDC_CONFIRM_PASSWORD_EDIT, OnModified )
	ON_BN_CLICKED( IDC_SHOW_PASSWORD_CHECK, OnToggleShowPassword )
END_MESSAGE_MAP()

void CPasswordDialog::OnUpdateOK( CCmdUI* pCmdUI )
{
	switch ( m_mode )
	{
		case EditMode:
			pCmdUI->Enable( IsDlgButtonChecked( IDC_SHOW_PASSWORD_CHECK ) || m_password == m_confirmPasswordEdit.GetText() );
			break;
		case VerifyMode:
			pCmdUI->Enable( m_password == m_validPassword );
	}
}

void CPasswordDialog::OnModified( void )
{
	UpdateData( DialogSaveChanges );
}

void CPasswordDialog::OnToggleShowPassword( void )
{
	REQUIRE( EditMode == m_mode );

	AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_showPassword, BST_CHECKED == IsDlgButtonChecked( IDC_SHOW_PASSWORD_CHECK ) );
	RecreateEditCtrls();
	GotoDlgCtrl( &m_passwordEdit );
}
