#ifndef SampleView_h
#define SampleView_h
#pragma once

#include "Range.h"
#include "ui_fwd.h"


interface ISampleCallback
{
	virtual void RenderBackground( CDC* pDC, const CRect& clientRect ) { pDC, clientRect; }
	virtual bool RenderSample( CDC* pDC, const CRect& clientRect ) = 0;
	virtual void ShowPixelInfo( const CPoint& pos, COLORREF color ) { pos, color; }
};


// a scrollable view used as control in a dialog

class CSampleView : public CScrollView
{
public:
	CSampleView( ISampleCallback* pSampleCallback );
	virtual ~CSampleView();

	void DDX_Placeholder( CDataExchange* pDX, int placeholderId );		// create and replace a placeholder static (with the same id)

	void SetSampleCallback( ISampleCallback* pSampleCallback ) { m_pSampleCallback = pSampleCallback; SafeRedraw(); }

	enum ContentUnits { Logical, Device };								// device units are compatible with client rect
	CSize GetContentSize( ContentUnits units = Device ) const { return Device == units ? m_totalDev : m_totalLog; }
	void SetContentSize( const CSize& contentSize );

	bool IsScrollable( DWORD scrollStyle = WS_HSCROLL | WS_VSCROLL ) const { return HasFlag( GetStyle(), scrollStyle ); };
	void SetNotScrollable( void ) { SetContentSize( CSize( 0, 0 ) ); }

	template< typename T > Range<T> GetScrollRangeAs( int bar ) const;

	CRect MakeDisplayRect( const CRect& clientRect, const CSize& displaySize ) const;

	void SafeRedraw( void );
	bool DrawContentFrame( CDC* pDC, const CRect& contentRect, COLORREF scrollableColor, BYTE alpha = 100 );
	void DrawError( CDC* pDC, const CRect& rect );
	void DrawCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha = 127 );
	void DrawDiagonalCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha = 127 );
private:
	void RunTrackScroll( CPoint point );
	bool TrackingScroll( const CPoint& mouseAnchor, const CPoint& scrollAnchor );
private:
	ISampleCallback* m_pSampleCallback;
	HCURSOR m_hScrollableCursor, m_hScrollDragCursor;

	// generated overrides
	protected:
	virtual void PostNcDestroy( void );
	virtual void OnDraw( CDC* pDC );
	public:
	virtual void OnInitialUpdate( void );
protected:
	// message map
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg BOOL OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message );
	afx_msg void OnLButtonDown( UINT flags, CPoint point );
	afx_msg void OnMouseMove( UINT flags, CPoint point );
	int OnMouseActivate( CWnd* pDesktopWnd, UINT hitTest, UINT message );

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename T >
Range<T> CSampleView::GetScrollRangeAs( int bar ) const
{
	int minPos, maxPos;
	GetScrollRange( bar, &minPos, &maxPos );
	return Range<T>( minPos, maxPos );
}


#endif // SampleView_h
