
#include "StdAfx.h"
#include "ResizeGripBar.h"
#include "ResizeFrameStatic.h"
#include "Color.h"
#include "LayoutMetrics.h"
#include "Utilities.h"
#include "ThemeItem.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	static COLORREF GetHotArrowColor( void )
	{
		return ui::GetAdjustLuminance( ::GetSysColor( COLOR_HOTLIGHT ), 30 );					// contrast blue
	}

	static COLORREF GetHotBkColor( void )
	{
		return ui::GetAdjustLuminance( ::GetSysColor( COLOR_GRADIENTINACTIVECAPTION ), 30 );	// light blueish (like scroll fill)
	}
}


enum GripperMetrics
{
	DepthSpacing = 1,
	GripSpacing = 2			// so that 1 grip line is drawn
};


HCURSOR CResizeGripBar::s_hCursors[ 2 ] = { NULL, NULL };


CResizeGripBar::CResizeGripBar( Orientation orientation, ToggleStyle toggleStyle /*= NoToggle*/, int windowExtent /*= -100*/ )
	: CStatic()
	, m_orientation( orientation )
	, m_toggleStyle( toggleStyle )
	, m_arrowSize( 0, 0 )
	, m_windowExtent( windowExtent )
	, m_windowDepth( 0 )
	, m_hasBorder( false )
	, m_pResizeFrame( NULL )
	, m_pFirstCtrl( NULL )
	, m_pSecondCtrl( NULL )
	, m_firstMinExtent( 2 * GetSystemMetrics( ResizeUpDown == m_orientation ? SM_CYHSCROLL : SM_CXVSCROLL ) )
	, m_secondMinExtent( m_firstMinExtent )
	, m_isCollapsed( false )
	, m_firstExtentPercentage( -1 )
	, m_hitOn( Nowhere )
	, m_pTrackingInfo( NULL )
{
	CreateArrowsImageList();

	if ( NULL == s_hCursors[ ResizeUpDown ] )
	{
		s_hCursors[ ResizeUpDown ] = AfxGetApp()->LoadCursor( IDR_RESIZE_UPDOWN_CURSOR );
		s_hCursors[ ResizeLeftRight ] = AfxGetApp()->LoadCursor( IDR_RESIZE_LEFTRIGHT_CURSOR );
	}
}

CResizeGripBar::~CResizeGripBar()
{
	delete m_pTrackingInfo;
}

