
#include "pch.h"
#include "ColorPopupDialogs.h"
#include <colordlg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


enum
{
	IDC_HUE_SPIN = COLOR_SOLID_RIGHT + 50,
	IDC_SAT_SPIN,
	IDC_LUM_SPIN,
	IDC_RED_SPIN,
	IDC_GREEN_SPIN,
	IDC_BLUE_SPIN
};


CColorPopupDialog::CColorPopupDialog( CWnd* pParentWnd, COLORREF color, DWORD dwFlags /*= CC_FULLOPEN | CC_ANYCOLOR | CC_RGBINIT*/ )
	: CBasePopupColorDialog<CColorDialog>( color, dwFlags, pParentWnd )
{
}

COLORREF CColorPopupDialog::GetCurrentColor( void ) const overrides(CBasePopupColorDialog)
{
	return RGB(
		GetDlgItemInt( COLOR_RED, nullptr, FALSE ),
		GetDlgItemInt( COLOR_GREEN, nullptr, FALSE ),
		GetDlgItemInt( COLOR_BLUE, nullptr, FALSE )
	);
}

void CColorPopupDialog::InitDialog( void ) overrides(CBasePopupColorDialog)
{
	__super::InitDialog();

	// attach spin buttons for color component edit fields
	CreateSpin( COLOR_HUE, m_hueSpin, IDC_HUE_SPIN, 240 );
	CreateSpin( COLOR_SAT, m_satSpin, IDC_SAT_SPIN, 240 );
	CreateSpin( COLOR_LUM, m_lumSpin, IDC_LUM_SPIN, 240 );
	CreateSpin( COLOR_RED, m_redSpin, IDC_RED_SPIN, 255 );
	CreateSpin( COLOR_GREEN, m_greenSpin, IDC_GREEN_SPIN, 255 );
	CreateSpin( COLOR_BLUE, m_blueSpin, IDC_BLUE_SPIN, 255 );
	OffsetControl( COLOR_REDACCEL, 1 );
	OffsetControl( COLOR_GREENACCEL, 1 );
	OffsetControl( COLOR_BLUEACCEL, 1 );
	OffsetControl( COLOR_RED, 1 );
	OffsetControl( COLOR_GREEN, 1 );
	OffsetControl( COLOR_BLUE, 1 );

	// align ADD button to the right of spin
	CWnd* pAddCustomColorButton = GetDlgItem( COLOR_ADD );
	CRect blueEditRect = ui::GetDlgItemRect( m_hWnd, COLOR_BLUE );
	CRect addButtonRect = ui::GetControlRect( pAddCustomColorButton->GetSafeHwnd() );
	enum { BtnSpacingX = 10 };

	blueEditRect.right = ui::GetControlRect( m_blueSpin ).right;		// edit + spin area
	addButtonRect.right = blueEditRect.left - BtnSpacingX;
	pAddCustomColorButton->MoveWindow( &addButtonRect );

	CRect pickButtonRect( blueEditRect.left, addButtonRect.top, blueEditRect.right, addButtonRect.bottom );

	VERIFY( m_menuPickerStatic.Create( _T("Edit"), WS_CHILD | WS_VISIBLE | WS_GROUP | SS_NOTIFY | SS_CENTER | SS_CENTERIMAGE, pickButtonRect, this, IDC_EDIT_PICK_STATIC ) );
	m_menuPickerStatic.SetFont( pAddCustomColorButton->GetFont() );		// share the dialog control font
}

void CColorPopupDialog::AdjustDlgWindowRect( CRect& rWindowRect ) overrides(CBasePopupColorDialog)
{
	enum { SpacingRight = 5 };

	rWindowRect.right += SpacingRight;		// a bit more room for the spin buttons
}

