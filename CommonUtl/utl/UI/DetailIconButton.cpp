
#include "stdafx.h"
#include "DetailIconButton.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "DetailMateCtrl.hxx"


// CDetailIconButton implementation

CDetailIconButton::CDetailIconButton( ui::IBuddyCommandHandler* pHostCmdHandler, UINT iconId /*= 0*/ )
	: CDetailMateCtrl<CIconButton>( pHostCmdHandler )
{
	SetIconId( iconId != NULL ? iconId : ID_EDIT_DETAILS );
	SetUseText( false );
}

void CDetailIconButton::CreateShrinkBuddy( const ui::CBuddyLayout& buddyLayout /*= ui::CBuddyLayout::s_tileToRight*/ )
{
	ASSERT_PTR( m_pHostCtrl->GetSafeHwnd() );

	CRect hostRect = ui::GetControlRect( m_pHostCtrl->GetSafeHwnd() );
	CRect buttonRect( 0, 0, hostRect.Height(), hostRect.Height() );		// square size based on host height

	buddyLayout.Align( buttonRect, hostRect );

	enum { ButtonStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_CENTER | BS_VCENTER };
	CWnd* pParentDialog = m_pHostCtrl->GetParent();

	VERIFY( CIconButton::Create( _T("..."), ButtonStyle, buttonRect, pParentDialog, (UINT)ButtonId ) );
	SetFont( pParentDialog->GetFont() );
	ui::SetTabOrder( this, m_pHostCtrl );

	buddyLayout.ShrinkBuddy( m_pHostCtrl, this );						// shrink the host so both are side-by-side

	// strangely the initial SetIcon call on PreSubclassWindow() doesn't stick -> set the icon again
	UpdateIcon();
}
