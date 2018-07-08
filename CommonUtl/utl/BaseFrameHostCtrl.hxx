#ifndef BaseFrameHostCtrl_hxx
#define BaseFrameHostCtrl_hxx

#include "Utilities.h"


// CBaseFrameHostCtrl template code

template< typename BaseCtrl >
bool CBaseFrameHostCtrl< BaseCtrl >::SetFrameColor( COLORREF frameColor )
{
	if ( m_frameColor == frameColor )
		return false;

	m_frameColor = frameColor;

	if ( m_hWnd != NULL )
		Invalidate();
	return true;
}

template< typename BaseCtrl >
bool CBaseFrameHostCtrl< BaseCtrl >::SetShowFocus( bool showFocus /*= true*/ )
{
	if ( m_showFocus == showFocus )
		return false;

	m_showFocus = showFocus;

	if ( m_hWnd != NULL )
		Invalidate();
	return true;
}

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseFrameHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseFrameHostCtrl< BaseCtrl >::OnPaint( void )
{
	BaseCtrl::OnPaint();

	if ( m_frameColor != CLR_NONE || m_showFocus )
	{
		CClientDC dc( this );
		CRect clientRect;
		GetClientRect( &clientRect );

		if ( m_frameColor != CLR_NONE )
		{
			CRect rect = clientRect;
			rect.DeflateRect( m_frameMargins );

			CBrush borderBrush( m_frameColor );
			dc.FrameRect( &rect, &borderBrush );
		}

		if ( m_showFocus && ui::OwnsFocus( m_hWnd ) )
		{
			CRect rect = clientRect;
			rect.DeflateRect( m_focusMargins );

			dc.DrawFocusRect( &rect );
		}
	}
}

template< typename BaseCtrl >
void CBaseFrameHostCtrl< BaseCtrl >::OnSetFocus( CWnd* pOldWnd )
{
	BaseCtrl::OnSetFocus( pOldWnd );

	if ( m_showFocus )
		Invalidate();
}

template< typename BaseCtrl >
void CBaseFrameHostCtrl< BaseCtrl >::OnKillFocus( CWnd* pNewWnd )
{
	BaseCtrl::OnKillFocus( pNewWnd );

	if ( m_showFocus )
		Invalidate();
}


#endif // BaseFrameHostCtrl_hxx
