
#include "stdafx.h"
#include "BaseZoomView.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseZoomView implementation

CBaseZoomView::CBaseZoomView( ui::ImageScalingMode scalingMode, UINT zoomPct )
	: CScrollView()
	, m_scalingMode( scalingMode )
	, m_zoomPct( zoomPct )
	, m_pZoomBar( NULL )
{
}

CBaseZoomView::~CBaseZoomView()
{
}

void CBaseZoomView::ModifyScalingMode( ui::ImageScalingMode scalingMode )
{
	AssignScalingMode( scalingMode );
	switch ( m_scalingMode )
	{
		case ui::ActualSize:
			AssignZoomPct( 100 );
			break;
	}
	SetupContentMetrics();
}

bool CBaseZoomView::ModifyZoomPct( UINT zoomPct )
{
	const Range< UINT >& zoomLimits = ui::CStdZoom::Instance().m_limits;
	if ( !zoomLimits.Contains( zoomPct ) )			// zoom out of range?
	{
		OutputZoomPct();
		return false;
	}

	AssignZoomPct( zoomPct );
	switch ( m_scalingMode )
	{
		case ui::UseZoomPct:
			break;
		case ui::ActualSize:
			if ( 100 == m_zoomPct )
				break;
			// fall-through
		default:
			AssignScalingMode( ui::UseZoomPct );
	}
	SetupContentMetrics();
	return true;
}

void CBaseZoomView::SetScaleZoom( ui::ImageScalingMode scalingMode, UINT zoomPct )
{
	AssignScalingMode( scalingMode );
	AssignZoomPct( zoomPct );
	SetupContentMetrics();
}

bool CBaseZoomView::ZoomRelative( ZoomBy zoomBy )
{
	if ( NULL == m_pZoomBar )
		return false;

	UINT zoomPct = m_pZoomBar->InputZoomPct( ui::ByEdit );		// input zoom just in case combo is not updated (editing)

	const std::vector< UINT >& stdZoomPcts = ui::CStdZoom::Instance().m_zoomPcts;
	size_t newPos = std::distance( stdZoomPcts.begin(), std::lower_bound( stdZoomPcts.begin(), stdZoomPcts.end(), zoomPct ) );

	if ( newPos < stdZoomPcts.size() && zoomPct == stdZoomPcts[ newPos ] )
		newPos += zoomBy;
	else if ( ZoomOut == zoomBy )
		--newPos;

	return
		newPos < stdZoomPcts.size() &&				// in range
		stdZoomPcts[ newPos ] != zoomPct &&			// has changed
		ModifyZoomPct( stdZoomPcts[ newPos ] );
}

void CBaseZoomView::SetupContentMetrics( bool doRedraw /*= true*/ )
{
	if ( NULL == m_hWnd || IsInternalChange() )
		return;

	GetClientRect( &m_clientRect );

	m_contentRect.TopLeft() = m_contentRect.BottomRight() = m_clientRect.TopLeft();		// empty rect to client origin

	const CSize clientSize = m_clientRect.Size();
	const CSize srcSize = GetSourceSize();

	if ( !ui::IsEmptySize( clientSize ) && !ui::IsEmptySize( srcSize ) )
	{
		UINT zoomPct = m_zoomPct;
		CSize displaySize = srcSize;

		switch ( m_scalingMode )
		{
			case ui::AutoFitLargeOnly:
				if ( ui::FitsInside( clientSize, srcSize ) )
				{
					zoomPct = 100;				// no shrinking necessary -> use normal zoom (100%)
					break;
				}
				// fall-through
			case ui::AutoFitAll:
				// scale for Best Fit
				// (1) try to use horizontal scaling factor
				displaySize = ui::ScaleSize( srcSize, clientSize.cx, srcSize.cx );
				zoomPct = MulDiv( 100, clientSize.cx, srcSize.cx );

				if ( displaySize.cy > clientSize.cy )		// vertical overflow?
				{	// (2) use vertical scaling factor
					displaySize = ui::ScaleSize( srcSize, clientSize.cy, srcSize.cy );
					zoomPct = MulDiv( 100, clientSize.cy, srcSize.cy );
				}
				break;
			case ui::FitWidth:
				displaySize = ui::ScaleSize( srcSize, clientSize.cx, srcSize.cx );
				zoomPct = MulDiv( 100, clientSize.cx, srcSize.cx );
				break;
			case ui::FitHeight:
				displaySize = ui::ScaleSize( srcSize, clientSize.cy, srcSize.cy );
				zoomPct = MulDiv( 100, clientSize.cy, srcSize.cy );
				break;
			case ui::ActualSize:
				zoomPct = 100;
				break;
			case ui::UseZoomPct:
				displaySize = ui::ScaleSize( displaySize, zoomPct, 100 );
				break;
		}

		AssignZoomPct( zoomPct );

		ui::SetRectSize( m_contentRect, displaySize );
		ui::CenterRect( m_contentRect, m_clientRect, displaySize.cx < clientSize.cx, displaySize.cy < clientSize.cy );		// center image rect horiz/vert if extent smaller than client extent
	}

	StoreScrollExtent();
	if ( doRedraw )
		Invalidate( TRUE );
}

