
#include "stdafx.h"
#include "SampleView.h"
#include "GpUtilities.h"
#include "WndUtils.h"
#include "resource.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSampleView::CSampleView( ISampleCallback* pSampleCallback )
	: CScrollView()
	, m_pSampleCallback( pSampleCallback )
	, m_hScrollableCursor( AfxGetApp()->LoadCursor( IDR_SCROLLABLE_CURSOR ) )
	, m_hScrollDragCursor( AfxGetApp()->LoadCursor( IDR_SCROLL_DRAG_CURSOR ) )
{
}

CSampleView::~CSampleView()
{
}

void CSampleView::DDX_Placeholder( CDataExchange* pDX, int placeholderId )
{
	if ( NULL == m_hWnd )
	{
		VERIFY( Create( NULL, NULL, WS_CHILD | WS_VISIBLE, CRect( 0, 0, 0, 0 ), pDX->m_pDlgWnd, (WORD)-1 ) );
		ModifyStyleEx( 0, WS_EX_STATICEDGE );

		ui::AlignToPlaceholder( this, placeholderId )->DestroyWindow();
		SetDlgCtrlID( placeholderId );
		SendMessage( WM_INITIALUPDATE, 0, 0 );
	}
}

void CSampleView::SetContentSize( const CSize& contentSize )
{
	SetScrollSizes( MM_TEXT, contentSize );
}

CRect CSampleView::MakeDisplayRect( const CRect& clientRect, const CSize& displaySize ) const
{
	CRect displayRect( 0, 0, displaySize.cx, displaySize.cy );

	TAlignment alignment = NoAlign;
	SetFlag( alignment, H_AlignCenter, displayRect.Width() < clientRect.Width() );
	SetFlag( alignment, V_AlignCenter, displayRect.Height() < clientRect.Height() );

	if ( alignment != NoAlign )
		ui::AlignRect( displayRect, clientRect, alignment );
	return displayRect;
}

void CSampleView::SafeRedraw( void )
{
	if ( m_hWnd != NULL )
	{
		Invalidate();
		UpdateWindow();
	}
}

bool CSampleView::DrawContentFrame( CDC* pDC, const CRect& contentRect, COLORREF scrollableColor, BYTE alpha /*= 100*/ )
{
	CRect clientRect;
	GetClientRect( &clientRect );
	bool scrollable = IsScrollable();

	Graphics graphics( *pDC );
	Rect rect = gp::ToRect( contentRect );
	Pen pen( gp::MakeColor( scrollable ? scrollableColor : GetSysColor( COLOR_BTNSHADOW ), alpha ) );
	gp::FrameRectangle( graphics, rect, &pen );
	return scrollable;
}

void CSampleView::DrawError( CDC* pDC, const CRect& rect )
{
	Graphics graphics( *pDC );
	HatchBrush brush( HatchStyleBackwardDiagonal, Color( 128, 200, 0, 0 ), Color( 0, 0, 0, 0 ) );
	graphics.FillRectangle( &brush, gp::ToRect( rect ) );
}

void CSampleView::DrawCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha /*= 127*/ )
{
	Graphics graphics( *pDC );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	Pen pen( gp::MakeColor( color, alpha ) );
	pen.SetAlignment( PenAlignmentCenter );
	pen.SetDashStyle( DashStyleDash );

	CPoint center = rect.CenterPoint();
	graphics.DrawLine( &pen, Point( center.x, rect.top ), Point( center.x, rect.bottom ) );
	graphics.DrawLine( &pen, Point( rect.left, center.y ), Point( rect.right, center.y ) );
}

void CSampleView::DrawDiagonalCross( CDC* pDC, const CRect& rect, COLORREF color, BYTE alpha /*= 127*/ )
{
	Graphics graphics( *pDC );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	Pen pen( gp::MakeColor( color, alpha ) );
	pen.SetAlignment( PenAlignmentCenter );
	pen.SetDashStyle( DashStyleDash );

	Rect bounds = gp::ToRect( rect );
	graphics.DrawLine( &pen, Point( bounds.X, bounds.Y ), Point( bounds.GetRight(), bounds.GetBottom() ) );
	graphics.DrawLine( &pen, Point( bounds.X, bounds.GetBottom() ), Point( bounds.GetRight(), bounds.Y ) );
}

