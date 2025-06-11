
#include "pch.h"
#include "ResizeGripBar.h"
#include "ResizeFrameStatic.h"
#include "Color.h"
#include "LayoutMetrics.h"
#include "WndUtils.h"
#include "ThemeItem.h"
#include "resource.h"
#include <afxcontrolbarutil.h>		// for CMemDC

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	bool ShowPaneWindow( CWnd* pPaneWnd, bool show /*= true*/ )
	{
		if ( ui::ILayoutFrame* pLayoutFrame = dynamic_cast<ui::ILayoutFrame*>( pPaneWnd ) )
			return pLayoutFrame->ShowPane( show );

		return ui::ShowWindow( pPaneWnd->GetSafeHwnd(), show );
	}
}


// CResizeGripBar class

HCURSOR CResizeGripBar::s_hCursors[ 2 ] = { nullptr, nullptr };


CResizeGripBar::CResizeGripBar( CWnd* pFirstCtrl, CWnd* pSecondCtrl, resize::Orientation orientation, resize::ToggleStyle toggleStyle /*= resize::ToggleSecond*/ )
	: CStatic()
	, m_layout( orientation )
	, m_toggleStyle( toggleStyle )
	, m_panelCtrls( pFirstCtrl, pSecondCtrl )
	, m_paneSpacing( 0, 0 )

	, m_pResizeFrame( nullptr )
	, m_windowDepth( 0 )
	, m_arrowSize( 0, 0 )
	, m_hitOn( Nowhere )
	, m_pTrackingInfo( nullptr )
{
	REQUIRE( m_panelCtrls.first != nullptr && m_panelCtrls.second != nullptr );

	if ( nullptr == s_hCursors[ resize::NorthSouth ] )
	{
		s_hCursors[ resize::NorthSouth ] = AfxGetApp()->LoadCursor( IDC_RESIZE_SPLITTER_NS_CURSOR );
		s_hCursors[ resize::WestEast ] = AfxGetApp()->LoadCursor( IDC_RESIZE_SPLITTER_WE_CURSOR );
	}

	StoreSplitterGripBar( m_panelCtrls.first );
	StoreSplitterGripBar( m_panelCtrls.second );
}

CResizeGripBar::~CResizeGripBar()
{
	delete m_pTrackingInfo;
}

void CResizeGripBar::StoreSplitterGripBar( CWnd* pPaneWnd )
{
	if ( ui::ILayoutFrame* pPaneLayoutFrame = dynamic_cast<ui::ILayoutFrame*>( pPaneWnd ) )
		pPaneLayoutFrame->SetSplitterGripBar( this );		// will be used for initial hidden state of pane's layout controls
}

CWnd* CResizeGripBar::GetCollapsiblePane( void ) const
{
	switch ( m_toggleStyle )
	{
		case resize::ToggleFirst:
			return m_panelCtrls.first;
		case resize::ToggleSecond:
			return m_panelCtrls.second;
	}

	return nullptr;
}

CResizeGripBar& CResizeGripBar::SetMinExtents( TValueOrPct firstMinExtent, TValueOrPct secondMinExtent )
{
	m_layout.m_minExtents = std::make_pair( firstMinExtent, secondMinExtent );
	return *this;
}