void CResizeGripBar::CreateArrowsImageList( void )
{
	ASSERT_NULL( m_arrowImageList.GetSafeHandle() );

	CBitmap bitmapArrows;
	UINT bitmapResId = ui::UpDown == m_orientation ? IDB_RESIZE_DOWN_UP_BITMAP : IDB_RESIZE_RIGHT_LEFT_BITMAP;

	// load state: Normal (and compute arrow-dependent sizes)
	m_arrowSize = LoadArrowsBitmap( &bitmapArrows, bitmapResId, ui::GetHotArrowColor() );
	m_windowDepth = GetArrowDepth() + DepthSpacing * 2;
	VERIFY( m_arrowImageList.Create( m_arrowSize.cx, m_arrowSize.cy, ILC_COLORDDB | ILC_MASK, 0, 2 ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGrey );

	// load state: Hot
	LoadArrowsBitmap( &bitmapArrows, bitmapResId, GetSysColor( COLOR_BTNTEXT ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGrey );

	// load state: Disabled
	LoadArrowsBitmap( &bitmapArrows, bitmapResId, GetSysColor( COLOR_GRAYTEXT ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGrey );
}

CSize CResizeGripBar::LoadArrowsBitmap( CBitmap* pBitmap, UINT bitmapResId, COLORREF arrowColor )
{
	ASSERT_PTR( pBitmap );
	pBitmap->DeleteObject();

	COLORMAP colorMap = { color::Black, arrowColor };
	VERIFY( pBitmap->LoadMappedBitmap( bitmapResId, 0, &colorMap, 1 ) );

	CSize arrowSize = gdi::GetBitmapSize( *pBitmap );
	arrowSize.cx /= 2;		// reduce to 1 arrow cell (out of 2)

	return arrowSize;
}

bool CResizeGripBar::CreateGripper( CResizeFrameStatic* pResizeFrame, CWnd* pFirstCtrl, CWnd* pSecondCtrl,
									UINT id /*= 0xFFFF*/, DWORD style /*= WS_CHILD | VS_VISIBLE | SS_NOTIFY*/ )
{
	REQUIRE( pResizeFrame != NULL && pFirstCtrl != NULL && pSecondCtrl != NULL );
	ASSERT( HasFlag( style, WS_CHILD ) );

	m_pResizeFrame = pResizeFrame;
	m_pFirstCtrl = pFirstCtrl;
	m_pSecondCtrl = pSecondCtrl;

	ComputeInitialMetrics();

	static const TCHAR* pCaption[] = { _T("<UpDown gripper>"), _T("<LeftRight gripper>") };

	if ( !Create( pCaption[ m_orientation ], style, CRect( 0, 0, 0, 0 ), m_pResizeFrame->GetParent(), id ) )
		return false;

	ASSERT_NULL( m_pTrackingInfo );

	LayoutProportionally();
	return true;
}

void CResizeGripBar::ComputeInitialMetrics( void )
{
	// if minimum extents are percentages, evaluate them to the actual limits
	if ( m_firstMinExtent < 0 )
	{
		CRect rectInitial;
		m_pFirstCtrl->GetWindowRect( &rectInitial );
		m_firstMinExtent = GetRectExtent( rectInitial ) * -m_firstMinExtent / 100;
	}

	if ( m_secondMinExtent < 0 )
	{
		CRect rectInitial;
		m_pSecondCtrl->GetWindowRect( &rectInitial );
		m_secondMinExtent = GetRectExtent( rectInitial ) * -m_secondMinExtent / 100;
	}

	if ( 0 == m_windowDepth )
	{	// default depth is the distance between first and second windows
		CRect rectFirst, rectSecond;
		m_pFirstCtrl->GetWindowRect( &rectFirst );
		m_pSecondCtrl->GetWindowRect( &rectSecond );

		if ( m_orientation == ResizeUpDown )
			m_windowDepth = rectSecond.top - rectFirst.bottom;
		else
			m_windowDepth = rectSecond.left - rectFirst.right;
	}

	if ( -1 == m_firstExtentPercentage )
	{	// take it from the current size of the first window versus frame window
		CFrameLayoutInfo info;
		ReadLayoutInfo( info );

		CRect rectFirst;
		m_pFirstCtrl->GetWindowRect( &rectFirst );

		int firstExtent = GetRectExtent( rectFirst );

		LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );

		m_firstExtentPercentage = 100 * firstExtent / info.m_maxExtent;
		ENSURE( m_firstExtentPercentage >= 0 && m_firstExtentPercentage <= 100 );
	}
}

void CResizeGripBar::SetFirstExtentPercentage( int firstExtentPercentage )
{
	REQUIRE( firstExtentPercentage >= 0 && firstExtentPercentage <= 100 );

	if ( m_hWnd != NULL )
	{
		CFrameLayoutInfo info;
		ReadLayoutInfo( info );

		int firstExtent = firstExtentPercentage * info.m_maxExtent / 100;		// convert to absolute extent

		LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );
		m_firstExtentPercentage = 100 * firstExtent / info.m_maxExtent;			// convert back to percentage

		LayoutProportionally();
	}
	else
		m_firstExtentPercentage = firstExtentPercentage;
}

void CResizeGripBar::SetCollapsed( bool collapsed )
{
	ASSERT( m_toggleStyle != NoToggle );

	m_isCollapsed = collapsed;

	if ( m_hWnd != NULL )
	{
		LayoutProportionally();
		Invalidate();
	}
}

void CResizeGripBar::LayoutProportionally( bool repaint /*= true*/ )
{
	ASSERT_PTR( m_hWnd );

	CFrameLayoutInfo info;
	ReadLayoutInfo( info );

	// computation is driven from info.m_frameRect and preserves m_firstExtentPercentage
	int firstExtent = m_firstExtentPercentage * info.m_maxExtent / 100;			// convert to absolute extent

	LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );

	LayoutGripperTo( info, firstExtent, repaint );
}