void CBaseZoomView::StoreScrollExtent( void )
{
	static int lineProportion = 12;
	CSize sizeLine( 1, 1 ), sizePage = CScrollView::sizeDefault;

	if ( !ui::IsEmptySize( m_contentRect.Size() ) )
	{
		sizePage = m_clientRect.Size();
		sizeLine = m_contentRect.Size() - sizePage;
		sizeLine.cx = std::max( 1L, sizeLine.cx / lineProportion );
		sizeLine.cy = std::max( 1L, sizeLine.cy / lineProportion );
	}
	if ( m_zoomPct > 100 )
		sizeLine = ui::ScaleSize( sizeLine, m_zoomPct, 100 );

	SetScrollSizes( MM_TEXT, m_contentRect.Size(), sizePage, sizeLine );		// calculate the total size of this view
	CenterOnPoint( m_contentRect.CenterPoint() );			// zoom on image's centre point
}

DWORD CBaseZoomView::GetScrollStyle( void ) const
{
	BOOL hasHorzBar, hasVertBar;
	CheckScrollBars( hasHorzBar, hasVertBar );		// ask the scroll view base, don't base this on checking GetStyle() for WS_HSCROLL/WS_VSCROLL

	DWORD scrollStyle = 0;
	if ( hasHorzBar )
		SetFlag( scrollStyle, WS_HSCROLL );
	if ( hasVertBar )
		SetFlag( scrollStyle, WS_VSCROLL );
	return scrollStyle;
}

bool CBaseZoomView::ClampScrollPos( CPoint& rScrollPos )
{
	CPoint oldScrollPos = rScrollPos;
	GetScrollRange<long>( SB_HORZ ).Constrain( rScrollPos.x );
	GetScrollRange<long>( SB_VERT ).Constrain( rScrollPos.y );
	return !( rScrollPos == oldScrollPos );								// changed?
}

CSize CBaseZoomView::GetContentPointedPct( const CPoint* pClientPoint /*= NULL*/ ) const
{
	CPoint point = pClientPoint != NULL ? *pClientPoint : ui::GetCursorPos( m_hWnd );

	point += GetScrollPosition() - m_contentRect.TopLeft();
	CSize pointedPct(
		MulDiv( 100, point.x, m_contentRect.Width() ),
		MulDiv( 100, point.y, m_contentRect.Height() ) );

	TRACE( _T(" - pointed percentage (%d%%, %d%%)\n"), pointedPct.cx, pointedPct.cy );
	return pointedPct;						// percentage of pointed client point relative to content origin: (50%, 50%) for content center
}

CPoint CBaseZoomView::TranslatePointedPct( const CSize& pointedPct ) const
{
	CPoint pos = m_contentRect.TopLeft() + CSize(
		MulDiv( m_contentRect.Width(), pointedPct.cx, 100 ),
		MulDiv( m_contentRect.Height(), pointedPct.cy, 100 ) );
	return pos;
}

BOOL CBaseZoomView::OnPreparePrinting( CPrintInfo* pInfo )
{
	return DoPreparePrinting( pInfo );		// default preparation
}