void CResizeGripBar::CreateArrowsImageList( void )
{
	ASSERT_NULL( m_arrowImageList.GetSafeHandle() );

	CBitmap bitmapArrows;
	UINT bitmapResId = resize::NorthSouth == m_layout.m_orientation ? IDB_RESIZE_DOWN_UP_BITMAP : IDB_RESIZE_RIGHT_LEFT_BITMAP;

	// load state: Normal (and compute arrow-dependent sizes)
	m_arrowSize = LoadArrowsBitmap( &bitmapArrows, bitmapResId, GetHotArrowColor() );
	m_windowDepth = GetArrowDepth() + DepthSpacing * 2;
	VERIFY( m_arrowImageList.Create( m_arrowSize.cx, m_arrowSize.cy, ILC_COLORDDB | ILC_MASK, 0, 2 ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGray );

	// load state: Hot
	LoadArrowsBitmap( &bitmapArrows, bitmapResId, GetSysColor( COLOR_BTNTEXT ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGray );

	// load state: Disabled
	LoadArrowsBitmap( &bitmapArrows, bitmapResId, GetSysColor( COLOR_GRAYTEXT ) );
	m_arrowImageList.Add( &bitmapArrows, color::LightGray );
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

bool CResizeGripBar::CreateGripper( CResizeFrameStatic* pResizeFrame, UINT id /*= 0xFFFF*/ )
{
	m_pResizeFrame = pResizeFrame;
	ASSERT_PTR( m_pResizeFrame->GetSafeHwnd() );
	ASSERT_PTR( m_panelCtrls.first->GetSafeHwnd() );
	ASSERT_PTR( m_panelCtrls.second->GetSafeHwnd() );

	ComputeInitialMetrics();

	static const TCHAR* s_caption[] = { _T("<UpDown grip-bar>"), _T("<LeftRight grip-bar>") };

	// Note: WS_CLIPSIBLINGS style is important: it prevents dirty drawing when collapsing embedded resize frames

	if ( !Create( s_caption[ m_layout.m_orientation ], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_NOTIFY, CRect( 0, 0, 0, 0 ), m_pResizeFrame->GetParent(), id ) )
		return false;

	ASSERT_NULL( m_pTrackingInfo );

	LayoutProportionally();
	return true;
}

COLORREF CResizeGripBar::GetHotArrowColor( void )
{
	return ui::GetAdjustLuminance( ::GetSysColor( COLOR_HOTLIGHT ), 30 );					// contrast blue
}

void CResizeGripBar::ComputeInitialMetrics( void )
{
	std::pair<CRect, CRect> rectPair;

	m_panelCtrls.first->GetWindowRect( &rectPair.first );
	m_panelCtrls.second->GetWindowRect( &rectPair.second );

	// if minimum extents are percentages, evaluate them to the actual limits
	if ( ui::IsPercentage( m_layout.m_minExtents.first ) )
		m_layout.m_minExtents.first = ui::EvalValueOrPercentage( m_layout.m_minExtents.first, GetRectExtent( rectPair.first ) );

	if ( ui::IsPercentage( m_layout.m_minExtents.second ) )
		m_layout.m_minExtents.second = ui::EvalValueOrPercentage( m_layout.m_minExtents.second, GetRectExtent( rectPair.second ) );

	if ( 0 == m_windowDepth )
	{	// default depth is the distance between first and second windows
		if ( m_layout.m_orientation == resize::NorthSouth )
			m_windowDepth = rectPair.second.top - rectPair.first.bottom;
		else
			m_windowDepth = rectPair.second.left - rectPair.first.right;
	}

	if ( -1 == GetFirstExtentPercentage() )
	{	// take it from the current size of the first window versus frame window
		CFrameLayoutInfo info;
		ReadLayoutInfo( info );

		int firstExtent = GetRectExtent( rectPair.first );

		LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );

		m_layout.m_firstExtentPercentage = ui::GetPercentageOf( firstExtent, info.m_maxExtent );
		ENSURE( ui::IsPercentage_0_100( m_layout.m_firstExtentPercentage ) );
	}
}

CResizeGripBar& CResizeGripBar::SetFirstExtentPercentage( TPercent firstExtentPercentage )
{
	REQUIRE( ui::IsPercentage_0_100( firstExtentPercentage ) );

	if ( m_hWnd != nullptr )
	{
		CFrameLayoutInfo info;
		ReadLayoutInfo( info );

		int firstExtent = ui::ScaleValue( info.m_maxExtent, firstExtentPercentage );				// convert to absolute extent

		LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );
		m_layout.m_firstExtentPercentage = ui::GetPercentageOf( firstExtent, info.m_maxExtent );	// convert back to percentage

		LayoutProportionally();
	}
	else
		m_layout.m_firstExtentPercentage = firstExtentPercentage;

	return *this;
}

CResizeGripBar& CResizeGripBar::SetCollapsed( bool collapsed )
 {
	REQUIRE( m_toggleStyle != resize::NoToggle );

	m_layout.m_isCollapsed = collapsed;

	if ( m_hWnd != nullptr )
	{
		LayoutProportionally();

		ui::RedrawDialog( GetParent()->GetSafeHwnd() );		// prevent clipping issues when toggling a frame control that contains (overlaps) group-boxes
	}

	return *this;
}

void CResizeGripBar::LayoutProportionally( bool repaint /*= true*/ )
{
	ASSERT_PTR( m_hWnd );

	CFrameLayoutInfo info;
	ReadLayoutInfo( info );

	// computation is driven from info.m_frameRect and preserves m_firstExtentPercentage
	int firstExtent = ui::ScaleValue( info.m_maxExtent, m_layout.m_firstExtentPercentage );		// convert to absolute extent

	LimitFirstExtentToBounds( firstExtent, info.m_maxExtent );

	LayoutGripperTo( info, firstExtent, repaint );
}

void CResizeGripBar::LayoutGripperTo( const CFrameLayoutInfo& info, const int firstExtent, bool repaint /*= true*/ )
{
	ASSERT_PTR( m_hWnd );

	CRect gripperRect, firstRect, secondRect;
	ComputeLayoutRects( gripperRect, firstRect, secondRect, info, firstExtent );

	if ( !IsTracking() )		// avoid toggling pane visibility during tracking, since it leads to partial background erasing for group boxes in pane frames
	{
		if ( resize::ToggleFirst == m_toggleStyle )
			layout::ShowPaneWindow( m_panelCtrls.first, !m_layout.m_isCollapsed || resize::ToggleSecond == m_toggleStyle );

		if ( resize::ToggleSecond == m_toggleStyle )
			layout::ShowPaneWindow( m_panelCtrls.second, !m_layout.m_isCollapsed || resize::ToggleFirst == m_toggleStyle );
	}

	layout::MoveControl( *m_panelCtrls.first, firstRect, repaint );
	layout::MoveControl( *m_panelCtrls.second, secondRect, repaint );

	// move lastly to prevent dirty painting issues
	this->MoveWindow( &gripperRect, false );

	if ( repaint )
		ui::RedrawControl( m_hWnd );
}

bool CResizeGripBar::TrackToPos( CPoint screenTrackPos )
{
	ASSERT_PTR( m_pTrackingInfo );

	CSize deltaPos = screenTrackPos - m_pTrackingInfo->m_trackPos;
	int trackDelta = resize::NorthSouth == m_layout.m_orientation ? deltaPos.cy : deltaPos.cx;

	if ( 0 == trackDelta )
		return false;

	m_pTrackingInfo->m_trackPos = screenTrackPos;
	m_pTrackingInfo->m_wasDragged = true;

	CFrameLayoutInfo info;
	ReadLayoutInfo( info );

	CRect rectFirstPane;
	m_panelCtrls.first->GetWindowRect( &rectFirstPane );

	int firstExtent = GetRectExtent( rectFirstPane ) + m_paneSpacing.first;

	// advance to track position
	firstExtent += trackDelta;

	if ( !LimitFirstExtentToBounds( firstExtent, info.m_maxExtent ) )
		return false;

	m_layout.m_firstExtentPercentage = ui::GetPercentageOf( firstExtent, info.m_maxExtent );			// convert back to percentage

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
	REQUIRE( !ui::IsPercentage( m_layout.m_minExtents.first ) && !ui::IsPercentage( m_layout.m_minExtents.second ) );		// should be evaluated by now

	int oldFirstExtent = rFirstExtent;

	if ( rFirstExtent < m_layout.m_minExtents.first )
		rFirstExtent = m_layout.m_minExtents.first;

	int secondExtent = maxExtent - rFirstExtent;

	if ( secondExtent < m_layout.m_minExtents.second )
		rFirstExtent -= ( m_layout.m_minExtents.second - secondExtent );

	ENSURE( rFirstExtent >= m_layout.m_minExtents.first && rFirstExtent <= maxExtent );

	return rFirstExtent == oldFirstExtent;		// false if constrained
}

void CResizeGripBar::ComputeLayoutRects( CRect& rGripperRect, CRect& rFirstRect, CRect& rSecondRect, const CFrameLayoutInfo& info,
										 const int firstExtent ) const
{
	// computation is driven from frameRect using firstExtent

	rGripperRect = rFirstRect = rSecondRect = info.m_frameRect;		// start with all rects to frame

	CWnd* pHiddenPane = nullptr;			// collapsed pane

	if ( m_layout.m_isCollapsed )
		pHiddenPane = resize::ToggleFirst == m_toggleStyle ? m_panelCtrls.first : m_panelCtrls.second;

	if ( resize::NorthSouth == m_layout.m_orientation )
	{
		if ( pHiddenPane == m_panelCtrls.first )
		{	// first collapsed  - gripper at top
			rFirstRect.bottom = rFirstRect.top + firstExtent;
			rGripperRect.bottom = rGripperRect.top + m_windowDepth;
			rSecondRect.top = rGripperRect.bottom;
		}
		else if ( pHiddenPane == m_panelCtrls.second )
		{	// second collapsed - gripper at bottom
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

		// ensure panes spacing around the gripper bar
		rFirstRect.bottom -= m_paneSpacing.first;
		rSecondRect.top += m_paneSpacing.second;

		rFirstRect.bottom = std::max( rFirstRect.top, rFirstRect.bottom );		// limit to top to keep it normalized
		rSecondRect.top = std::min( rSecondRect.bottom, rSecondRect.top );		// limit to bottom to keep it normalized
	}
	else
	{
		if ( pHiddenPane == m_panelCtrls.first )
		{	// first collapsed  - gripper at left
			rFirstRect.right = rFirstRect.left + firstExtent;
			rGripperRect.right = rGripperRect.left + m_windowDepth;
			rSecondRect.left = rGripperRect.right;
		}
		else if ( pHiddenPane == m_panelCtrls.second )
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
	}

	ENSURE( ui::IsNormalized( rGripperRect ) );
	ENSURE( ui::IsNormalized( rFirstRect ) );
	ENSURE( ui::IsNormalized( rSecondRect ) );
}

CRect CResizeGripBar::ComputeMouseTrapRect( const CSize& trackOffset ) const
{
	// percentages (negative) must have been evaluated by now
	REQUIRE( m_layout.m_minExtents.first >= 0 && m_layout.m_minExtents.second >= 0 );

	CRect mouseTrapRect;
	m_pResizeFrame->GetWindowRect( &mouseTrapRect );

	if ( resize::NorthSouth == m_layout.m_orientation )
	{
		mouseTrapRect.top += m_layout.m_minExtents.first;
		mouseTrapRect.bottom -= ( m_layout.m_minExtents.second + m_windowDepth );

		mouseTrapRect.OffsetRect( 0, trackOffset.cy );
	}
	else
	{
		mouseTrapRect.left += m_layout.m_minExtents.first;
		mouseTrapRect.right -= ( m_layout.m_minExtents.second + m_windowDepth );

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

    int arrowExtent = GetArrowExtent();
    int arrowSpacing = arrowExtent;

	if ( resize::NoToggle == m_toggleStyle )
		areas.m_arrowRect1 = areas.m_arrowRect2 = CRect( 0, 0, 0, 0 );
	else
		if ( resize::NorthSouth == m_layout.m_orientation )
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
		return IsCollapsed() ? ToggleArrow : GripBar;

	if ( m_toggleStyle != resize::NoToggle )
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


void CResizeGripBar::Draw( CDC* pDC, const CRect& clientRect )
{
	DrawBackground( pDC, clientRect );

	CDrawAreas areas = GetDrawAreas();
	HitTest hitOn = GetMouseHitTest( areas );

	if ( !IsCollapsed() )
		DrawGripBar( pDC, areas.m_gripRect );

	if ( m_toggleStyle != resize::NoToggle )
	{
		ArrowPart part = GetArrowPart();
		ArrowState state = GetArrowState( hitOn );

		DrawArrow( pDC, areas.m_arrowRect1, part, state );
		DrawArrow( pDC, areas.m_arrowRect2, part, state );
	}

	if ( !IsCollapsed() )
		if ( m_layout.m_hasBorder )
			pDC->Draw3dRect( &areas.m_clientRect, GetSysColor( COLOR_BTNHIGHLIGHT ), GetSysColor( COLOR_BTNSHADOW ) );
}

void CResizeGripBar::DrawBackground( CDC* pDC, const CRect& clientRect )
{
	CBrush brush( Nowhere == m_hitOn ? GetSysColor( COLOR_3DLIGHT ) : HotCyan );
	CRect rect = clientRect;

	if ( !IsCollapsed() )
		pDC->FillRect( &rect, &brush );
	else
	{
		pDC->Draw3dRect( &rect, GetSysColor( COLOR_BTNHIGHLIGHT ), MildGray );

		rect.DeflateRect( 1, 1 );
		pDC->FillRect( &rect, &brush );
	}
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

CResizeGripBar::ArrowPart CResizeGripBar::GetArrowPart( void ) const
{
	if ( resize::ToggleSecond == m_toggleStyle )
		return m_layout.m_isCollapsed ? Collapsed : Expanded;
	else
		return m_layout.m_isCollapsed ? Expanded : Collapsed;
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

void CResizeGripBar::PreSubclassWindow( void )
{
	CreateArrowsImageList();

	__super::PreSubclassWindow();
}


// CResizeGripBar messages

BEGIN_MESSAGE_MAP( CResizeGripBar, CStatic )
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE( WM_MOUSELEAVE, OnMouseLeave )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CResizeGripBar::OnEraseBkgnd( CDC* pDC )
{
	pDC;
	return TRUE;			// erased, no default erasing
}

void CResizeGripBar::OnPaint( void )
{
	CPaintDC paintDC( this );

	// draw background + foreground using double buffering
	CMemDC memDC( paintDC, this );
	CDC* pDC = &memDC.GetDC();

	CRect clientRect;
	GetClientRect( &clientRect );

	Draw( pDC, clientRect );	// background + content
}

BOOL CResizeGripBar::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	pWnd, hitTest, message;
	if ( GripBar == m_hitOn )
	{
		::SetCursor( s_hCursors[ m_layout.m_orientation ] );
		return TRUE;
	}

	return __super::OnSetCursor( pWnd, hitTest, message );
}

void CResizeGripBar::OnLButtonDown( UINT flags, CPoint point )
{
	__super::OnLButtonDown( flags, point );

	if ( IsTracking() )
	{	// can happen while debugging layout, and hitting a breakpoint; we need to exit tracking mode first
		if ( ::GetCapture() == m_hWnd )
			ReleaseCapture();

		delete m_pTrackingInfo;
		m_pTrackingInfo = nullptr;
	}
	ENSURE( !IsTracking() );

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

	if ( IsTracking() )
	{
		if ( ::GetCapture() == m_hWnd )
			ReleaseCapture();

		if ( m_pTrackingInfo->m_wasDragged )
			event = EndResize;
		else if ( m_toggleStyle != resize::NoToggle )
			event = Toggle;

		delete m_pTrackingInfo;
		m_pTrackingInfo = nullptr;
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
			SetCollapsed( !IsCollapsed() );
			m_pResizeFrame->NotifyParent( CResizeFrameStatic::RF_GRIPPER_TOGGLE );
			break;
	}
}

void CResizeGripBar::OnMouseMove( UINT flags, CPoint point )
{
	__super::OnMouseMove( flags, point );

	CDrawAreas drawAreas = GetDrawAreas();			// PHC: use temporary object, because otherwise it crashes unexpectedly on 64-bit Release build (?!)
	SetHitOn( GetHitTest( drawAreas, point ) );

	if ( m_pTrackingInfo != nullptr )
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
