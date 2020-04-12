
#include "stdafx.h"
#include "BaseSplitButton.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaseSplitButton::CBaseSplitButton( void )
	: CIconButton()
	, m_splitState( Normal )
{
}

CBaseSplitButton::~CBaseSplitButton()
{
}

bool CBaseSplitButton::SetSplitState( SplitState splitState )
{
	if ( m_splitState == splitState )
		return false;

	m_splitState = splitState;
	RedrawRhsPart();
	return true;
}

void CBaseSplitButton::DrawRhsPart( CDC* pDC, const CRect& clientRect )
{
	CRect rhsRect = GetRhsPartRect( &clientRect );
	CRect sepRect( 0, rhsRect.top, 1, rhsRect.bottom );
	ui::AlignRect( sepRect, rhsRect, H_AlignLeft );
	sepRect.DeflateRect( 0, 3 );
	pDC->DrawEdge( &sepRect, BDR_RAISED, BF_LEFT | BF_MONO );
}

void CBaseSplitButton::DrawFocus( CDC* pDC, const CRect& clientRect )
{	// draw focus on left-hand-side
	if ( GetSplitState() != RhsPressed )		// draw focus on right-hand-side
	{
		CRect focusRect = clientRect;
		focusRect.DeflateRect( DefaultBorderSpacing, DefaultBorderSpacing );

		focusRect.right = GetRhsPartRect( &clientRect ).left + 1;
		focusRect.DeflateRect( FocusSpacing, FocusSpacing );
		pDC->DrawFocusRect( &focusRect );
	}
}

void CBaseSplitButton::RedrawRhsPart( void )
{
	CClientDC dc( this );
	CRect clientRect;
	GetClientRect( &clientRect );
	if ( HasRhsPart() )
		DrawRhsPart( &dc, clientRect );
}

void CBaseSplitButton::DrawItem( DRAWITEMSTRUCT* pDrawItem )
{
	__super::DrawItem( pDrawItem );
	if ( HasRhsPart() )
		DrawRhsPart( CDC::FromHandle( pDrawItem->hDC ), pDrawItem->rcItem );
}

void CBaseSplitButton::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();
	ModifyStyle( 0, BS_LEFT );
}


// message handlers

BEGIN_MESSAGE_MAP( CBaseSplitButton, CIconButton )
	ON_WM_MOUSEMOVE()
	ON_MESSAGE( WM_MOUSELEAVE, OnMouseLeave )
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CBaseSplitButton::OnMouseMove( UINT flags, CPoint point )
{
	__super::OnMouseMove( flags, point );

	if ( m_splitState != RhsPressed )
	{
		SetSplitState( GetRhsPartRect().PtInRect( point ) ? HotRhs : HotButton );

		TRACKMOUSEEVENT trackInfo = { sizeof( TRACKMOUSEEVENT ), TME_LEAVE, m_hWnd, HOVER_DEFAULT };
		::_TrackMouseEvent( &trackInfo );
	}
}

LRESULT CBaseSplitButton::OnMouseLeave( WPARAM, LPARAM )
{
	if ( m_splitState != RhsPressed )
		SetSplitState( Normal );
	return Default();				// process default button animations
}

void CBaseSplitButton::OnPaint( void )
{
	__super::OnPaint();

	if ( HasRhsPart() && !IsOwnerDraw() )
	{
		CRect clientRect;
		GetClientRect( &clientRect );
		CRect focusRect = clientRect; focusRect.DeflateRect( DefaultBorderSpacing + FocusSpacing, DefaultBorderSpacing + FocusSpacing );

		CClientDC dc( this );
		bool focused = m_hWnd == ::GetFocus();
		if ( focused )
			dc.DrawFocusRect( &focusRect );		// clear original focus

		DrawRhsPart( &dc, clientRect );
		if ( focused )
			DrawFocus( &dc, clientRect );		// redraw the shrunk focus frame
	}
}
