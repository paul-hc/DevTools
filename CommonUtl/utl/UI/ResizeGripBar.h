#ifndef ResizeGripBar_h
#define ResizeGripBar_h
#pragma once


class CResizeFrameStatic;


namespace ui
{
	enum Orientation { UpDown, LeftRight };
}


class CResizeGripBar : public CStatic
{
public:
	enum Orientation { ResizeUpDown, ResizeLeftRight };
	enum ToggleStyle { NoToggle, ToggleFirst, ToggleSecond };

	CResizeGripBar( Orientation orientation, ToggleStyle toggleStyle = NoToggle, int windowExtent = -100 );
	~CResizeGripBar();

	void SetMinExtents( int firstMinExtent, int secondMinExtent );

	// size of the small dimension (height for an up-down, width for an left-right)
	int GetWindowDepth( void ) const { return m_windowDepth; }
	void SetWindowDepth( int windowDepth ) { m_windowDepth = windowDepth; }
	void IncreaseWindowDepth( int byWindowDepth ) { m_windowDepth += byWindowDepth; }

	bool HasBorder( void ) const { return m_hasBorder; }
	bool SetBorder( bool hasBorder = true ) { return m_hasBorder = hasBorder; }

	int GetFirstExtentPercentage( void ) const { return m_firstExtentPercentage; }
	void SetFirstExtentPercentage( int firstExtentPercentage );

	bool IsCollapsed( void ) const { return m_isCollapsed; }
	void SetCollapsed( bool collapsed );

	// for state persistence
	int& RefFirstExtentPercentage( void ) { return m_firstExtentPercentage; }
	bool& RefCollapsed( void ) { return m_isCollapsed; }

	bool CreateGripper( CResizeFrameStatic* pResizeFrame, CWnd* pFirstCtrl, CWnd* pSecondCtrl,
						UINT id = 0xFFFF, DWORD style = WS_CHILD | WS_VISIBLE | SS_NOTIFY );

	void LayoutProportionally( bool repaint = true );		// preserves existing aspect ratio
private:
	void CreateArrowsImageList( void );
	static CSize LoadArrowsBitmap( CBitmap* pBitmap, UINT bitmapResId, COLORREF arrowColor );	// returns the size of ONE arrow

	int GetArrowDepth( void ) const { ASSERT( m_arrowSize != CSize( 0, 0 ) ); return std::min( m_arrowSize.cx, m_arrowSize.cy ); }		// depth is the smaller arrow dimension
	int GetArrowExtent( void ) const { ASSERT( m_arrowSize != CSize( 0, 0 ) ); return std::max( m_arrowSize.cx, m_arrowSize.cy ); }		// extent is the bigger arrow dimension
protected:
	struct CFrameLayoutInfo
	{
		CRect m_frameRect; // in parent coordinates
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
	int GetRectExtent( const CRect& rect ) const { return ResizeUpDown == m_orientation ? rect.Height() : rect.Width(); }

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

	void DrawGripBar( CDC* pDC, const CRect& rect );
	void DrawArrow( CDC* pDC, const CRect& rect, ArrowPart part, ArrowState state );
private:
	const Orientation m_orientation;
	const ToggleStyle m_toggleStyle;
	const int m_windowExtent;
	int m_windowDepth;
	bool m_hasBorder;					// for non-collapsed state

	CSize m_arrowSize;
	CImageList m_arrowImageList;

	CResizeFrameStatic* m_pResizeFrame;
	CWnd* m_pFirstCtrl;
	CWnd* m_pSecondCtrl;
	int m_firstMinExtent;				// if negative, is percentage of the initial extent
	int m_secondMinExtent;				// if negative, is percentage of the initial extent

	bool m_isCollapsed;
	int m_firstExtentPercentage;		// when not collapsed

	HitTest m_hitOn;
	CTrackingInfo* m_pTrackingInfo;

	static HCURSOR s_hCursors[ 2 ];		// indexed by orientation

	// generated stuff
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


// inline code

inline void CResizeGripBar::SetMinExtents( int firstMinExtent, int secondMinExtent )
{
	m_firstMinExtent = firstMinExtent;
	m_secondMinExtent = secondMinExtent;
}


#endif // ResizeGripBar_h
