
#include "stdafx.h"
#include "DragListCtrl.h"
#include "GpUtilities.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDropMark implementation

CDropMark::CDropMark( const CListCtrl* pListCtrl, int dropIndex )
	: m_placement( dropIndex < pListCtrl->GetItemCount() ? BeforeItem : AfterItem )
	, m_orientation( GetOrientation( pListCtrl ) )
{
	ASSERT( dropIndex != -1 );

	GetListItemRect( pListCtrl, BeforeItem == m_placement ? dropIndex : ( dropIndex - 1 ), &m_markRect );

	if ( pListCtrl->GetItemCount() != 0 )
		if ( BeforeItem == m_placement )
			if ( HorizMark == m_orientation )
				m_markRect.bottom = m_markRect.top;				// fold to top
			else
				m_markRect.right = m_markRect.left;				// fold to left
		else
			if ( HorizMark == m_orientation )
				m_markRect.top = --m_markRect.bottom;			// fold to bottom
			else
				m_markRect.left = --m_markRect.right;			// fold to right

	if ( HorizMark == m_orientation )
		m_markRect.InflateRect( 0, PenWidth + ArrowExtent + 1 );
	else
		m_markRect.InflateRect( PenWidth + ArrowExtent + 1, 0 );

	CRect clientRect;
	pListCtrl->GetClientRect( &clientRect );
	ui::EnsureVisibleRect( m_markRect, clientRect );
}

void CDropMark::Draw( CDC* pDC )
{
	Pen pen( gp::MakeOpaqueColor( color::Red, 80 ), PenWidth );
	pen.SetAlignment( PenAlignmentCenter );

	GraphicsPath path( FillModeWinding );
	CPoint center = m_markRect.CenterPoint();
	Range<Point> core;						// the core line

	if ( HorizMark == m_orientation )
	{
		UINT edgeX = m_markRect.Width() * EdgePct / 100;
		edgeX += ArrowExtent;

		core.m_start.X = m_markRect.left + edgeX;
		core.m_end.X = m_markRect.right - edgeX;
		core.m_start.Y = core.m_end.Y = center.y;

		CRect bounds( core.m_start.X, core.m_start.Y, core.m_end.X, core.m_end.Y );
		bounds.InflateRect( ArrowExtent, ArrowExtent );

		path.StartFigure();			// LEFT inverted arrow
		path.AddLine( bounds.left, bounds.top, core.m_start.X, core.m_start.Y );
		path.AddLine( bounds.left, bounds.bottom, core.m_start.X, core.m_start.Y );

		path.StartFigure();			// CORE line
		path.AddLine( core.m_start, core.m_end );

		path.StartFigure();			// RIGHT inverted arrow
		path.AddLine( bounds.right, bounds.top, core.m_end.X, core.m_end.Y );
		path.AddLine( bounds.right, bounds.bottom, core.m_end.X, core.m_end.Y );
	}
	else
	{
		UINT edgeY = m_markRect.Height() * EdgePct / 100;
		edgeY += ArrowExtent;
		//edgeY = std::max( , edgeY );
		core.m_start.Y = m_markRect.top + edgeY;
		core.m_end.Y = m_markRect.bottom - edgeY;
		core.m_start.X = core.m_end.X = center.x;

		CRect bounds( core.m_start.X, core.m_start.Y, core.m_end.X, core.m_end.Y );
		bounds.InflateRect( ArrowExtent, ArrowExtent );

		path.StartFigure();			// TOP inverted arrow
		path.AddLine( bounds.left, bounds.top, core.m_start.X, core.m_start.Y );
		path.AddLine( bounds.right, bounds.top, core.m_start.X, core.m_start.Y );

		path.StartFigure();			// CORE line
		path.AddLine( core.m_start, core.m_end );

		path.StartFigure();			// BOTTOM inverted arrow
		path.AddLine( bounds.left, bounds.bottom, core.m_end.X, core.m_end.Y );
		path.AddLine( bounds.right, bounds.bottom, core.m_end.X, core.m_end.Y );
	}

	Graphics graphics( pDC->GetSafeHdc() );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	graphics.DrawPath( &pen, &path );
}

void CDropMark::DrawLine( CDC* pDC )
{
	Pen pen( gp::MakeOpaqueColor( color::Red, 80 ), PenWidth );
	pen.SetAlignment( PenAlignmentCenter );
	pen.SetStartCap( LineCapArrowAnchor );
	pen.SetEndCap( LineCapArrowAnchor );

	CPoint center = m_markRect.CenterPoint();

	Range<Point> line;
	if ( HorizMark == m_orientation )
	{
		UINT edgeX = m_markRect.Width() * EdgePct / 100;
		line.m_start.X = m_markRect.left + edgeX;
		line.m_end.X = m_markRect.right - edgeX;
		line.m_start.Y = line.m_end.Y = center.y;
	}
	else
	{
		UINT edgeY = m_markRect.Height() * EdgePct / 100;
		line.m_start.Y = m_markRect.top + edgeY;
		line.m_end.Y = m_markRect.bottom - edgeY;
		line.m_start.X = line.m_end.X = center.x;
	}

	Graphics graphics( pDC->GetSafeHdc() );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	graphics.DrawLine( &pen, line.m_start, line.m_end );
}

void CDropMark::Invalidate( CListCtrl* pListCtrl )
{
	CRect bounds = m_markRect;
	bounds.InflateRect( 2, 2 );
	pListCtrl->InvalidateRect( &bounds );
}

CDropMark::Orientation CDropMark::GetOrientation( const CListCtrl* pListCtrl )
{
	bool stacksVertically = EqFlag( pListCtrl->GetStyle(), LVS_ALIGNLEFT );
	switch ( pListCtrl->GetView() )
	{
		default: ASSERT( false );
		case LV_VIEW_LIST:
		case LV_VIEW_DETAILS:
			return HorizMark;		// always arranged top-to-bottom
		case LV_VIEW_ICON:
		case LV_VIEW_SMALLICON:
		case LV_VIEW_TILE:
			return stacksVertically ? HorizMark : VertMark;
	}
}

bool CDropMark::GetListItemRect( const CListCtrl* pListCtrl, int index, CRect* pRect ) const
{
	if ( pListCtrl->GetItemRect( index, pRect, LVIR_BOUNDS ) )
		return true;

	// empty list: simulate top item
	pListCtrl->GetClientRect( pRect );

	if ( HWND hHeaderWnd = pListCtrl->GetHeaderCtrl()->GetSafeHwnd() )
		pRect->top += ui::GetControlRect( hHeaderWnd ).Height();

	pRect->bottom = pRect->top + 16;
	return false;
}
