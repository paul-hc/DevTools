#ifndef FrameHostCtrl_hxx
#define FrameHostCtrl_hxx

#include "WndUtils.h"


// CFrameHostCtrl template code

template< typename BaseCtrl >
void CFrameHostCtrl<BaseCtrl>::Refresh( void )
{
	if ( m_hWnd != nullptr )
	{
		InvalidateFrame( SolidFrame );

		if ( m_showFocus )
			InvalidateFrame( FocusFrame );
	}
}

template< typename BaseCtrl >
bool CFrameHostCtrl<BaseCtrl>::SetFrameColor( COLORREF frameColor )
{
	if ( !utl::ModifyValue( m_frameColor, frameColor ) )
		return false;

	m_frameColor = frameColor;
	Refresh();
	return true;
}

template< typename BaseCtrl >
bool CFrameHostCtrl<BaseCtrl>::SetShowFocus( bool showFocus /*= true*/ )
{
	if ( m_showFocus == showFocus )
		return false;

	m_showFocus = showFocus;
	Refresh();
	return true;
}

template< typename BaseCtrl >
CRect CFrameHostCtrl<BaseCtrl>::GetFrameRect( FrameType frameType ) const
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
CWnd* CFrameHostCtrl<BaseCtrl>::InvalidateFrame( FrameType frameType )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	CRect frameRect = GetFrameRect( frameType );
	CWnd* pRedrawWnd = this;

	if ( !ui::InBounds( clientRect, frameRect ) )		// need smarties?
	{	// frame is outside of client bounds: invalidate the parent dialog in its own client coordinates
		pRedrawWnd = GetParent();
		MapWindowPoints( pRedrawWnd, &frameRect );
	}

	CRgn frameRgn;
	frameRgn.CreateRectRgnIndirect( &frameRect );

	frameRect.DeflateRect( 1, 1 );
	ui::CombineWithRegion( &frameRgn, frameRect, RGN_DIFF );

	pRedrawWnd->InvalidateRgn( &frameRgn );
	return pRedrawWnd;
}

BEGIN_TEMPLATE_MESSAGE_MAP( CFrameHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CFrameHostCtrl<BaseCtrl>::OnPaint( void )
{
	__super::OnPaint();

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
void CFrameHostCtrl<BaseCtrl>::OnSetFocus( CWnd* pOldWnd )
{
	__super::OnSetFocus( pOldWnd );

	if ( m_showFocus )
		Invalidate();
}

template< typename BaseCtrl >
void CFrameHostCtrl<BaseCtrl>::OnKillFocus( CWnd* pNewWnd )
{
	__super::OnKillFocus( pNewWnd );

	if ( m_showFocus )
		Invalidate();
}


#endif // FrameHostCtrl_hxx
