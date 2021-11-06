
#include "StdAfx.h"
#include "OleDragDrop_fwd.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//#define TRACE_DD TRACE
#define TRACE_DD __noop


namespace ole
{
	static size_t s_traceCount = 0;


	HCURSOR GetDropEffectCursor( DROPEFFECT dropEffect )
	{
		switch ( dropEffect )
		{
			case DROPEFFECT_NONE:
			{
				static HCURSOR hNoneCursor = AfxGetApp()->LoadCursor( IDR_DROP_NONE_CURSOR );
				return hNoneCursor;
			}
			case DROPEFFECT_COPY:
			{
				static HCURSOR hCopyCursor = AfxGetApp()->LoadCursor( IDR_DROP_COPY_CURSOR );
				return hCopyCursor;
			}
			case DROPEFFECT_MOVE:
			{
				static HCURSOR hMoveCursor = AfxGetApp()->LoadCursor( IDR_DROP_MOVE_CURSOR );
				return hMoveCursor;
			}
		}
		return NULL;
	}


	// IDropTargetEventsStub implementation

	DROPEFFECT IDropTargetEventsStub::Event_OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		pDataObject, keyState, point;
		return DROPEFFECT_NONE;
	}

	DROPEFFECT IDropTargetEventsStub::Event_OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		pDataObject, keyState, point;
		return DROPEFFECT_NONE;
	}

	bool IDropTargetEventsStub::Event_OnDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
	{
		pDataObject, dropEffect, point;
		return false;
	}

	DROPEFFECT IDropTargetEventsStub::Event_OnDropEx( COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point )
	{
		pDataObject, dropDefault, dropList, point;
		return DROPEFFECT_NOT_IMPL;
	}

	DROPEFFECT IDropTargetEventsStub::Event_OnDragScroll( DWORD keyState, CPoint point )
	{
		keyState, point;
		return DROPEFFECT_SCROLL;		// this means do the default scrolling
	}

	void IDropTargetEventsStub::Event_OnDragLeave( void )
	{
	}

	void IDropTargetEventsStub::Event_DoScroll( ole::ScrollDir horzDir, ole::ScrollDir vertDir )
	{
		horzDir, vertDir;
	}


	// C_DropTargetAdapter implementation

	C_DropTargetAdapter::C_DropTargetAdapter( void )
		: COleDropTarget()
	{
	}

	C_DropTargetAdapter::~C_DropTargetAdapter()
	{
	}

	DROPEFFECT C_DropTargetAdapter::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		s_traceCount = 0;
		TRACE_DD( _T(">DD(%d) OnDragEnter: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		if ( IDropTargetEventsStub* pEventsWnd = dynamic_cast<IDropTargetEventsStub*>( pWnd ) )
			return pEventsWnd->Event_OnDragEnter( pDataObject, keyState, point );

		return COleDropTarget::OnDragEnter( pWnd, pDataObject, keyState, point );
	}

	DROPEFFECT C_DropTargetAdapter::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		TRACE_DD( _T(">DD(%d) OnDragOver: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		if ( IDropTargetEventsStub* pEventsWnd = dynamic_cast<IDropTargetEventsStub*>( pWnd ) )
			return pEventsWnd->Event_OnDragOver( pDataObject, keyState, point );

		return COleDropTarget::OnDragOver( pWnd, pDataObject, keyState, point );
	}

	BOOL C_DropTargetAdapter::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
	{
		TRACE_DD( _T(">DD(%d) OnDrop: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		if ( IDropTargetEventsStub* pEventsWnd = dynamic_cast<IDropTargetEventsStub*>( pWnd ) )
			return pEventsWnd->Event_OnDrop( pDataObject, dropEffect, point );

		return COleDropTarget::OnDrop( pWnd, pDataObject, dropEffect, point );
	}

	DROPEFFECT C_DropTargetAdapter::OnDropEx( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point )
	{
		TRACE_DD( _T(">DD(%d) OnDropEx: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		if ( IDropTargetEventsStub* pEventsWnd = dynamic_cast<IDropTargetEventsStub*>( pWnd ) )
			return pEventsWnd->Event_OnDropEx( pDataObject, dropDefault, dropList, point );

		return COleDropTarget::OnDropEx( pWnd, pDataObject, dropDefault, dropList, point );
	}

	void C_DropTargetAdapter::OnDragLeave( CWnd* pWnd )
	{
		TRACE_DD( _T(">DD(%d) OnDropLeave: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		if ( IDropTargetEventsStub* pEventsWnd = dynamic_cast<IDropTargetEventsStub*>( pWnd ) )
			pEventsWnd->Event_OnDragLeave();
		else
			COleDropTarget::OnDragLeave( pWnd );
	}

	DROPEFFECT C_DropTargetAdapter::OnDragScroll( CWnd* pWnd, DWORD keyState, CPoint point )
	{
		TRACE_DD( _T(">DD(%d) OnDragScroll: %s\n"), s_traceCount++, str::mfc::GetTypeName( pWnd ).GetString() );

		// VERBATIM: COleDropTarget::OnDragScroll() implementation, with scrollbar hit testing
		//
		ASSERT_VALID(this);
		ASSERT_VALID(pWnd);

		// CWnd-s are allowed, but don't support autoscrolling
		if ( !pWnd->IsKindOf( RUNTIME_CLASS( CView ) ) )
			return DROPEFFECT_NONE;

		CView* pView = (CView*)pWnd;
		DROPEFFECT dropEffect = pView->OnDragScroll(keyState, point);

		// DROPEFFECT_SCROLL means do the default
		if (dropEffect != DROPEFFECT_SCROLL)
			return dropEffect;

		// PaulC: { my customization
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
						timerID = ScrollHitTest( point, nonClientRect, HorizontalBar );
						break;
					case HTVSCROLL:
						timerID = ScrollHitTest( point, nonClientRect, VerticalBar );
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
		// PaulC: } My customization

		if (timerID == MAKEWORD(-1, -1))
		{
			if (m_nTimerID != MAKEWORD(-1, -1))
			{
				// send fake OnDragEnter when transition from scroll->normal
				COleDataObject dataObject;
				dataObject.Attach(m_lpDataObject, FALSE);
				OnDragEnter(pWnd, &dataObject, keyState, point);
				m_nTimerID = MAKEWORD(-1, -1);
			}
			return DROPEFFECT_NONE;
		}

		// save tick count when timer ID changes
		DWORD tickCount = GetTickCount();
		if (timerID != m_nTimerID)
		{
			m_dwLastTick = tickCount;
			m_nScrollDelay = nScrollDelay;
		}

		// scroll if necessary
		if (tickCount - m_dwLastTick > m_nScrollDelay)
		{
			if (pSplitter != NULL)
				pSplitter->DoScroll(pView, timerID, TRUE);
			else
				pView->OnScroll(timerID, 0, TRUE);
			m_dwLastTick = tickCount;
			m_nScrollDelay = nScrollInterval;
		}
		if (m_nTimerID == MAKEWORD(-1, -1))
		{
			// send fake OnDragLeave when transitioning from normal->scroll
			OnDragLeave(pWnd);
		}

		m_nTimerID = timerID;
		// check for force link
		if ((keyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
			dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_LINK;
		// check for force copy
		else if ((keyState & MK_CONTROL) == MK_CONTROL)
			dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_COPY;
		// check for force move
		else if ((keyState & MK_ALT) == MK_ALT ||
			(keyState & MK_SHIFT) == MK_SHIFT)
			dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
		// default -- recommended action is move
		else
			dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
		return dropEffect;
	}

	// Returns the scroll command associated with scrollbar hit test:
	//		- left/right page scroll, or
	//		- begin/end
	UINT C_DropTargetAdapter::ScrollHitTest( const CPoint& point, const CRect& nonClientRect, ScrollBar scrollBar )
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

} //namespace ole
