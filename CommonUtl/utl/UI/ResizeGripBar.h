#ifndef ResizeGripBar_h
#define ResizeGripBar_h
#pragma once

#include "ui_fwd.h"


namespace resize
{
	enum Orientation
	{
		NorthSouth,		// top & bottom panels: horizontal splitter grip-bar
		WestEast		// left & right panels: vertical splitter grip-bar
	};

	enum ToggleStyle { NoToggle, ToggleFirst, ToggleSecond };
}


class CResizeFrameStatic;


class CResizeGripBar : public CStatic
{
public:
	CResizeGripBar( CWnd* pFirstCtrl, CWnd* pSecondCtrl, resize::Orientation orientation, resize::ToggleStyle toggleStyle = resize::ToggleSecond );
	virtual ~CResizeGripBar();

	CResizeGripBar& SetMinExtents( TValueOrPct firstMinExtent, TValueOrPct secondMinExtent );

	bool HasBorder( void ) const { return m_layout.m_hasBorder; }
	bool SetBorder( bool hasBorder = true ) { return m_layout.m_hasBorder = hasBorder; }

	TPercent GetFirstExtentPercentage( void ) const { return m_layout.m_firstExtentPercentage; }
	CResizeGripBar& SetFirstExtentPercentage( TPercent firstExtentPercentage );

	bool IsCollapsed( void ) const { return m_layout.m_isCollapsed; }
	CResizeGripBar& SetCollapsed( bool collapsed );

	bool IsTracking( void ) const { return m_pTrackingInfo != nullptr; }

	// size of the small dimension of the bar: height for an up-down, width for an left-right
	//int GetWindowDepth( void ) const { return m_windowDepth; }
	//void SetWindowDepth( int windowDepth ) { m_windowDepth = windowDepth; }
	//void IncreaseWindowDepth( int byWindowDepth ) { m_windowDepth += byWindowDepth; }

	bool CreateGripper( CResizeFrameStatic* pResizeFrame, UINT id = 0xFFFF );

	void LayoutProportionally( bool repaint = true );		// preserves existing aspect ratio

	static COLORREF GetHotArrowColor( void );
public:
	typedef std::pair<TValueOrPct, TValueOrPct> TPanelMinExtents;


	struct CLayout
	{
		CLayout( resize::Orientation orientation )
			: m_orientation( orientation )
			, m_minExtents( utl::make_pair_single( 2 * ::GetSystemMetrics( resize::NorthSouth == m_orientation ? SM_CYHSCROLL : SM_CXVSCROLL ) ) )
			, m_firstExtentPercentage( -1 )
			, m_isCollapsed( false )
			, m_hasBorder( false )
		{
		}
	public:
		const resize::Orientation m_orientation;
		TPanelMinExtents m_minExtents;		// if negative, is percentage of the initial extent
		TPercent m_firstExtentPercentage;	// when not collapsed
		bool m_isCollapsed;
		bool m_hasBorder;					// for non-collapsed state
	};


	const CLayout& GetLayout( void ) const { return m_layout; }
private:
	void CreateArrowsImageList( void );
	static CSize LoadArrowsBitmap( CBitmap* pBitmap, UINT bitmapResId, COLORREF arrowColor );	// returns the size of ONE arrow

	int GetArrowDepth( void ) const { ASSERT( m_arrowSize != CSize( 0, 0 ) ); return std::min( m_arrowSize.cx, m_arrowSize.cy ); }		// depth is the smaller arrow dimension
	int GetArrowExtent( void ) const { ASSERT( m_arrowSize != CSize( 0, 0 ) ); return std::max( m_arrowSize.cx, m_arrowSize.cy ); }		// extent is the bigger arrow dimension
protected:
	enum GripperMetrics
	{
		DepthSpacing = 1,
		GripSpacing = 2			// so that 1 grip line is drawn
	};

	struct CFrameLayoutInfo
	{
		CRect m_frameRect;		// in parent coordinates
		int m_maxExtent;
	};

	struct CTrackingInfo
	{
		CTrackingInfo( const CSize& trackOffset, const CPoint& trackPos, const CRect& mouseTrapRect );
		~CTrackingInfo() { ClipCursor( &m_oldMouseTrapRect ); }
	public:
		// all in screen coordinates
		const CSize m_trackOffset;
		CPoint m_trackPos;
		CRect m_mouseTrapRect;
		CRect m_oldMouseTrapRect;
		bool m_wasDragged;
	};

	struct CDrawAreas
	{
		CRect m_clientRect;
		CRect m_gripRect;
		CRect m_arrowRect1;
		CRect m_arrowRect2;
	};
protected:
	int GetRectExtent( const CRect& rect ) const { return resize::NorthSouth == m_layout.m_orientation ? rect.Height() : rect.Width(); }

	void ReadLayoutInfo( CFrameLayoutInfo& rInfo ) const;
	bool LimitFirstExtentToBounds( int& rFirstExtent, int maxExtent ) const;
	void LayoutGripperTo( const CFrameLayoutInfo& info, const int firstExtent, bool repaint = true );

	void ComputeInitialMetrics( void );
	void ComputeLayoutRects( CRect& rGripperRect, CRect& rFirstRect, CRect& rSecondRect,
							 const CFrameLayoutInfo& info, const int firstExtent ) const;
	CRect ComputeMouseTrapRect( const CSize& trackOffset ) const;

	bool TrackToPos( CPoint screenTrackPos );
private:
	enum ArrowPart { Expanded, Collapsed };
	enum ArrowState { Normal, Hot, Disabled };
	enum HitTest { Nowhere, GripBar, ToggleArrow };

	ArrowPart GetArrowPart( void ) const;
	ArrowState GetArrowState( HitTest hitOn ) const;
	int GetImageIndex( ArrowPart part, ArrowState state ) const;

	HitTest GetHitTest( const CDrawAreas& rAreas, const CPoint& clientPos ) const;
	bool SetHitOn( HitTest hitOn );
private:
	CDrawAreas GetDrawAreas( void ) const;
	HitTest GetMouseHitTest( const CDrawAreas& rAreas ) const;

	void Draw( CDC* pDC, const CRect& clientRect );
	void DrawBackground( CDC* pDC, const CRect& clientRect );

	void DrawGripBar( CDC* pDC, const CRect& rect );
	void DrawArrow( CDC* pDC, const CRect& rect, ArrowPart part, ArrowState state );
private:
	typedef std::pair<CWnd*, CWnd*> TPanelCtrls;

	CLayout m_layout;
	const resize::ToggleStyle m_toggleStyle;
	TPanelCtrls m_panelCtrls;

	CResizeFrameStatic* m_pResizeFrame;		// sibling of this bar
	int m_windowDepth;
	CSize m_arrowSize;
	CImageList m_arrowImageList;

	HitTest m_hitOn;
	CTrackingInfo* m_pTrackingInfo;

	static HCURSOR s_hCursors[ 2 ];		// indexed by orientation

	enum Colors { HotCyan = RGB( 169, 219, 246 ), HotDeepCyan = RGB( 189, 237, 255 ), MildGray = RGB( 192, 192, 192 ), MildDarkerGray = RGB( 173, 178, 181 ) };

	// generated stuff
protected:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg BOOL OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message );
	afx_msg void OnLButtonDown( UINT flags, CPoint point );
	afx_msg void OnLButtonUp( UINT flags, CPoint point );
	afx_msg void OnMouseMove( UINT flags, CPoint point );
	afx_msg LRESULT OnMouseLeave( WPARAM wParam, LPARAM lParam );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ResizeGripBar_h