void CSampleView::RunTrackScroll( CPoint point )
{
	ClientToScreen( &point );

	MSG msg = { NULL };
	CPoint mouseAnchor = point, scrollAnchor = GetScrollPosition();

	SetCapture();

	HCURSOR hOldCursor = ::SetCursor( m_hScrollDragCursor );

	while ( m_hWnd == ::GetCapture() )
		if ( ::PeekMessage( &msg, m_hWnd, 0, 0, PM_REMOVE ) )
			switch ( msg.message )
			{
				case WM_MOUSEMOVE:
					// eat all unprocessed mouse-move messages that exist in message queue
					while ( ::PeekMessage( &msg, m_hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE ) ) {}
					TrackingScroll( mouseAnchor, scrollAnchor );
					break;
				case WM_LBUTTONUP:
					::ReleaseCapture();
					break;
				case WM_KEYDOWN:
					if ( VK_ESCAPE == msg.wParam )
					{	// cancel tracking and scroll to initial position
						ScrollToPosition( scrollAnchor );
						UpdateWindow();
						::ReleaseCapture();			// quit the modal loop
					}
					break;
			}

	::SetCursor( hOldCursor );
}

bool CSampleView::TrackingScroll( const CPoint& mouseAnchor, const CPoint& scrollAnchor )
{
	DWORD style = GetStyle();
	if ( !( style & ( WS_HSCROLL | WS_VSCROLL ) ) )
		return false;

	CPoint mousePos, scrollPos;
	::GetCursorPos( &mousePos );

	CSize delta = mouseAnchor - mousePos;

	if ( !HasFlag( style, WS_HSCROLL ) )
		delta.cx = 0;
	if ( !HasFlag( style, WS_VSCROLL ) )
		delta.cy = 0;

	scrollPos = scrollAnchor + delta;

	GetScrollRangeAs< long >( SB_HORZ ).Constrain( scrollPos.x );
	GetScrollRangeAs< long >( SB_VERT ).Constrain( scrollPos.y );

	if ( scrollPos == GetScrollPosition() )
		return false;

	ScrollToPosition( scrollPos );
	UpdateWindow();
	return true;
}


// message handlers

BEGIN_MESSAGE_MAP( CSampleView, CScrollView )
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEACTIVATE()
END_MESSAGE_MAP()

void CSampleView::PostNcDestroy( void )
{
	// no auto-delete, this is used as a dialog control
}

void CSampleView::OnInitialUpdate( void )
{
	SetNotScrollable();
}

void CSampleView::OnDraw( CDC* pDC )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	if ( NULL == m_pSampleCallback || !m_pSampleCallback->RenderSample( pDC, clientRect ) )
		DrawError( pDC, clientRect );
}

BOOL CSampleView::OnEraseBkgnd( CDC* pDC )
{
	if ( NULL == m_pSampleCallback )
		return CScrollView::OnEraseBkgnd( pDC );

	CRect clientRect;
	GetClientRect( &clientRect );
	m_pSampleCallback->RenderBackground( pDC, clientRect );
	return TRUE;			// erased
}

BOOL CSampleView::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	if ( HTCLIENT == hitTest && IsScrollable() && m_hScrollableCursor != NULL )
	{
		::SetCursor( m_hScrollableCursor );
		return TRUE;
	}
	return CScrollView::OnSetCursor( pWnd, hitTest, message );
}

void CSampleView::OnLButtonDown( UINT flags, CPoint point )
{
	CScrollView::OnLButtonDown( flags, point );

	if ( IsScrollable() )
		RunTrackScroll( point );
}

void CSampleView::OnMouseMove( UINT flags, CPoint point )
{
	CScrollView::OnMouseMove( flags, point );

	if ( m_pSampleCallback != NULL )
	{
		CClientDC dc( this );
		m_pSampleCallback->ShowPixelInfo( point, dc.GetPixel( point ) );
	}
}

int CSampleView::OnMouseActivate( CWnd* pDesktopWnd, UINT hitTest, UINT message )
{
	// skip CView base call to prevent assertion failure due to GetParentFrame()
	return CWnd::OnMouseActivate( pDesktopWnd, hitTest, message );
}
