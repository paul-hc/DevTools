
#include "stdafx.h"
#include "Control_fwd.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CBuddyLayout implementation

	const CBuddyLayout CBuddyLayout::s_tileToRight( H_AlignRight | V_AlignCenter, Spacing );

	CRect& CBuddyLayout::Align( CRect& rCtrlRect, const RECT& buddyAnchor ) const
	{
		return ui::AlignRectOutside( rCtrlRect, buddyAnchor, m_alignment, m_spacing );		// tile align to buddy: layout the rect outside the anchor (by the anchor)
	}

	CRect& CBuddyLayout::ShrinkBuddyRect( CRect& rBuddyRect, const CSize& anchorCtrlSize ) const
	{
		switch ( m_alignment & HorizontalMask )
		{
			case H_AlignLeft:
				rBuddyRect.left += anchorCtrlSize.cx + m_spacing.cx;
				break;
			case H_AlignRight:
				rBuddyRect.right -= anchorCtrlSize.cx + m_spacing.cx;
				break;
		}

		switch ( m_alignment & VerticalMask )
		{
			case V_AlignTop:
				rBuddyRect.top += anchorCtrlSize.cy + m_spacing.cy;
				break;
			case V_AlignBottom:
				rBuddyRect.bottom -= anchorCtrlSize.cy + m_spacing.cy;
				break;
		}
		return rBuddyRect;
	}

	CRect CBuddyLayout::LayoutCtrl( CWnd* pCtrl, const CWnd* pBuddyCtrl, const CSize* pCustomSize /*= NULL*/ ) const
	{
		const CRect buddyRect = ui::GetControlRect( pBuddyCtrl->GetSafeHwnd() );
		CRect ctrlRect = ui::GetControlRect( pCtrl->GetSafeHwnd() );

		if ( pCustomSize != NULL )
			ui::SetRectSize( ctrlRect, *pCustomSize );

		Align( ctrlRect, buddyRect );
		pCtrl->MoveWindow( &ctrlRect );
		return ctrlRect;
	}

	void CBuddyLayout::ShrinkBuddy( CWnd* pBuddyCtrl, const CWnd* pCtrl ) const
	{
		const CSize ctrlSize = ui::GetControlRect( pCtrl->GetSafeHwnd() ).Size();
		CRect buddyRect = ui::GetControlRect( pBuddyCtrl->GetSafeHwnd() );

		ShrinkBuddyRect( buddyRect, ctrlSize );
		pBuddyCtrl->MoveWindow( &buddyRect );
	}
}
