#ifndef BaseZoomView_h
#define BaseZoomView_h
#pragma once

#include "InternalChange.h"
#include "IZoomBar.h"


// scroll view with zomming that displays scaled content of the original source object (e.g. image)
//
abstract class CBaseZoomView : public CScrollView
							 , public CInternalChange
{
protected:
	CBaseZoomView( ui::ImageScalingMode scalingMode, UINT zoomPct );
	virtual ~CBaseZoomView();

	void StoreScrollExtent( void );
	void SetupContentMetrics( bool doRedraw = true );
public:
	using CScrollView::CenterOnPoint;

	// pure interface
	virtual CSize GetSourceSize( void ) const = 0;
	virtual COLORREF GetBkColor( void ) const = 0;

	const CRect& GetContentRect( void ) const { return m_contentRect; }
	const CRect& _GetClientRect( void ) const { return m_clientRect; }

	// hit test (client coordinates)
	bool InClientRect( const CPoint& point ) const { return _GetClientRect().PtInRect( point ) != FALSE; }
	bool InContentRect( const CPoint& point ) const { return GetContentRect().PtInRect( point ) != FALSE; }
	bool InBackgroundRect( const CPoint& point ) const { return !InContentRect( point ) && InClientRect( point ); }

	DWORD GetScrollStyle( void ) const;
	bool AnyScrollBar( void ) const { return GetScrollStyle() != 0; }
	bool ClampScrollPos( CPoint& rScrollPos );								// limit to valid H/V scroll range

	template< typename Type >
	Range< Type > GetScrollRange( int bar ) const { int lower, upper; CScrollView::GetScrollRange( bar, &lower, &upper ); return Range< Type >( lower, upper ); }

	// coords translation
	CSize GetContentPointedPct( const CPoint* pClientPoint = NULL ) const;			// use mouse cursor if NULL
	CPoint TranslatePointedPct( const CSize& pointedPct ) const;					// equivalent point after rescaling

	// zoom editor
	ui::ImageScalingMode GetScalingMode( void ) const { return m_scalingMode; }
	void ModifyScalingMode( ui::ImageScalingMode scalingMode );

	UINT GetZoomPct( void ) const { return m_zoomPct; }
	bool ModifyZoomPct( UINT zoomPct );

	void SetScaleZoom( ui::ImageScalingMode scalingMode, UINT zoomPct );

	// view state
	enum ViewStatusFlags
	{
		FullScreen			= BIT_FLAG( 0 ),
		ZoomMouseTracking	= BIT_FLAG( 1 )
	};
	typedef int TViewStatusFlag;

	bool HasViewStatusFlag( TViewStatusFlag flag ) const { return HasFlag( m_viewStatusFlags, flag ); }
	bool SetViewStatusFlag( TViewStatusFlag flag, bool on = true );
	virtual void OnViewStatusChanged( TViewStatusFlag flag );

	static COLORREF MakeAccentedBkColor( COLORREF bkColor );	// background highlighting (used in full screen)
protected:
	bool AssignScalingMode( ui::ImageScalingMode scalingMode ) { return utl::ModifyValue( m_scalingMode, scalingMode ) && OutputScalingMode(); }
	bool OutputScalingMode( void ) { return m_pZoomBar != NULL && m_pZoomBar->OutputScalingMode( m_scalingMode ); }
	void InputScalingMode( void ) { if ( m_pZoomBar != NULL ) ModifyScalingMode( m_pZoomBar->InputScalingMode() ); }

	bool AssignZoomPct( UINT zoomPct ) { return utl::ModifyValue( m_zoomPct, zoomPct ) && OutputZoomPct(); }
	bool OutputZoomPct( void ) { return m_pZoomBar != NULL && m_pZoomBar->OutputZoomPct( m_zoomPct ); }
	bool InputZoomPct( ui::ComboField byField ) { return m_pZoomBar != NULL && ModifyZoomPct( m_pZoomBar->InputZoomPct( byField ) ); }

	enum ZoomBy { ZoomIn = 1, ZoomOut = -1 };
	bool ZoomRelative( ZoomBy zoomBy );

	ui::IZoomBar* GetZoomBar( void ) const { return m_pZoomBar; }
	void SetZoomBar( ui::IZoomBar* pZoomBar ) { m_pZoomBar = pZoomBar; }

	TViewStatusFlag& RefViewStatusFlags( void ) { return m_viewStatusFlags; }
private:
	ui::ImageScalingMode m_scalingMode;		// default auto image size (app::Slider_v4_0+)
	UINT m_zoomPct;
	ui::IZoomBar* m_pZoomBar;
	TViewStatusFlag m_viewStatusFlags;

	CRect m_clientRect;
	CRect m_contentRect;

	// generated stuff
protected:
	virtual BOOL OnPreparePrinting( CPrintInfo* pInfo );
protected:
	virtual void OnSize( UINT sizeType, int cx, int cy );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );

	DECLARE_MESSAGE_MAP()
};


class CScopedScaleZoom;


class CZoomViewMouseTracker : private utl::noncopyable
{
public:
	enum TrackOperation { OpZoom, OpScroll, OpZoomNormal, _Auto };

	CZoomViewMouseTracker( CBaseZoomView* pZoomView, CPoint point, TrackOperation trackOp );
	~CZoomViewMouseTracker();

	static bool Run( CBaseZoomView* pZoomView, UINT mkFlags, CPoint point, TrackOperation trackOp = _Auto );
protected:
	static CPoint GetMsgPoint( const MSG* pMsg );

	bool RunLoop( void );
	bool TrackScroll( const CPoint& point );
	bool TrackZoom( const CPoint& point );
	void Cancel( void );
private:
	CBaseZoomView* m_pZoomView;
	TrackOperation m_trackOp;
	std::auto_ptr< CScopedScaleZoom > m_pZoomNormal;		// must be created before storing original data-members (if OpZoomNormal)

	// original data
	const CPoint m_origPoint;
	const CPoint m_origScrollPos;
	const ui::ImageScalingMode m_origScalingMode;
	const UINT m_origZoomPct;
	HCURSOR m_hOrigCursor;
};


class CScopedScaleZoom
{
public:
	CScopedScaleZoom( CBaseZoomView* pZoomView, ui::ImageScalingMode scalingMode, UINT zoomPct, const CPoint* pClientPoint = NULL );
	~CScopedScaleZoom();
private:
	CBaseZoomView* m_pZoomView;
	ui::ImageScalingMode m_oldScalingMode;
	UINT m_oldZoomPct;
	CPoint m_oldScrollPosition;
	CSize m_refPointedPct;						// percentage of clicked point to old content origin
	bool m_changed;
};


#endif // BaseZoomView_h
