#ifndef SampleView_h
#define SampleView_h
#pragma once

#include "Range.h"
#include "ui_fwd.h"
#include "SampleView_fwd.h"


// a scrollable view used as control in a dialog

class CSampleView : public CScrollView
{
public:
	CSampleView( ui::ISampleCallback* pSampleCallback, bool useDoubleBuffering = true );
	virtual ~CSampleView();

	void DDX_Placeholder( CDataExchange* pDX, int placeholderId );		// create and replace a placeholder static (with the same id)

	void SetSampleCallback( ui::ISampleCallback* pSampleCallback ) { m_pSampleCallback = pSampleCallback; SafeRedraw(); }

	void SetUseDoubleBuffering( bool useDoubleBuffering = true ) { m_useDoubleBuffering = useDoubleBuffering; }
	void SetBorderColor( COLORREF borderColor = CLR_DEFAULT ) { m_borderColor = borderColor; SafeRedraw(); }
	void SetBkColor( COLORREF bkColor ) { m_bkColor = bkColor; SafeRedraw(); }

	enum ContentUnits { Logical, Device };								// device units are compatible with client rect
	CSize GetContentSize( ContentUnits units = Device ) const { return Device == units ? m_totalDev : m_totalLog; }
	void SetContentSize( const CSize& contentSize );

	bool IsScrollable( DWORD scrollStyle = WS_HSCROLL | WS_VSCROLL ) const { return HasFlag( GetStyle(), scrollStyle ); };
	void SetNotScrollable( void ) { SetContentSize( CSize( 0, 0 ) ); }

	template< typename T > Range<T> GetScrollRangeAs( int bar ) const;

	CRect MakeDisplayRect( const CRect& clientRect, const CSize& displaySize ) const;

	void SafeRedraw( void );
	bool DrawContentFrame( CDC* pDC, const CRect& contentRect, COLORREF scrollableColor = CLR_NONE, BYTE alpha = 100 );
	void DrawError( CDC* pDC, const CRect& rect );
	void DrawCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha = 127 );
	void DrawDiagonalCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha = 127 );

	enum { LightCyan = RGB( 204, 232, 255 ) };
private:
	void RunTrackScroll( CPoint point );
	bool TrackingScroll( const CPoint& mouseAnchor, const CPoint& scrollAnchor );

	void Draw( CDC* pDC, IN OUT CRect& rBoundsRect, const CRect& clipRect );
	bool FillBackground( CDC* pDC, IN OUT CRect& rBoundsRect );
private:
	ui::ISampleCallback* m_pSampleCallback;
	bool m_useDoubleBuffering;
	COLORREF m_borderColor;				// by default CLR_NONE, it uses WS_EX_STATICEDGE extended style; if CLR_DEFAULT it uses LightCyan
	COLORREF m_bkColor;

	HCURSOR m_hScrollableCursor, m_hScrollDragCursor;

	// generated stuff
protected:
	virtual void PostNcDestroy( void );
	virtual void OnDraw( CDC* pPaintDC );
public:
	virtual void OnInitialUpdate( void );
protected:
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
