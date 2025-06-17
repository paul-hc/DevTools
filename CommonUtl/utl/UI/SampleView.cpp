
#include "pch.h"
#include "SampleView.h"
#include "GpUtilities.h"
#include "WndUtils.h"
#include "resource.h"
#include <afxcontrolbarutil.h>		// for CMemDC

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define WM_INITIALUPDATE    0x0364  // defined in <afxpriv.h>


CSampleView::CSampleView( ui::ISampleCallback* pSampleCallback, bool useDoubleBuffering /*= true*/ )
	: CScrollView()
	, m_pSampleCallback( pSampleCallback )
	, m_useDoubleBuffering( useDoubleBuffering )
	, m_borderColor( CLR_NONE )
	, m_bkColor( CLR_NONE )
	, m_hScrollableCursor( AfxGetApp()->LoadCursor( IDR_SCROLLABLE_CURSOR ) )
	, m_hScrollDragCursor( AfxGetApp()->LoadCursor( IDR_SCROLL_DRAG_CURSOR ) )
{
}

CSampleView::~CSampleView()
{
}

void CSampleView::DDX_Placeholder( CDataExchange* pDX, int placeholderId )
{
	if ( nullptr == m_hWnd )
	{
		VERIFY( Create( nullptr, nullptr, WS_CHILD | WS_VISIBLE, CRect( 0, 0, 0, 0 ), pDX->m_pDlgWnd, (WORD)-1 ) );

		if ( CLR_NONE == m_borderColor )
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
	if ( m_hWnd != nullptr )
	{
		Invalidate();
		UpdateWindow();
	}
}

bool CSampleView::DrawContentFrame( CDC* pDC, const CRect& contentRect, COLORREF scrollableColor /*= CLR_NONE*/, BYTE alpha /*= 100*/ )
{
	CRect clientRect;
	GetClientRect( &clientRect );
	bool scrollable = IsScrollable();

	Graphics graphics( *pDC );
	Rect rect = gp::ToRect( contentRect );
	Pen pen( gp::MakeColor( scrollable && ui::IsRealColor( scrollableColor ) ? scrollableColor : ::GetSysColor( COLOR_BTNSHADOW ), alpha ) );

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

	MSG msg = { nullptr };
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

	GetScrollRangeAs<long>( SB_HORZ ).Constrain( scrollPos.x );
	GetScrollRangeAs<long>( SB_VERT ).Constrain( scrollPos.y );

	if ( scrollPos == GetScrollPosition() )
		return false;

	ScrollToPosition( scrollPos );
	UpdateWindow();
	return true;
}

void CSampleView::Draw( CDC* pDC, IN OUT CRect& rBoundsRect, const CRect& clipRect )
{
	clipRect;
	if ( m_pSampleCallback != nullptr )
	{
		if ( m_useDoubleBuffering )
		{
			rBoundsRect |= clipRect;				// fix issues with background not getting erased on scrolling
			FillBackground( pDC, rBoundsRect );
		}

		if ( m_pSampleCallback->RenderSample( pDC, rBoundsRect, this ) )
			return;
	}

	DrawError( pDC, rBoundsRect );
}

bool CSampleView::FillBackground( CDC* pDC, IN OUT CRect& rBoundsRect )
{
	if ( m_borderColor != CLR_NONE )
	{
		ui::FrameRect( *pDC, rBoundsRect, m_borderColor != CLR_DEFAULT ? m_borderColor : LightCyan );		// draw the border
		rBoundsRect.DeflateRect( 1, 1 );
	}

	if ( m_bkColor != CLR_NONE )
	{
		ui::FillRect( *pDC, rBoundsRect, m_bkColor );
		return true;
	}

	return m_pSampleCallback->RenderBackground( pDC, rBoundsRect, this );
}

void CSampleView::OnDraw( CDC* pPaintDC )
{
	CRect clientRect, clipRect;

	GetClientRect( &clientRect );
	pPaintDC->GetClipBox( &clipRect );

	if ( m_useDoubleBuffering )
	{
		CMemDC memDC( *pPaintDC, this );
		CDC* pDC = &memDC.GetDC();

		Draw( pDC, clientRect, clipRect );
	}
	else
		Draw( pPaintDC, clientRect, clipRect );
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

BOOL CSampleView::OnEraseBkgnd( CDC* pDC )
{
	if ( m_useDoubleBuffering )
		return TRUE;		// simulate that we erased the background to prevent default erasing

	if ( nullptr == m_pSampleCallback )
		return __super::OnEraseBkgnd( pDC );

	CRect clientRect;

	GetClientRect( &clientRect );
	return FillBackground( pDC, clientRect );		// TRUE if erased
}

BOOL CSampleView::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	if ( HTCLIENT == hitTest && IsScrollable() && m_hScrollableCursor != nullptr )
	{
		::SetCursor( m_hScrollableCursor );
		return TRUE;
	}
	return __super::OnSetCursor( pWnd, hitTest, message );
}

void CSampleView::OnLButtonDown( UINT flags, CPoint point )
{
	__super::OnLButtonDown( flags, point );

	if ( IsScrollable() )
		RunTrackScroll( point );
}

void CSampleView::OnMouseMove( UINT flags, CPoint point )
{
	__super::OnMouseMove( flags, point );

	if ( m_pSampleCallback != nullptr )
	{
		CClientDC dc( this );
		m_pSampleCallback->ShowPixelInfo( point, dc.GetPixel( point ), this );
	}
}

int CSampleView::OnMouseActivate( CWnd* pDesktopWnd, UINT hitTest, UINT message )
{
	// skip CView base call to prevent assertion failure due to GetParentFrame()
	return CWnd::OnMouseActivate( pDesktopWnd, hitTest, message );
}
