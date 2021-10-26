
#include "stdafx.h"
#include "DetailButton.h"
#include "CmdInfoStore.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDetailButton implementation

CDetailButton::CDetailButton( ui::IBuddyCommand* pOwnerCallback, UINT iconId /*= 0*/ )
	: CIconButton( iconId, false )
	, m_pOwnerCallback( pOwnerCallback )
	, m_pHostCtrl( NULL )
	, m_buddyLayout( H_AlignRight | V_AlignCenter, Spacing )
{
	ASSERT_PTR( m_pOwnerCallback );
	SetIconId( ID_EDIT_DETAILS );
}

void CDetailButton::Create( CWnd* pHostCtrl )
{
	m_pHostCtrl = pHostCtrl;
	ASSERT_PTR( m_pHostCtrl->GetSafeHwnd() );

	CRect hostRect = ui::GetControlRect( m_pHostCtrl->GetSafeHwnd() );
	CRect buttonRect( 0, 0, hostRect.Height(), hostRect.Height() );		// square size of hostRect.Height()

	ui::AlignRect( buttonRect, hostRect, m_buddyLayout.m_alignment );

	CWnd* pParentDialog = m_pHostCtrl->GetParent();
	enum { ButtonStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_CENTER | BS_VCENTER };
	VERIFY( CIconButton::Create( _T("..."), ButtonStyle, buttonRect, pParentDialog, (UINT)ButtonId ) );
	ui::SetTabOrder( this, m_pHostCtrl );
	SetFont( pParentDialog->GetFont() );

	m_buddyLayout.ShrinkBuddy( m_pHostCtrl, this );				// shrink the host so both are side-by-side

	// strangely the initial SetIcon call on PreSubclassWindow() doesn't stick -> set the icon again
	UpdateIcon();
}

void CDetailButton::LayoutButton( void )
{
	m_buddyLayout.LayoutCtrl( this, m_pHostCtrl );		// layout this button
}

void CDetailButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	cmdId, pTooltip;

	if ( UINT iconId = GetIconId().m_id )
		if ( const ui::CCmdInfo* pFoundInfo = ui::CCmdInfoStore::Instance().RetrieveInfo( iconId ) )
			if ( pFoundInfo->IsValid() )
				rText = pFoundInfo->m_tooltipText;
}

BEGIN_MESSAGE_MAP( CDetailButton, CIconButton )
	ON_CONTROL_REFLECT_EX( BN_CLICKED, OnReflect_BnClicked )
END_MESSAGE_MAP()

BOOL CDetailButton::OnReflect_BnClicked( void )
{
	m_pOwnerCallback->OnBuddyCommand( GetDlgCtrlID() );
	return TRUE;		// skip parent handling
}