void CColorPopupDialog::CreateSpin( UINT editId, CSpinButtonCtrl& rSpinButton, UINT spinId, int maxValue )
{
	int editPos = (int)GetDlgItemInt( editId, nullptr, FALSE );
	CEdit* pEditBuddy = (CEdit*)GetDlgItem( editId );
	ASSERT_PTR( pEditBuddy );

	CRect editRect;
	pEditBuddy->GetWindowRect( &editRect );
	ScreenToClient( &editRect );
	editRect.left -= 1;
	editRect.right += ( GetSystemMetrics( SM_CXHSCROLL ) - 2 );
	pEditBuddy->MoveWindow( &editRect );

	rSpinButton.Create( WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT, CRect( 0, 0, 0, 0 ), this, spinId );
	rSpinButton.SetWindowPos( pEditBuddy, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE );
	rSpinButton.SetBuddy( pEditBuddy );
	rSpinButton.SetRange( 0, (short)maxValue );
	rSpinButton.SetPos( editPos );
}

void CColorPopupDialog::OffsetControl( UINT ctrlId, int offsetX )
{
	if ( CWnd* pCtrl = GetDlgItem( ctrlId ) )
	{
		CRect windowRect;
		pCtrl->GetWindowRect( &windowRect );
		ScreenToClient( &windowRect );
		windowRect.OffsetRect( offsetX, 0 );
		pCtrl->MoveWindow( &windowRect );
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CColorPopupDialog, CBasePopupColorDialog<CColorDialog> )
END_MESSAGE_MAP()


namespace reg
{
	static const TCHAR section_Settings[] = _T("Settings");			// default section
	static const TCHAR entry_ActiveColorPage[] = _T("ActiveColorPage");
}


// COfficeColorPopupDialog implementation

COfficeColorPopupDialog::COfficeColorPopupDialog( CWnd* pParentWnd, COLORREF color, int activePageIndex /*= -1*/ )
	: CBasePopupColorDialog<CMFCColorDialog>( color, 0, pParentWnd )
	, m_activePageIndex( activePageIndex != -1 ? activePageIndex : AfxGetApp()->GetProfileInt( reg::section_Settings, reg::entry_ActiveColorPage, 0 ) )
	, m_pPropertySheet( nullptr )
{
	m_menuPickerStatic.SetPopupAlign( ui::DropRight );
}

void COfficeColorPopupDialog::ModifyColor( COLORREF newColor ) overrides(CBasePopupColorDialog)
{
	SetNewColor( newColor );
	SetPageOne( GetRValue( m_NewColor ), GetGValue( m_NewColor ), GetBValue( m_NewColor ) );
	SetPageTwo( GetRValue( m_NewColor ), GetGValue( m_NewColor ), GetBValue( m_NewColor ) );
}

void COfficeColorPopupDialog::InitDialog( void ) overrides(CBasePopupColorDialog)
{
	__super::InitDialog();

	CWnd* pSelectButton = GetDlgItem( IDC_AFXBARRES_COLOR_SELECT );
	CRect selectBtnRect = ui::GetControlRect( pSelectButton->GetSafeHwnd() );
	enum { BtnSpacingY = 10 };

	CRect pickButtonRect = selectBtnRect;
	pickButtonRect.OffsetRect( 0, selectBtnRect.Height() + BtnSpacingY );

	VERIFY( m_menuPickerStatic.Create( _T("&Tools"), WS_CHILD | WS_VISIBLE | WS_GROUP | SS_NOTIFY | SS_CENTER | SS_CENTERIMAGE, pickButtonRect, this, IDC_EDIT_PICK_STATIC ) );
	m_menuPickerStatic.SetFont( pSelectButton->GetFont() );		// share the dialog control font
}

BOOL COfficeColorPopupDialog::OnInitDialog( void ) overrides(CMFCColorDialog)
{
	BOOL result = __super::OnInitDialog();

	// only now the child property sheet is created and available
	m_pPropertySheet = (CPropertySheet*)m_pPropSheet;
	m_pPropertySheet->SetActivePage( m_activePageIndex );
	return result;
}


// message handlers

BEGIN_MESSAGE_MAP( COfficeColorPopupDialog, CBasePopupColorDialog<CMFCColorDialog> )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void COfficeColorPopupDialog::OnDestroy( void )
{
	if ( m_pPropertySheet != nullptr )
		AfxGetApp()->WriteProfileInt( reg::section_Settings, reg::entry_ActiveColorPage, m_pPropertySheet->GetActiveIndex() );

	__super::OnDestroy();
}
