
#include "stdafx.h"
#include "BaseDetailHostCtrl.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CItemListEdit::CDetailButton implementation

CDetailButton::CDetailButton( ui::IBuddyCommand* pOwnerCallback, UINT iconId /*= 0*/ )
	: CIconButton( iconId, false )
	, m_pOwnerCallback( pOwnerCallback )
	, m_pHostCtrl( NULL )
	, m_spacingToButton( SpacingToButton )
{
	ASSERT_PTR( m_pOwnerCallback );
	SetIconId( ID_EDIT_DETAILS );
}

void CDetailButton::Create( CWnd* pHostCtrl )
{
	m_pHostCtrl = pHostCtrl;
	ASSERT_PTR( m_pHostCtrl->GetSafeHwnd() );

	CRect hostRect = GetHostCtrlRect();
	CRect buttonRect = hostRect;

	buttonRect.left = buttonRect.right - buttonRect.Height();
	hostRect.right = buttonRect.left - m_spacingToButton;
	m_pHostCtrl->MoveWindow( &hostRect );

	CWnd* pParentDialog = m_pHostCtrl->GetParent();
	enum { ButtonStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_CENTER | BS_VCENTER };
	VERIFY( CIconButton::Create( _T("..."), ButtonStyle, buttonRect, pParentDialog, (UINT)ButtonId ) );
	SetWindowPos( m_pHostCtrl, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );		// move in tab order after the host
	SetFont( pParentDialog->GetFont() );

	// strangely the initial SetIcon call on PreSubclassWindow() doesn't stick -> set the icon again
	UpdateIcon();
}

void CDetailButton::Layout( void )
{
	// layout both: host & detail
	CRect hostRect = GetHostCtrlRect();
	CRect buttonRect;
	GetWindowRect( &buttonRect );

	CSize buttonSize = buttonRect.Size();

	buttonRect.left = hostRect.right + m_spacingToButton;
	buttonRect.top = hostRect.top;
	buttonRect.right = buttonRect.left + buttonSize.cx;
	buttonRect.bottom = buttonRect.top + buttonSize.cy;

	MoveWindow( &buttonRect );
}

CRect CDetailButton::GetHostCtrlRect( void ) const
{
	CRect hostRect = ui::GetControlRect( *m_pHostCtrl );
	if ( CComboBox* pComboBox = dynamic_cast< CComboBox* >( m_pHostCtrl ) )
		if ( CBS_SIMPLE == ( pComboBox->GetStyle() & 0x0F ) )
			hostRect.bottom = hostRect.top + 21;			// magic combo height

	return hostRect;
}

BEGIN_MESSAGE_MAP( CDetailButton, CIconButton )
	ON_CONTROL_REFLECT_EX( BN_CLICKED, OnReflect_BnClicked )
END_MESSAGE_MAP()

BOOL CDetailButton::OnReflect_BnClicked( void )
{
	m_pOwnerCallback->OnBuddyCommand( GetDlgCtrlID() );
	return TRUE;		// skip parent handling
}
