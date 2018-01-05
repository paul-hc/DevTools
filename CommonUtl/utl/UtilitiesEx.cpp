
#include "stdafx.h"
#include "UtilitiesEx.h"
#include "ScopedGdi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	void DrawBorder( CDC* pDC, const CRect& rect, COLORREF borderColor, int borderWidth /*= 1*/ )
	{
		CPen pen( PS_SOLID | PS_INSIDEFRAME, borderWidth, borderColor );
		CScopedGdi< CPen > scopedPen( pDC, &pen );
		CScopedGdi< CGdiObject > scopedBrush( pDC, HOLLOW_BRUSH );

		pDC->Rectangle( &rect );
	}

	int MakeBorderRegion( CRgn* pBorderRegion, const CRect& rect, int borderWidth /*= 1*/ )
	{
		CRect innerRect = rect;
		innerRect.DeflateRect( borderWidth, borderWidth );
		innerRect &= rect;
		return CombineRects( pBorderRegion, rect, innerRect, RGN_DIFF );
	}
}


// CScopedLockRedraw implementation

CScopedLockRedraw::CScopedLockRedraw( CWnd* pWnd, CScopedWindowBorder* pBorder /*= NULL*/, bool doRedraw /*= true*/ )
	: m_pWnd( pWnd )
	, m_doRedraw( doRedraw )
	, m_pBorder( pBorder )
{
	m_pWnd->SetRedraw( FALSE );
}

CScopedLockRedraw::~CScopedLockRedraw()
{
	m_pBorder.reset();

	m_pWnd->SetRedraw( TRUE );

	if ( m_doRedraw )
	{
		m_pWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );		// redraw scrollbars
		m_pWnd->Invalidate();
	}
}


// CNonClientDraw implementation

CNonClientDraw::CNonClientDraw( CWnd* pWnd, INonClientRender* pCallback /*= NULL*/ )
	: m_pWnd( pWnd )
	, m_pCallback( pCallback != NULL ? pCallback : dynamic_cast< INonClientRender* >( m_pWnd ) )
{
	ASSERT_PTR( m_pWnd->GetSafeHwnd() );
	HookWindow( *m_pWnd );
}

CNonClientDraw::~CNonClientDraw()
{
	if ( IsHooked() )
		UnhookWindow();
}

LRESULT CNonClientDraw::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	LRESULT result = CWindowHook::WindowProc( message, wParam, lParam );
	if ( WM_NCPAINT == message && m_pCallback != NULL )
	{
		CWindowDC dc( m_pWnd );
		m_pCallback->DrawNonClient( &dc, ui::GetControlRect( *m_pWnd ) );		// in parent's coords
	}
	return result;
}


// CScopedWindowBorder implementation

CScopedWindowBorder::CScopedWindowBorder( CWnd* pWnd, COLORREF borderColor, int borderWidth /*= 1*/ )
	: m_pWnd( pWnd )
	, m_borderColor( borderColor )
	, m_borderWidth( borderWidth )
	, m_drawHook( m_pWnd, this )
{
	RedrawBorder();		// redraw non-client border
}

CScopedWindowBorder::~CScopedWindowBorder()
{
	Release();
}

void CScopedWindowBorder::Release( void )
{
	if ( m_drawHook.IsHooked() )
	{
		m_drawHook.UnhookWindow();
		m_drawHook.RedrawNonClient();
	}
}

void CScopedWindowBorder::DrawNonClient( CDC* pDC, const CRect& ctrlRect )
{
	ui::DrawBorder( pDC, ctrlRect, m_borderColor, m_borderWidth );
}

void CScopedWindowBorder::RedrawBorder( void )
{
	CRgn borderRegion;
	ui::MakeBorderRegion( &borderRegion, ui::GetControlRect( *m_pWnd ), m_borderWidth );

	// according to documentation should use RDW_INVALIDATE, but that interferes with client region, which is undesirable for tree control;
	// seems to work fine with just RDW_FRAME
	m_pWnd->RedrawWindow( NULL, &borderRegion, RDW_FRAME );		// redraw non-client border
}


// CFlashCtrlFrame implementation

void CFlashCtrlFrame::OnSequenceStep( void )
{
	HWND hCtrl = GetHwnd();
	if ( !::IsWindow( hCtrl ) )
		return;

	CWnd* pParentWnd = CWnd::FromHandle( ::GetParent( hCtrl ) );
	CRect innerRect = ui::GetControlRect( hCtrl );						// in parent's client coords
	innerRect.InflateRect( Spacing, Spacing );

	CRect outerRect = innerRect;
	outerRect.InflateRect( FrameWidth, FrameWidth );

	CRgn region;
	region.CreateRectRgnIndirect( &outerRect );
	ui::CombineWithRegion( &region, innerRect, RGN_DIFF );

	m_frameOn = !m_frameOn;
	if ( m_frameOn )			// was just flipped
	{
		CClientDC dc( pParentWnd );
		dc.FillRgn( &region, &m_frameBrush );
	}
	else
	{
		pParentWnd->InvalidateRgn( &region );
		pParentWnd->UpdateWindow();
	}
}