void CResizeGripBar::LayoutGripperTo( const CFrameLayoutInfo& info, const int firstExtent, bool repaint /*= true*/ )
{
	ASSERT_PTR( m_hWnd );

	CRect gripperRect, firstRect, secondRect;
	ComputeLayoutRects( gripperRect, firstRect, secondRect, info, firstExtent );

	if ( ToggleFirst == m_toggleStyle )
		ui::ShowWindow( *m_pFirstCtrl, !m_isCollapsed || ToggleSecond == m_toggleStyle );

	if ( ToggleSecond == m_toggleStyle )
		ui::ShowWindow( *m_pSecondCtrl, !m_isCollapsed || ToggleFirst == m_toggleStyle );

	m_pFirstCtrl->MoveWindow( &firstRect, repaint );
	MoveWindow( &gripperRect, repaint );
	m_pSecondCtrl->MoveWindow( &secondRect, repaint );
}

bool CResizeGripBar::TrackToPos( CPoint screenTrackPos )
{
	ASSERT_PTR( m_pTrackingInfo );

	CSize deltaPos = screenTrackPos - m_pTrackingInfo->m_trackPos;
	int trackDelta = ResizeUpDown == m_orientation ? deltaPos.cy : deltaPos.cx;

	if ( 0 == trackDelta )
		return false;

	m_pTrackingInfo->m_trackPos = screenTrackPos;
	m_pTrackingInfo->m_wasDragged = true;

	CFrameLayoutInfo info;
	ReadLayoutInfo( info );

	CRect rectFirst;
	m_pFirstCtrl->GetWindowRect( &rectFirst );

	int firstExtent = GetRectExtent( rectFirst );

	// advance to track position
	firstExtent += trackDelta;

	if ( !LimitFirstExtentToBounds( firstExtent, info.m_maxExtent ) )
		return false;

	m_firstExtentPercentage = 100 * firstExtent / info.m_maxExtent;				// convert back to percentage

	LayoutGripperTo( info, firstExtent, true );
	return true;
}

void CResizeGripBar::ReadLayoutInfo( CFrameLayoutInfo& rInfo ) const
{
	m_pResizeFrame->GetWindowRect( &rInfo.m_frameRect );
	GetParent()->ScreenToClient( &rInfo.m_frameRect );

	rInfo.m_maxExtent = GetRectExtent( rInfo.m_frameRect ) - m_windowDepth;
}

bool CResizeGripBar::LimitFirstExtentToBounds( int& rFirstExtent, int maxExtent ) const
{
	int oldFirstExtent = rFirstExtent;

	if ( rFirstExtent < m_firstMinExtent )
		rFirstExtent = m_firstMinExtent;

	int secondExtent = maxExtent - rFirstExtent;

	if ( secondExtent < m_secondMinExtent )
		rFirstExtent -= ( m_secondMinExtent - secondExtent );

	ENSURE( rFirstExtent >= m_firstMinExtent && rFirstExtent <= maxExtent );

	return rFirstExtent == oldFirstExtent;		// false if constrained
}

