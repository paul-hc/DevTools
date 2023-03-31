
#include "pch.h"
#include "TestColorsDialog.h"
#include "utl/UI/Color.h"
#include "utl/UI/StdColors.h"
#include "utl/UI/ColorPickerButton.h"
#include "utl/UI/ColorRepository.h"
#include "utl/UI/WndUtils.h"
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
	, m_pMyColorPicker( new CColorPickerButton() )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	m_colorPickerButton.EnableAutomaticButton( _T("Automatic"), color::Yellow );
	m_colorPickerButton.EnableOtherButton( _T("Other") );
	m_colorPickerButton.SetColumnsNumber( 16 );
	m_colorPickerButton.SetColor( m_color );

	m_pMyColorPicker->EnableAutomaticButton( _T("Automatic"), color::Lime );
	m_pMyColorPicker->EnableOtherButton( _T("Other") );
//	m_pMyColorPicker->SetColumnsNumber( 16 );
	m_pMyColorPicker->SetColor( m_color );
}

CTestColorsDialog::~CTestColorsDialog()
{
}

void CTestColorsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_colorPickerButton.m_hWnd;

	ui::DDX_ColorButton( pDX, IDC_COLOR_PICKER_BUTTON, m_colorPickerButton, &m_color );
	ui::DDX_ColorButton( pDX, IDC_MY_COLOR_PICKER_BUTTON, *m_pMyColorPicker, &m_color );

	ui::DDX_ColorText( pDX, IDC_RGB_EDIT, &m_color );
	ui::DDX_ColorRepoText( pDX, IDC_REPO_COLOR_INFO_EDIT, m_color );

	if ( firstInit )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		CClientDC dc( this );
		CPalette halftonePalette;
		VERIFY( halftonePalette.CreateHalftonePalette( &dc ) );

		m_colorPickerButton.SetPalette( &halftonePalette );
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTestColorsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_COLOR_PICKER_BUTTON, OnColorPicker )
	ON_BN_CLICKED( IDC_MY_COLOR_PICKER_BUTTON, OnMyColorPicker )
END_MESSAGE_MAP()

void CTestColorsDialog::OnColorPicker( void )
{
	m_color = m_colorPickerButton.GetColor();
	if ( CLR_NONE == m_color )
		m_color = m_colorPickerButton.GetAutomaticColor();

	UpdateData( DialogOutput );
}

void CTestColorsDialog::OnMyColorPicker( void )
{
	m_color = m_pMyColorPicker->GetColor();
	if ( CLR_NONE == m_color )
		m_color = m_pMyColorPicker->GetAutomaticColor();

	UpdateData( DialogOutput );
}
