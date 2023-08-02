
#include "pch.h"
#include "TestColorsDialog.h"
#include "Application.h"
#include "utl/UI/Color.h"
#include "utl/UI/StdColors.h"
#include "utl/UI/ColorPickerButton.h"
#include "utl/UI/MenuPickerButton.h"
#include "utl/UI/ColorRepository.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("TestColorsDialog");
	static const TCHAR entry_SelColor[] = _T("SelColor");
}

namespace layout
{
	static CLayoutStyle s_styles[] =
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
	, m_color( static_cast<COLORREF>( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_SelColor, CLR_NONE ) ) )
	, m_editChecked( true )
	, m_pColorPicker( new CColorPickerButton() )
	, m_pMenuPicker( new CMenuPickerButton() )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );

	m_mfcColorPickerButton.SetColor( m_color );
	m_mfcColorPickerButton.EnableAutomaticButton( mfc::CColorLabels::s_autoLabel, color::Yellow );
	m_mfcColorPickerButton.EnableOtherButton( mfc::CColorLabels::s_moreLabel );
	mfc::TColorList docColors;
	CColorRepository::Instance()->FindTable( ui::Office2003_Colors )->QueryMfcColors( docColors );
	m_mfcColorPickerButton.SetDocumentColors( _T("Document:"), docColors );

	m_pColorPicker->SetColor( m_color );
	m_pColorPicker->SetAutomaticColor( color::Lime );

	ui::LoadPopupMenu( &m_popupMenu, IDR_CONTEXT_MENU, app::TestColorsPopup );
	m_pMenuPicker->m_hMenu = m_popupMenu;
}

CTestColorsDialog::~CTestColorsDialog()
{
}

void CTestColorsDialog::SetPickerUserColors( bool pickerUserColors )
{
	if ( pickerUserColors )
	{
		std::vector<COLORREF> userColors;
		ui::MakeHalftoneColorTable( userColors, 24 );
		m_pColorPicker->SetUserColors( userColors, 8 );
	}
	else
		m_pColorPicker->SetSelColorTable( CHalftoneRepository::Instance()->FindTable( ui::Halftone16_Colors ) );
}

void CTestColorsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_mfcColorPickerButton.m_hWnd;

	ui::DDX_ColorButton( pDX, IDC_MFC_COLOR_PICKER_BUTTON, m_mfcColorPickerButton, &m_color, true );	// evaluate color
	ui::DDX_ColorButton( pDX, IDC_COLOR_PICKER_BUTTON, *m_pColorPicker, &m_color );
	DDX_Control( pDX, IDC_MENU_PICKER_BUTTON, *m_pMenuPicker );

	ui::DDX_ColorText( pDX, IDC_RGB_EDIT, &m_color );
	ui::DDX_ColorRepoText( pDX, IDC_REPO_COLOR_INFO_EDIT, m_color );

	if ( firstInit )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		//CMFCToolBar::AddToolBarForImageCollection( IDR_STD_BUTTONS_STRIP );		// feed afxCommandManager [:CCommandManager] with images from the strip
	}

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_SelColor, m_color );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTestColorsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_MFC_COLOR_PICKER_BUTTON, OnMfcColorPicker )
	ON_BN_CLICKED( IDC_COLOR_PICKER_BUTTON, OnColorPicker )
	ON_BN_CLICKED( IDC_MENU_PICKER_BUTTON, OnMenuPicker )
	ON_BN_CLICKED( IDC_EDIT_COLOR_BUTTON, On_EditColor )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CUT, OnUpdate_EditItem )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdate_EditItem )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ITEM, OnUpdate_EditItem )
	ON_BN_CLICKED( IDC_PICKER_USER_COLORS_TOGGLE, OnToggle_PickerUserColors )
END_MESSAGE_MAP()

void CTestColorsDialog::OnMfcColorPicker( void )
{
	m_color = m_mfcColorPickerButton.GetColor();
	//if ( CLR_NONE == m_color )
	//	m_color = m_mfcColorPickerButton.GetAutomaticColor();

	UpdateData( DialogOutput );
}

void CTestColorsDialog::OnColorPicker( void )
{
	m_color = m_pColorPicker->GetColor();
	//if ( CLR_NONE == m_color )
	//	m_color = m_pColorPicker->GetAutomaticColor();

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

void CTestColorsDialog::OnToggle_PickerUserColors( void )
{
	SetPickerUserColors( BST_CHECKED == IsDlgButtonChecked( IDC_PICKER_USER_COLORS_TOGGLE ) );
}