void CResizeGripBar::ComputeLayoutRects( CRect& rGripperRect, CRect& rFirstRect, CRect& rSecondRect, const CFrameLayoutInfo& info,
										 const int firstExtent ) const
{
	// computation is driven from frameRect using firstExtent

	rGripperRect = rFirstRect = rSecondRect = info.m_frameRect;		// start with all rects to frame

	CWnd* pHiddenWnd = NULL;

	if ( m_isCollapsed )
		pHiddenWnd = m_toggleStyle == ToggleFirst ? m_pFirstCtrl : m_pSecondCtrl;

	if ( ResizeUpDown == m_orientation )
		if ( pHiddenWnd == m_pFirstCtrl )
		{	// first collapsed  - gripper at top
			rFirstRect.bottom = rFirstRect.top + firstExtent;
			rGripperRect.bottom = rGripperRect.top + m_windowDepth;
			rSecondRect.top = rGripperRect.bottom;
		}
		else if ( pHiddenWnd == m_pSecondCtrl )
		{	// second collapsed  - gripper at bottom
			int secondExtent = info.m_maxExtent - firstExtent;

			rFirstRect.bottom -= m_windowDepth;
			rGripperRect.top = rGripperRect.bottom - m_windowDepth;
			rSecondRect.top = rSecondRect.bottom - secondExtent;
		}
		else
		{	// not collapsed - all visible
			rFirstRect.bottom = rFirstRect.top + firstExtent;
			rGripperRect.top = rFirstRect.bottom;
			rGripperRect.bottom = rGripperRect.top + m_windowDepth;
			rSecondRect.top = rGripperRect.bottom;
		}
	else
		if ( pHiddenWnd == m_pFirstCtrl )
		{	// first collapsed  - gripper at left
			rFirstRect.right = rFirstRect.left + firstExtent;
			rGripperRect.right = rGripperRect.left + m_windowDepth;
			rSecondRect.left = rGripperRect.right;
		}
		else if ( pHiddenWnd == m_pSecondCtrl )
		{	// second collapsed  - gripper at right
			int secondExtent = info.m_maxExtent - firstExtent;

			rFirstRect.right -= m_windowDepth;
			rGripperRect.left = rGripperRect.right - m_windowDepth;
			rSecondRect.left = rSecondRect.right - secondExtent;
		}
		else
		{	// not collapsed - all visible
			rFirstRect.right = rFirstRect.left + firstExtent;
			rGripperRect.left = rFirstRect.right;
			rGripperRect.right = rGripperRect.left + m_windowDepth;
			rSecondRect.left = rGripperRect.right;
		}

	ENSURE( ui::IsNormalized( rGripperRect ) );
	ENSURE( ui::IsNormalized( rFirstRect ) );
	ENSURE( ui::IsNormalized( rSecondRect ) );
}

CRect CResizeGripBar::ComputeMouseTrapRect( const CSize& trackOffset ) const
{
	// percentages (negative) must have been evaluated by now
	REQUIRE( m_firstMinExtent >= 0 && m_secondMinExtent >= 0 );

	CRect mouseTrapRect;
	m_pResizeFrame->GetWindowRect( &mouseTrapRect );

	if ( ResizeUpDown == m_orientation )
	{
		mouseTrapRect.top += m_firstMinExtent;
		mouseTrapRect.bottom -= ( m_secondMinExtent + m_windowDepth );

		mouseTrapRect.OffsetRect( 0, trackOffset.cy );
	}
	else
	{
		mouseTrapRect.left += m_firstMinExtent;
		mouseTrapRect.right -= ( m_secondMinExtent + m_windowDepth );

		mouseTrapRect.OffsetRect( trackOffset.cy, 0 );
	}

	ENSURE( ui::IsNormalized( mouseTrapRect ) );
	return mouseTrapRect;
}

CResizeGripBar::CDrawAreas CResizeGripBar::GetDrawAreas( void ) const
{
	CDrawAreas areas;
	GetClientRect( &areas.m_clientRect );

	areas.m_gripRect = areas.m_clientRect;

    int arrowExtent = GetArrowExtent(), arrowSpacing = arrowExtent;

	if ( NoToggle == m_toggleStyle )
		areas.m_arrowRect1 = areas.m_arrowRect2 = CRect( 0, 0, 0, 0 );
	else
		if ( ResizeUpDown == m_orientation )
		{
			areas.m_gripRect.DeflateRect( arrowExtent + arrowSpacing * 2, 0 );

			areas.m_arrowRect1 = areas.m_arrowRect2 = areas.m_clientRect;
			areas.m_arrowRect1.right = areas.m_gripRect.left;
			areas.m_arrowRect2.left = areas.m_gripRect.right;
		}
		else
		{
			areas.m_gripRect.DeflateRect( 0, arrowExtent + arrowSpacing * 2 );

			areas.m_arrowRect1 = areas.m_arrowRect2 = areas.m_clientRect;
			areas.m_arrowRect1.bottom = areas.m_gripRect.top;
			areas.m_arrowRect2.top = areas.m_gripRect.bottom;
		}

	return areas;
}

