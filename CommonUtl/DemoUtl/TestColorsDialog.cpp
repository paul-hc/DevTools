
#include "pch.h"
#include "TestColorsDialog.h"
#include "Application.h"
#include "utl/UI/Color.h"
#include "utl/UI/StdColors.h"
#include "utl/UI/ColorPickerButton.h"
#include "utl/UI/ColorRepository.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	void DDX_ColorButton( CDataExchange* pDX, int ctrlId, CMFCColorButton& rCtrl, COLORREF* pColor )
	{
		ASSERT_PTR( pColor );
		DDX_Control( pDX, ctrlId, rCtrl );

		if ( pColor != nullptr )
			if ( DialogOutput == pDX->m_bSaveAndValidate )
				rCtrl.SetColor( *pColor );
			else
			{
				*pColor = rCtrl.GetColor();
				//if ( CLR_NONE == *pColor )
				//	*pColor = rCtrl.GetAutomaticColor();
			}
	}

	void DDX_ColorText( CDataExchange* pDX, int ctrlId, COLORREF* pColor, bool doInput /*= false*/ )
	{
		HWND hCtrl = pDX->PrepareEditCtrl( ctrlId );
		ASSERT_PTR( pColor );
		ASSERT_PTR( hCtrl );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ui::SetWindowText( hCtrl, ui::FormatColor( *pColor ) );
		else if ( doInput && ui::IsWriteableEditBox( hCtrl ) )
		{
			std::tstring text = ui::GetWindowText( hCtrl );
			if ( !ui::ParseColor( pColor, text.c_str() ) )
				 ui::ddx::FailInput( pDX, ctrlId, str::Format( _T("Error parsing color text: '%s'"), text.c_str() ) );
		}
	}

	void DDX_ColorRepoText( CDataExchange* pDX, int ctrlId, COLORREF color )
	{
		HWND hCtrl = pDX->PrepareEditCtrl( ctrlId );
		ASSERT_PTR( hCtrl );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ui::SetWindowText( hCtrl, CColorRepository::Instance()->FormatColorMatch( color ) );
	}
}


namespace reg
{
	static const TCHAR section_dialog[] = _T("TestColorsDialog");
	static const TCHAR entry_color[] = _T("Color");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_RGB_EDIT, SizeX },
		{ IDC_REPO_COLOR_INFO_EDIT, SizeX },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}


// CTestColorsDialog implementation

CTestColorsDialog::CTestColorsDialog( CWnd* pParent )
	: CLayoutDialog( IDD_TEST_COLORS_DIALOG, pParent )
	, m_color( CLR_NONE )
	, m_pUtlColorPicker( new CColorPickerButton() )
	, m_pMenuPicker( new CMenuPickerButton() )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	m_mfcColorPickerButton.EnableAutomaticButton( _T("Automatic"), color::Yellow );
	m_mfcColorPickerButton.EnableOtherButton( _T("More...") );
	ui::TMFCColorList docColors;
	CColorRepository::Instance()->GetTable( ui::Office2003_Colors )->QueryMfcColors( docColors );
	m_mfcColorPickerButton.SetDocumentColors( _T(" ") /*_T("Document")*/, docColors);
//	m_mfcColorPickerButton.SetColumnsNumber( 16 );
	m_mfcColorPickerButton.SetColor( m_color );

	m_pUtlColorPicker->SetAutomaticColor( color::Lime );
	m_pUtlColorPicker->SetColor( m_color );

	ui::LoadPopupMenu( &m_popupMenu, IDR_CONTEXT_MENU, app::TestColorsPopup );
	m_pMenuPicker->m_bOSMenu = FALSE;
	m_editChecked = true;
}

CTestColorsDialog::~CTestColorsDialog()
{
}

void CTestColorsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_mfcColorPickerButton.m_hWnd;

	ui::DDX_ColorButton( pDX, IDC_COLOR_PICKER_BUTTON, m_mfcColorPickerButton, &m_color );
	ui::DDX_ColorButton( pDX, IDC_MY_COLOR_PICKER_BUTTON, *m_pUtlColorPicker, &m_color );
	DDX_Control( pDX, IDC_MY_MENU_PICKER_BUTTON, *m_pMenuPicker );

	ui::DDX_ColorText( pDX, IDC_RGB_EDIT, &m_color );
	ui::DDX_ColorRepoText( pDX, IDC_REPO_COLOR_INFO_EDIT, m_color );

	if ( firstInit )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		CMFCToolBar::AddToolBarForImageCollection( IDR_STD_STRIP );

		//CMFCButton::EnableWindowsTheming();		already on!

		m_pMenuPicker->m_hMenu = m_popupMenu;
		m_pMenuPicker->SizeToContent();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTestColorsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_COLOR_PICKER_BUTTON, OnColorPicker )
	ON_BN_CLICKED( IDC_MY_COLOR_PICKER_BUTTON, OnMyColorPicker )
	ON_BN_CLICKED( IDC_MY_MENU_PICKER_BUTTON, OnMenuPicker )
	ON_BN_CLICKED( IDC_EDIT_COLOR_BUTTON, On_EditColor )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CUT, OnUpdate_EditItem )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdate_EditItem )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ITEM, OnUpdate_EditItem )
END_MESSAGE_MAP()

void CTestColorsDialog::OnColorPicker( void )
{
	m_color = m_mfcColorPickerButton.GetColor();
	if ( CLR_NONE == m_color )
		m_color = m_mfcColorPickerButton.GetAutomaticColor();

	UpdateData( DialogOutput );
}

void CTestColorsDialog::OnMyColorPicker( void )
{
	m_color = m_pUtlColorPicker->GetColor();
	if ( CLR_NONE == m_color )
		m_color = m_pUtlColorPicker->GetAutomaticColor();

	UpdateData( DialogOutput );
}

void CTestColorsDialog::OnMenuPicker( void )
{
	switch ( m_pMenuPicker->m_nMenuResult )
	{
		case ID_EDIT_CUT:
		case ID_EDIT_COPY:
		case ID_EDIT_PASTE:
		case ID_RESET_DEFAULT:
			break;
		case ID_EDIT_ITEM:
			m_editChecked = !m_editChecked;
			break;
	}
}

void CTestColorsDialog::On_EditColor( void )
{
	if ( ui::EditColor( &m_color, GetDlgItem( IDC_EDIT_COLOR_BUTTON ), !ui::IsKeyPressed( VK_CONTROL ) ) )
		UpdateData( DialogOutput );
}

void CTestColorsDialog::OnUpdate_EditItem( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case ID_EDIT_CUT:
			pCmdUI->Enable( m_editChecked );
			break;
		case ID_RESET_DEFAULT:
		case ID_EDIT_ITEM:
			pCmdUI->SetCheck( m_editChecked );
			break;
	}
}
