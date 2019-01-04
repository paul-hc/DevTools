
#include "stdafx.h"
#include "AutoScrollDropTarget.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAutoScrollDropTarget::CAutoScrollDropTarget( void )
	: COleDropTarget()
{
}

CAutoScrollDropTarget::~CAutoScrollDropTarget()
{
}

DROPEFFECT CAutoScrollDropTarget::OnDragScroll( CWnd* pWnd, DWORD dwKeyState, CPoint point )
{
	// VERBATIM: COleDropTarget::OnDragScroll() implementation, with scrollbar hit testing
	//
	ASSERT_VALID(this);
	ASSERT_VALID(pWnd);

	// CWnd-s are allowed, but don't support autoscrolling
	if (!pWnd->IsKindOf(RUNTIME_CLASS(CView)))
		return DROPEFFECT_NONE;
	CView* pView = (CView*)pWnd;
	DROPEFFECT dropEffect = pView->OnDragScroll(dwKeyState, point);

	// DROPEFFECT_SCROLL means do the default
	if (dropEffect != DROPEFFECT_SCROLL)
		return dropEffect;

	// { My customization
	//

	// get client rectangle of destination window
	CRect rectClient, nonClientRect;

	pWnd->GetClientRect( &rectClient );
	pWnd->GetWindowRect( &nonClientRect );
	pWnd->ScreenToClient( &nonClientRect );

	CRect innerClientRect = rectClient;

	// hit-test against inset region
	UINT timerID = MAKEWORD(-1, -1);
	innerClientRect.InflateRect(-nScrollInset, -nScrollInset);
	CSplitterWnd* pSplitter = NULL;

	if ( !rectClient.PtInRect( point ) )
	{
		if ( nonClientRect.PtInRect( point ) )
		{
			CPoint screenPoint = point;

			pWnd->ClientToScreen( &screenPoint );
			switch ( pWnd->SendMessage( WM_NCHITTEST, 0l, MAKELPARAM( screenPoint.x, screenPoint.y ) ) )
			{
				case HTHSCROLL:
					timerID = computeScrollHitTest( point, nonClientRect, HorizontalBar );
					break;
				case HTVSCROLL:
					timerID = computeScrollHitTest( point, nonClientRect, VerticalBar );
					break;
			}
		}
	}
	else if ( !innerClientRect.PtInRect( point ) )
	{
		// determine which way to scroll along both X & Y axis
		if (point.x < innerClientRect.left)
			timerID = MAKEWORD(SB_LINEUP, HIBYTE(timerID));
		else if (point.x >= innerClientRect.right)
			timerID = MAKEWORD(SB_LINEDOWN, HIBYTE(timerID));
		if (point.y < innerClientRect.top)
			timerID = MAKEWORD(LOBYTE(timerID), SB_LINEUP);
		else if (point.y >= innerClientRect.bottom)
			timerID = MAKEWORD(LOBYTE(timerID), SB_LINEDOWN);
	}

	if ( timerID != MAKEWORD( -1, -1 ) )
	{
		// check for valid scroll first
		pSplitter = CView::GetParentSplitter(pView, FALSE);
		BOOL bEnableScroll = FALSE;
		if (pSplitter != NULL)
			bEnableScroll = pSplitter->DoScroll(pView, timerID, FALSE);
		else
			bEnableScroll = pView->OnScroll(timerID, 0, FALSE);
		if (!bEnableScroll)
			timerID = MAKEWORD(-1, -1);
	}

	//
	// } My customization

	if (timerID == MAKEWORD(-1, -1))
	{
		if (m_nTimerID != MAKEWORD(-1, -1))
		{
			// send fake OnDragEnter when transition from scroll->normal
			COleDataObject dataObject;
			dataObject.Attach(m_lpDataObject, FALSE);
			OnDragEnter(pWnd, &dataObject, dwKeyState, point);
			m_nTimerID = MAKEWORD(-1, -1);
		}
		return DROPEFFECT_NONE;
	}

	// save tick count when timer ID changes
	DWORD dwTick = GetTickCount();
	if (timerID != m_nTimerID)
	{
		m_dwLastTick = dwTick;
		m_nScrollDelay = nScrollDelay;
	}

	// scroll if necessary
	if (dwTick - m_dwLastTick > m_nScrollDelay)
	{
		if (pSplitter != NULL)
			pSplitter->DoScroll(pView, timerID, TRUE);
		else
			pView->OnScroll(timerID, 0, TRUE);
		m_dwLastTick = dwTick;
		m_nScrollDelay = nScrollInterval;
	}
	if (m_nTimerID == MAKEWORD(-1, -1))
	{
		// send fake OnDragLeave when transitioning from normal->scroll
		OnDragLeave(pWnd);
	}

	m_nTimerID = timerID;
	// check for force link
	if ((dwKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_LINK;
	// check for force copy
	else if ((dwKeyState & MK_CONTROL) == MK_CONTROL)
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_COPY;
	// check for force move
	else if ((dwKeyState & MK_ALT) == MK_ALT ||
		(dwKeyState & MK_SHIFT) == MK_SHIFT)
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
	// default -- recommended action is move
	else
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
	return dropEffect;
}

// Returns the scroll command associated with scrollbar hit test:
//		- left/right page scroll, or
//		- begin/end
UINT CAutoScrollDropTarget::computeScrollHitTest( const CPoint& point, const CRect& nonClientRect, ScrollBar scrollBar )
{
	static int scrollBarSize = GetSystemMetrics( SM_CYHSCROLL );
	CRect barRect = nonClientRect;

	if ( HorizontalBar == scrollBar )
	{
		barRect.top = barRect.bottom - scrollBarSize;

		if ( barRect.PtInRect( point ) )
		{	// hit the scrollbar
			int leftX = barRect.left + scrollBarSize, rightX = barRect.right - scrollBarSize;
			int midX = barRect.CenterPoint().x;

			if ( point.x < leftX )
				return MAKEWORD( SB_LEFT, -1 );
			else if ( point.x > rightX )
				return MAKEWORD( SB_RIGHT, -1 );
			else
				return MAKEWORD( point.x < midX ? SB_PAGELEFT : SB_PAGERIGHT, -1 );
		}
	}
	else
	{	// vertical scrollbar
		barRect.left = barRect.right - scrollBarSize;

		if ( barRect.PtInRect( point ) )
		{	// hit the scrollbar
			int topY = barRect.top + scrollBarSize, bottomY = barRect.bottom - scrollBarSize;
			int midY = barRect.CenterPoint().y;

			if ( point.y < topY )
				return MAKEWORD( -1, SB_TOP );
			else if ( point.y > bottomY )
				return MAKEWORD( -1, SB_BOTTOM );
			else
				return MAKEWORD( -1, point.y < midY ? SB_PAGEUP : SB_PAGEDOWN );
		}
	}

	return MAKEWORD( -1, -1 );
}