CResizeGripBar::HitTest CResizeGripBar::GetHitTest( const CDrawAreas& rAreas, const CPoint& clientPos ) const
{
	if ( rAreas.m_gripRect.PtInRect( clientPos ) )
		return m_isCollapsed ? ToggleArrow : GripBar;

	if ( m_toggleStyle != NoToggle )
		if ( rAreas.m_arrowRect1.PtInRect( clientPos ) || rAreas.m_arrowRect2.PtInRect( clientPos ) )
			return ToggleArrow;

	return Nowhere;
}

CResizeGripBar::HitTest CResizeGripBar::GetMouseHitTest( const CDrawAreas& rAreas ) const
{
	CPoint mousePos;
	::GetCursorPos( &mousePos );
	ScreenToClient( &mousePos );

	return GetHitTest( rAreas, mousePos );
}

bool CResizeGripBar::SetHitOn( HitTest hitOn )
{
	if ( hitOn == m_hitOn )
		return false;

	m_hitOn = hitOn;
	Invalidate();
	return true;
}

void CResizeGripBar::DrawGripBar( CDC* pDC, const CRect& rectZone )
{
	CRect gripRect = rectZone;
	bool horizontalBar = rectZone.Width() > rectZone.Height();

	if ( horizontalBar )
		gripRect.DeflateRect( 0, GripSpacing );
	else
		gripRect.DeflateRect( GripSpacing, 0 );

	CThemeItem gripperTheme( L"REBAR", horizontalBar ? RP_GRIPPER : RP_GRIPPERVERT );

	gripperTheme.DrawBackground( *pDC, gripRect );
}

void CResizeGripBar::DrawArrow( CDC* pDC, const CRect& rect, ArrowPart part, ArrowState state )
{
	CRect arrowRect( rect.TopLeft(), m_arrowSize );
	ui::CenterRect( arrowRect, rect );

	int imageIndex = GetImageIndex( part, state );
	m_arrowImageList.Draw( pDC, imageIndex, arrowRect.TopLeft(), ILD_TRANSPARENT );
}


BEGIN_MESSAGE_MAP( CResizeGripBar, CStatic )
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE( WM_MOUSELEAVE, OnMouseLeave )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CResizeGripBar::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	pWnd, hitTest, message;
	if ( GripBar == m_hitOn )
	{
		::SetCursor( s_hCursors[ m_orientation ] );
		return TRUE;
	}

	return __super::OnSetCursor( pWnd, hitTest, message );
}

void CResizeGripBar::OnLButtonDown( UINT flags, CPoint point )
{
	__super::OnLButtonDown( flags, point );

	ASSERT_NULL( m_pTrackingInfo );

	if ( GripBar == m_hitOn )
	{
		CPoint trackPos = point;
		ClientToScreen( &trackPos );

		CRect gripperRect;
		GetWindowRect( &gripperRect );

		CSize trackOffset = trackPos - gripperRect.TopLeft();

		SetCapture();

		m_pTrackingInfo = new CTrackingInfo( trackOffset, trackPos, ComputeMouseTrapRect( trackOffset ) );
	}
}

void CResizeGripBar::OnLButtonUp( UINT flags, CPoint point )
{
	enum Event { None, EndResize, Toggle } event = None;

	if ( m_pTrackingInfo != NULL )
	{
		if ( ::GetCapture() == m_hWnd )
			ReleaseCapture();

		if ( m_pTrackingInfo->m_wasDragged )
			event = EndResize;
		else if ( m_toggleStyle != NoToggle )
			event = Toggle;

		delete m_pTrackingInfo;
		m_pTrackingInfo = NULL;
	}
	else if ( ToggleArrow == m_hitOn )
		event = Toggle;

	__super::OnLButtonUp( flags, point );

	switch ( event )
	{
		case EndResize:
			m_pResizeFrame->NotifyParent( CResizeFrameStatic::RF_GRIPPER_RESIZED );
			break;
		case Toggle:
			SetCollapsed( !m_isCollapsed );
			m_pResizeFrame->NotifyParent( CResizeFrameStatic::RF_GRIPPER_TOGGLE );
			break;
	}
}

