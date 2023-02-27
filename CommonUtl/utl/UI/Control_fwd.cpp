
#include "stdafx.h"
#include "Control_fwd.h"
#include "WndUtils.h"
#include "utl/ScopedValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CTandemLayout implementation

	const CTandemLayout CTandemLayout::s_mateOnRight( ui::EditShinkHost_MateOnRight, Spacing );

	bool CTandemLayout::IsValidAlignment( void ) const
	{
		if ( HasFlag( m_tandemAlign, ui::H_ShrinkHost ) && HasFlag( m_tandemAlign, ui::H_TileMate ) )
			return false;	// exclusive flags, should be either SHRINK or TILE

		if ( HasFlag( m_tandemAlign, ui::V_ShrinkHost ) && HasFlag( m_tandemAlign, ui::V_TileMate ) )
			return false;	// exclusive flags, should be either SHRINK or TILE

		if ( EqMaskedValue( m_tandemAlign, HorizontalMask, H_AlignCenter ) && HasFlag( m_tandemAlign, ui::H_ShrinkHost ) )
			return false;	// mate would be covered by the host

		if ( EqMaskedValue( m_tandemAlign, VerticalMask, V_AlignCenter ) && HasFlag( m_tandemAlign, ui::V_ShrinkHost ) )
			return false;	// mate would be covered by the host

		return ui::IsValidAlignment( m_tandemAlign );			// no standard alignment conflicts?
	}

	void CTandemLayout::AlignTandem( CRect& rHostRect, CRect& rMateRect, const CSize* pMateCustomSize /*= nullptr*/ ) const
	{
		if ( pMateCustomSize != nullptr )
			ui::SetRectSize( rMateRect, *pMateCustomSize );		// resize mate to ideal size

		AlignMate( rMateRect, rHostRect, true );
		ShrinkHostRect( rHostRect, rMateRect.Size() );
	}

	void CTandemLayout::AlignMate( CRect& rMateRect, const RECT& hostRect, bool inTandem ) const
	{	// align mate rect relative to the host rect
		CRect tileRect = hostRect;

		TTandemAlign custTandemAlign = m_tandemAlign;

		if ( !inTandem )
		{	// translate SHRINK flags to TILE flags if host does not get shrunk (i.e. layout operation)
			if ( HasFlag( m_tandemAlign, H_ShrinkHost ) )
				custTandemAlign |= H_TileMate;

			if ( HasFlag( m_tandemAlign, V_ShrinkHost ) )
				custTandemAlign |= V_TileMate;
		}

		if ( HasFlag( custTandemAlign, H_TileMate ) )
			tileRect.InflateRect( rMateRect.Width() + m_spacing.cx, 0 );

		if ( HasFlag( custTandemAlign, V_TileMate ) )
			tileRect.InflateRect( 0, rMateRect.Height() + m_spacing.cy );

		ui::AlignRect( rMateRect, tileRect, m_tandemAlign );
	}

	void CTandemLayout::ShrinkHostRect( CRect& rHostRect, const CSize& mateSize ) const
	{
		if ( HasFlag( m_tandemAlign, H_ShrinkHost ) )
			switch ( m_tandemAlign & HorizontalMask )
			{
				case H_AlignLeft:
					rHostRect.left += mateSize.cx + m_spacing.cx;
					break;
				case H_AlignRight:
					rHostRect.right -= mateSize.cx + m_spacing.cx;
					break;
			}

		if ( HasFlag( m_tandemAlign, V_ShrinkHost ) )
			switch ( m_tandemAlign & VerticalMask )
			{
				case V_AlignTop:
					rHostRect.top += mateSize.cy + m_spacing.cy;
					break;
				case V_AlignBottom:
					rHostRect.bottom -= mateSize.cy + m_spacing.cy;
					break;
			}
	}

	CRect CTandemLayout::LayoutMate( CWnd* pMateCtrl, const CWnd* pHostCtrl ) const
	{
		const CRect hostRect = ui::GetControlRect( pHostCtrl->GetSafeHwnd() );
		CRect mateRect = ui::GetControlRect( pMateCtrl->GetSafeHwnd() );

		AlignMate( mateRect, hostRect, false );
		pMateCtrl->MoveWindow( &mateRect );
		return mateRect;
	}

	void CTandemLayout::LayoutTandem( CWnd* pHostCtrl, CWnd* pMateCtrl, const CSize* pMateCustomSize /*= nullptr*/ ) const
	{
		CRect hostRect = ui::GetControlRect( pHostCtrl->GetSafeHwnd() );
		CRect mateRect = ui::GetControlRect( pMateCtrl->GetSafeHwnd() );

		AlignTandem( hostRect, mateRect, pMateCustomSize );

		pHostCtrl->MoveWindow( &hostRect );
		pMateCtrl->MoveWindow( &mateRect );
	}
}
