
#include "pch.h"
#include "PopupSplitButton.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include "VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPopupSplitButton::CPopupSplitButton( UINT menuId /*= 0*/, int popupIndex /*= 0*/ )
	: CBaseSplitButton()
	, m_pTargetWnd( nullptr )
	, m_dropItem( CThemeItem( L"COMBOBOX", vt::CP_DROPDOWNBUTTONRIGHT, vt::CBXSR_NORMAL ) )
{
	m_dropItem
		.SetStateId( CThemeItem::Hot, vt::CBXSR_HOT )
		.SetStateId( CThemeItem::Pressed, vt::CBXSR_PRESSED )
		.SetStateId( CThemeItem::Disabled, vt::CBXSR_DISABLED );

	if ( menuId != 0 )
		LoadMenu( menuId, popupIndex );
}

CPopupSplitButton::~CPopupSplitButton()
{
}

void CPopupSplitButton::LoadMenu( UINT menuId, int popupIndex )
{
	if ( menuId != 0 )
		ui::LoadPopupMenu( &m_popupMenu, menuId, popupIndex );
	else
		m_popupMenu.DestroyMenu();
}

CRect CPopupSplitButton::GetRhsPartRect( const CRect* pClientRect /*= nullptr*/ ) const
{
	CRect rhsRect( 0, 0, 0, 0 );
	if ( HasRhsPart() )
	{
		if ( pClientRect != nullptr )
			rhsRect = *pClientRect;
		else
			GetClientRect( &rhsRect );

		--rhsRect.right;		// exclude default frame
		rhsRect.left = rhsRect.right - DropWidth;
		rhsRect.DeflateRect( 0, 1 );
	}
	return rhsRect;
}

void CPopupSplitButton::DrawRhsPart( CDC* pDC, const CRect& clientRect )
{
	switch ( GetSplitState() )
	{
		case Normal:
		case HotButton:
		{
			CBaseSplitButton::DrawRhsPart( pDC, clientRect );
			break;
		}
	}

	CRect rhsRect = GetRhsPartRect( &clientRect );
	SplitState splitState = GetSplitState();
	CThemeItem::Status drawStatus = CThemeItem::Normal;

	if ( HasFlag( GetStyle(), WS_DISABLED ) )
		drawStatus = CThemeItem::Disabled;
	else if ( RhsPressed == splitState )
		drawStatus = CThemeItem::Pressed;
	else if ( HotRhs == splitState )
		drawStatus = CThemeItem::Hot;

	m_dropItem.DrawStatusBackground( drawStatus, *pDC, rhsRect );
}

void CPopupSplitButton::DropDown( void )
{
	ASSERT_PTR( m_popupMenu.GetSafeHmenu() );
	CRect buttonRect;
	GetWindowRect( &buttonRect );
	buttonRect.DeflateRect( 1, 1 );		// exclude default frame

	CPoint trackPos( buttonRect.left, buttonRect.bottom );
	ui::TrackPopupMenu( m_popupMenu, m_pTargetWnd != nullptr ? m_pTargetWnd : GetParent(), trackPos, TPM_LEFTALIGN | TPM_TOPALIGN, &buttonRect );
}


// message handlers

BEGIN_MESSAGE_MAP( CPopupSplitButton, CBaseSplitButton )
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CPopupSplitButton::OnLButtonDown( UINT flags, CPoint point )
{
	if ( HasRhsPart() && GetSplitState() != RhsPressed )
		if ( GetRhsPartRect().PtInRect( point ) )
		{
			ui::TakeFocus( *this );
			SetSplitState( RhsPressed );
			DropDown();
			SetSplitState( Normal );
			Invalidate();
			return;
		}

	CBaseSplitButton::OnLButtonDown( flags, point );
}