// message handlers

BEGIN_MESSAGE_MAP( CBaseZoomView, CScrollView )
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CBaseZoomView::OnSize( UINT sizeType, int cx, int cy )
{
	CScrollView::OnSize( sizeType, cx, cy );

	if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
		SetupContentMetrics();							// layout content
}

BOOL CBaseZoomView::OnEraseBkgnd( CDC* pDC )
{
	pDC;
	return TRUE;
}


// CScopedScaleZoom implementation

CScopedScaleZoom::CScopedScaleZoom( CBaseZoomView* pZoomView, ui::ImageScalingMode scalingMode, UINT zoomPct, const CPoint* pClientPoint /*= NULL*/ )
	: m_pZoomView( pZoomView )
	, m_oldScalingMode( pZoomView->GetScalingMode() )
	, m_oldZoomPct( pZoomView->GetZoomPct() )
	, m_oldScrollPosition( pZoomView->GetScrollPosition() )
	, m_refPointedPct( m_pZoomView->GetContentPointedPct( pClientPoint ) )
	, m_changed( scalingMode != m_oldScalingMode || zoomPct != m_oldZoomPct )
{
	if ( m_changed )
	{
		m_pZoomView->SetScaleZoom( scalingMode, zoomPct );
		m_pZoomView->CenterOnPoint( m_pZoomView->TranslatePointedPct( m_refPointedPct ) );		// center on the equivalent of the clicked point
		m_pZoomView->UpdateWindow();
	}
}

CScopedScaleZoom::~CScopedScaleZoom()
{
	if ( m_changed )
	{
		m_pZoomView->SetScaleZoom( m_oldScalingMode, m_oldZoomPct );
		m_pZoomView->ScrollToPosition( m_oldScrollPosition );
		m_pZoomView->UpdateWindow();
	}
}


// CZoomViewMouseTracker implementation

CZoomViewMouseTracker::CZoomViewMouseTracker( CBaseZoomView* pZoomView, CPoint point, TrackOperation trackOp )
	: m_pZoomView( pZoomView )
	, m_pZoomNormal( OpZoomNormal == trackOp ? new CScopedScaleZoom( pZoomView, ui::ActualSize, 100, &point ) : NULL )
	, m_trackOp( trackOp )
	, m_origPoint( point )
	, m_origScrollPos( m_pZoomView->GetScrollPosition() )
	, m_origScalingMode( m_pZoomView->GetScalingMode() )
	, m_origZoomPct( m_pZoomView->GetZoomPct() )
	, m_hOrigCursor( NULL )
{
	switch ( trackOp )
	{
		case OpZoom:
			m_hOrigCursor = ::SetCursor( AfxGetApp()->LoadCursor( IDR_TRACK_ZOOM_CURSOR ) );
			break;
		case OpScroll:
			m_hOrigCursor = ::SetCursor( AfxGetApp()->LoadCursor( IDR_TRACK_SCROLL_CURSOR ) );
			break;
		case OpZoomNormal:
			m_hOrigCursor = ::SetCursor( NULL );
			break;
	}
}

CZoomViewMouseTracker::~CZoomViewMouseTracker()
{
	m_pZoomNormal.reset();
	m_pZoomView->UpdateWindow();

	if ( m_hOrigCursor != NULL )
		::SetCursor( m_hOrigCursor );
}

bool CZoomViewMouseTracker::Run( CBaseZoomView* pZoomView, UINT mkFlags, CPoint point, TrackOperation trackOp /*= _Auto*/ )
{
	if ( _Auto == trackOp )
		if ( HasFlag( mkFlags, MK_CONTROL ) )
			trackOp = OpScroll;
		else if ( HasFlag( mkFlags, MK_SHIFT ) )
			trackOp = OpZoom;
		else
			trackOp = OpZoomNormal;

	ENSURE( trackOp != _Auto );

	if ( OpScroll == trackOp )
		if ( !pZoomView->AnyScrollBar() )
			return false;				// cancel tracking if scrolling not enabled

	CZoomViewMouseTracker tracker( pZoomView, point, trackOp );
	return tracker.RunLoop();
}

