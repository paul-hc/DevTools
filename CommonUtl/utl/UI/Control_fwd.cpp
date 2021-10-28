
#include "stdafx.h"
#include "Control_fwd.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CTandemLayout implementation

	const CTandemLayout CTandemLayout::s_mateOnRight( H_AlignRight | V_AlignCenter, Spacing );

	void CTandemLayout::AlignTandem( CRect& rHostRect, CRect& rMateRect, const CSize* pMateCustomSize /*= NULL*/ ) const
	{
		if ( pMateCustomSize != NULL )
			ui::SetRectSize( rMateRect, *pMateCustomSize );		// resize mate to ideal size

		ui::AlignRect( rMateRect, rHostRect, m_alignment );
		ShrinkHostRect( rHostRect, rMateRect.Size() );
	}

	void CTandemLayout::AlignOutside( CRect& rMateRect, const RECT& hostRect ) const
	{
		ui::AlignRectOutside( rMateRect, hostRect, m_alignment, m_spacing );		// tile align to host: layout the rect outside the anchor (by the anchor)
	}

	void CTandemLayout::ShrinkHostRect( CRect& rHostRect, const CSize& mateSize ) const
	{
		switch ( m_alignment & HorizontalMask )
		{
			case H_AlignLeft:
				rHostRect.left += mateSize.cx + m_spacing.cx;
				break;
			case H_AlignRight:
				rHostRect.right -= mateSize.cx + m_spacing.cx;
				break;
		}

		switch ( m_alignment & VerticalMask )
		{
			case V_AlignTop:
				rHostRect.top += mateSize.cy + m_spacing.cy;
				break;
			case V_AlignBottom:
				rHostRect.bottom -= mateSize.cy + m_spacing.cy;
				break;
		}
	}

	CRect CTandemLayout::LayoutMate( CWnd* pMateCtrl, const CWnd* pHostCtrl, const CSize* pCustomSize /*= NULL*/ ) const
	{
		const CRect hostRect = ui::GetControlRect( pHostCtrl->GetSafeHwnd() );
		CRect ctrlRect = ui::GetControlRect( pMateCtrl->GetSafeHwnd() );

		if ( pCustomSize != NULL )
			ui::SetRectSize( ctrlRect, *pCustomSize );

		ui::AlignRect( ctrlRect, hostRect, m_alignment );
		AlignOutside( ctrlRect, hostRect );
		pMateCtrl->MoveWindow( &ctrlRect );
		return ctrlRect;
	}

	void CTandemLayout::LayoutTandem( CWnd* pHostCtrl, CWnd* pMateCtrl, const CSize* pMateCustomSize /*= NULL*/ ) const
	{
		CRect hostRect = ui::GetControlRect( pHostCtrl->GetSafeHwnd() );
		CRect mateRect = ui::GetControlRect( pMateCtrl->GetSafeHwnd() );

		AlignTandem( hostRect, mateRect, pMateCustomSize );

		pHostCtrl->MoveWindow( &hostRect );
		pMateCtrl->MoveWindow( &mateRect );
	}
}
