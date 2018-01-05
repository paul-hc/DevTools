#ifndef AutoScrollDropTarget_h
#define AutoScrollDropTarget_h
#pragma once

#include <afxole.h>


class CAutoScrollDropTarget : public COleDropTarget
{
public:
	CAutoScrollDropTarget( void );
	virtual ~CAutoScrollDropTarget();

	// dragging overrides
	virtual DROPEFFECT OnDragScroll( CWnd* pWnd, DWORD dwKeyState, CPoint point );
protected:
	enum ScrollBar { HorizontalBar, VerticalBar };

	static UINT computeScrollHitTest( const CPoint& point, const CRect& nonClientRect, ScrollBar scrollBar );
};


#endif // AutoScrollDropTarget_h