bool CZoomViewMouseTracker::RunLoop( void )
{
	m_pZoomView->SetCapture();

	MSG msg = { NULL };
	for ( CPoint point; ::GetCapture() == m_pZoomView->m_hWnd; )
		if ( ::PeekMessage( &msg, m_pZoomView->m_hWnd, 0, 0, PM_REMOVE ) )
			switch ( msg.message )
			{
				case WM_MOUSEMOVE:
					point = GetMsgPoint( &msg );
					while ( ::PeekMessage( &msg, m_pZoomView->m_hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE ) ) {}		// eat extra posted messages

					switch ( m_trackOp )
					{
						case OpZoom:		TrackZoom( point ); break;
						case OpScroll:		// fall-through
						case OpZoomNormal:	TrackScroll( point ); break;
					}
					break;
				case WM_LBUTTONUP:
				case WM_MBUTTONUP:
				case WM_RBUTTONUP:
					::ReleaseCapture();
					break;
				case WM_KEYDOWN:
					if ( VK_ESCAPE == msg.wParam )
					{
						Cancel();
						::ReleaseCapture();		// quit the modal loop
						return false;
					}
					break;
			}

	return true;								// commited
}

void CZoomViewMouseTracker::Cancel( void )
{
	// reset to the original metrics
	switch ( m_trackOp )
	{
		case OpZoom:		m_pZoomView->SetScaleZoom( m_origScalingMode, m_origZoomPct ); break;
		case OpScroll:		m_pZoomView->ScrollToPosition( m_origScrollPos ); break;
		case OpZoomNormal:	break;
	}
}

CPoint CZoomViewMouseTracker::GetMsgPoint( const MSG* pMsg )
{
	ASSERT_PTR( pMsg );
	POINTS point = MAKEPOINTS( pMsg->lParam );
	return CPoint( point.x, point.y );
}

bool CZoomViewMouseTracker::TrackScroll( const CPoint& point )
{
	DWORD scrollStyle = m_pZoomView->GetScrollStyle();
	if ( 0 == scrollStyle )
		return false;

	bool scrollH = HasFlag( scrollStyle, WS_HSCROLL ), scrollV = HasFlag( scrollStyle, WS_VSCROLL );

	CSize delta = m_origPoint - point;
	if ( OpZoomNormal == m_trackOp )
	{	// reverse scroll direction and amplify
		delta = point - m_origPoint;
		delta.cx = MulDiv( delta.cx, m_pZoomView->GetContentRect().Width(), m_pZoomView->_GetClientRect().Width() );
		delta.cy = MulDiv( delta.cy, m_pZoomView->GetContentRect().Height(), m_pZoomView->_GetClientRect().Height() );
	}

	if ( !scrollH )
		delta.cx = 0;
	if ( !scrollV )
		delta.cy = 0;

	CPoint scrollPos = m_origScrollPos + delta;
	m_pZoomView->ClampScrollPos( scrollPos );

	if ( scrollPos == m_pZoomView->GetScrollPosition() )
		return false;

	m_pZoomView->ScrollToPosition( scrollPos );
	m_pZoomView->UpdateWindow();
	return true;
}

bool CZoomViewMouseTracker::TrackZoom( const CPoint& point )
{
	int delta = point.x - m_origPoint.x;
	if ( 0 == delta )
		return false;

	enum { Divider = 10 };

	int rawZoomPct = m_origZoomPct;
	if ( delta > 0 )
		rawZoomPct += MulDiv( m_origZoomPct, delta, Divider );
	else
		rawZoomPct -= MulDiv( m_origZoomPct, -delta, Divider * 10 );

	rawZoomPct = std::max( rawZoomPct, (int)ui::CStdZoom::Instance().m_limits.m_start );

	UINT zoomPct = rawZoomPct;

	ui::CStdZoom::Instance().m_limits.Constrain( zoomPct );
	if ( m_pZoomView->GetZoomPct() == zoomPct )
		return false;

	m_pZoomView->ModifyZoomPct( zoomPct );
	m_pZoomView->UpdateWindow();
	return true;
}