void CResizeGripBar::OnMouseMove( UINT flags, CPoint point )
{
	__super::OnMouseMove( flags, point );

	SetHitOn( GetHitTest( GetDrawAreas(), point ) );

	if ( m_pTrackingInfo != NULL )
	{
		CPoint screenTrackPos = point;

		ClientToScreen( &screenTrackPos );
		TrackToPos( screenTrackPos );
		m_pResizeFrame->NotifyParent( CResizeFrameStatic::RF_GRIPPER_RESIZING );
	}
	else
	{
		TRACKMOUSEEVENT trackInfo = { sizeof( TRACKMOUSEEVENT ), TME_LEAVE, m_hWnd, HOVER_DEFAULT };
		::_TrackMouseEvent( &trackInfo );
	}
}

LRESULT CResizeGripBar::OnMouseLeave( WPARAM, LPARAM )
{
	SetHitOn( Nowhere );
	return TRUE;
}

BOOL CResizeGripBar::OnEraseBkgnd( CDC* pDC )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	CBrush brush( Nowhere == m_hitOn ? GetSysColor( COLOR_3DLIGHT ) : ui::GetHotBkColor() );

	if ( !m_isCollapsed )
		pDC->FillRect( &clientRect, &brush );
	else
	{
		pDC->DrawFrameControl( &clientRect, DFC_BUTTON, DFCS_BUTTONPUSH );

		if ( m_hitOn != Nowhere )
		{
			clientRect.DeflateRect( 1, 1 );
			pDC->FillRect( &clientRect, &brush );
		}
	}

	return TRUE;			// erased
}


CResizeGripBar::ArrowPart CResizeGripBar::GetArrowPart( void ) const
{
	if ( ToggleSecond == m_toggleStyle )
		return m_isCollapsed ? Collapsed : Expanded;
	else
		return m_isCollapsed ? Expanded : Collapsed;
}

CResizeGripBar::ArrowState CResizeGripBar::GetArrowState( HitTest hitOn ) const
{
	if ( !IsWindowEnabled() )
		return Disabled;

	return hitOn == ToggleArrow ? Hot : Normal;
}

int CResizeGripBar::GetImageIndex( ArrowPart part, ArrowState state ) const
{
	int imageIndex = ( 2 * state ) + part;
	return imageIndex;
}

void CResizeGripBar::OnPaint( void )
{
	CPaintDC dc( this );

	CDrawAreas areas = GetDrawAreas();
	HitTest hitOn = GetMouseHitTest( areas );

	if ( !m_isCollapsed )
		DrawGripBar( &dc, areas.m_gripRect );

	if ( m_toggleStyle != NoToggle )
	{
		ArrowPart part = GetArrowPart();
		ArrowState state = GetArrowState( hitOn );

		DrawArrow( &dc, areas.m_arrowRect1, part, state );
		DrawArrow( &dc, areas.m_arrowRect2, part, state );
	}

	if ( !m_isCollapsed )
		if ( m_hasBorder )
			dc.Draw3dRect( &areas.m_clientRect, GetSysColor( COLOR_BTNHIGHLIGHT ), GetSysColor( COLOR_BTNSHADOW ) );
}


// CResizeGripBar::CTrackingInfo implementation

CResizeGripBar::CTrackingInfo::CTrackingInfo( const CSize& trackOffset, const CPoint& trackPos, const CRect& mouseTrapRect )
	: m_trackOffset( trackOffset )
	, m_trackPos( trackPos )
	, m_mouseTrapRect( mouseTrapRect )
	, m_wasDragged( false )
{
	GetClipCursor( &m_oldMouseTrapRect );
	ClipCursor( &m_mouseTrapRect );
}
