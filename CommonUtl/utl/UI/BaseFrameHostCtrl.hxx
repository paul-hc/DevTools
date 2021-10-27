#ifndef BaseFrameHostCtrl_hxx
#define BaseFrameHostCtrl_hxx

#include "Utilities.h"


// CBaseFrameHostCtrl template code

template< typename BaseCtrl >
void CBaseFrameHostCtrl<BaseCtrl>::Refresh( void )
{
	if ( m_hWnd != NULL )
		Invalidate();
}

template< typename BaseCtrl >
bool CBaseFrameHostCtrl<BaseCtrl>::SetFrameColor( COLORREF frameColor )
{
	if ( m_frameColor == frameColor )
		return false;

	m_frameColor = frameColor;
	Refresh();
	return true;
}

template< typename BaseCtrl >
bool CBaseFrameHostCtrl<BaseCtrl>::SetShowFocus( bool showFocus /*= true*/ )
{
	if ( m_showFocus == showFocus )
		return false;

	m_showFocus = showFocus;
	Refresh();
	return true;
}

template< typename BaseCtrl >
CRect CBaseFrameHostCtrl<BaseCtrl>::GetFrameRect( FrameType frameType ) const
{
	CRect frameRect;
	GetClientRect( &frameRect );

	switch ( frameType )
	{
		case SolidFrame: frameRect.DeflateRect( m_frameMargins ); break;
		case FocusFrame: frameRect.DeflateRect( m_focusMargins ); break;
		default: ASSERT( false );
	}
	return frameRect;
}

template< typename BaseCtrl >
void CBaseFrameHostCtrl<BaseCtrl>::InvalidateFrame( FrameType frameType )
{
	CRect frameRect = GetFrameRect( frameType );
	CRgn frameRgn;
	frameRgn.CreateRectRgnIndirect( &frameRect );

	frameRect.DeflateRect( 1, 1 );
	ui::CombineWithRegion( &frameRgn, frameRect, RGN_DIFF );

	InvalidateRgn( &frameRgn );
}

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseFrameHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseFrameHostCtrl<BaseCtrl>::OnPaint( void )
{
	BaseCtrl::OnPaint();

	if ( m_frameColor != CLR_NONE || m_showFocus )
	{
		CClientDC dc( this );

		if ( m_frameColor != CLR_NONE )
		{
			CRect frameRect = GetFrameRect( SolidFrame );
			CBrush borderBrush( m_frameColor );
			dc.FrameRect( &frameRect, &borderBrush );
		}

		if ( m_showFocus && ui::OwnsFocus( m_hWnd ) )
		{
			CRect frameRect = GetFrameRect( FocusFrame );
			dc.DrawFocusRect( &frameRect );
		}
	}
}

template< typename BaseCtrl >
void CBaseFrameHostCtrl<BaseCtrl>::OnSetFocus( CWnd* pOldWnd )
{
	BaseCtrl::OnSetFocus( pOldWnd );

	if ( m_showFocus )
		Invalidate();
}

template< typename BaseCtrl >
void CBaseFrameHostCtrl<BaseCtrl>::OnKillFocus( CWnd* pNewWnd )
{
	BaseCtrl::OnKillFocus( pNewWnd );

	if ( m_showFocus )
		Invalidate();
}


#endif // BaseFrameHostCtrl_hxx
