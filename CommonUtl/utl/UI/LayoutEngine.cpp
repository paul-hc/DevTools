
#include "pch.h"
#include "LayoutEngine.h"
#include "ResizeGripBar.h"
#include "MemoryDC.h"
#include "ScopedValue.h"
#include "WndUtils.h"
#include "utl/Algorithms.h"
#include <afxext.h>				// for CFormView

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLayoutEngine implementation

int CLayoutEngine::s_defaultFlags = Smooth;


CLayoutEngine::CLayoutEngine( int flags /*= s_defaultFlags*/ )
	: m_layoutType( DialogLayout )
	, m_flags( flags )
	, m_layoutEnabled( true )
	, m_pDialog( nullptr )
	, m_dlgTemplClientRect( 0, 0, 0, 0 )
	, m_maxClientSize( INT_MAX, INT_MAX )
	, m_nonClientSize( 0, 0 )
	, m_prevClientRect( 0, 0, 0, 0 )
	, m_collapsedDelta( 0, 0 )
	, m_collapsed( false )
{
}

CLayoutEngine::~CLayoutEngine()
{
}

void CLayoutEngine::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], UINT count )
{
	for ( UINT i = 0; i != count; ++i )
	{
		const CLayoutStyle* pStyle = &layoutStyles[i];

		ASSERT( pStyle->m_ctrlId != 0 );
		m_controlStates[ pStyle->m_ctrlId ].SetLayoutStyle( pStyle->m_layoutStyle, false );
	}
}

void CLayoutEngine::RegisterDualCtrlLayout( const CDualLayoutStyle dualLayoutStyles[], UINT count )
{
	for ( UINT i = 0; i != count; ++i )
	{
		ASSERT( dualLayoutStyles[ i ].m_ctrlId != 0 );
		layout::CControlState& rCtrlState = m_controlStates[ dualLayoutStyles[ i ].m_ctrlId ];

		rCtrlState.SetLayoutStyle( dualLayoutStyles[ i ].m_expandedStyle, false );
		rCtrlState.SetLayoutStyle( dualLayoutStyles[ i ].m_collapsedStyle != layout::UseExpanded
			? dualLayoutStyles[ i ].m_collapsedStyle : dualLayoutStyles[ i ].m_expandedStyle, true );
	}
}

void CLayoutEngine::Reset( void )
{
	m_pDialog = nullptr;

	m_dlgTemplClientRect.SetRectEmpty();
	m_maxClientSize.cx = m_maxClientSize.cy = INT_MAX;
	m_nonClientSize.cx = m_nonClientSize.cy = 0;
	m_prevClientRect.SetRectEmpty();

	for ( std::unordered_map<UINT, layout::CControlState>::iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
		itCtrlState->second.ResetCtrl();

	m_hiddenGroups.clear();
}

void CLayoutEngine::CreateResizeGripper( const CSize& offset /*= CSize( 1, 0 )*/ )
{
	ASSERT_NULL( m_pGripper.get() );		// create once
	m_pGripper.reset( new layout::CResizeGripper( m_pDialog, offset ) );
}

layout::CResizeGripper* CLayoutEngine::GetResizeGripper( void ) const
{
	return IsInitialized() && !m_pDialog->IsZoomed() ? m_pGripper.get() : nullptr;
}

void CLayoutEngine::StoreInitialSize( CWnd* pDialog )
{
	CRect clientRect, windowRect;
	pDialog->GetClientRect( &clientRect );
	pDialog->GetWindowRect( &windowRect );

	CSize minClientSize = clientRect.Size();

	// store initial size of the dialog
	m_dlgTemplClientRect = clientRect;
	m_nonClientSize = windowRect.Size() - minClientSize;

	m_maxClientSize.cx = std::max( m_maxClientSize.cx, minClientSize.cx );
	m_maxClientSize.cy = std::max( m_maxClientSize.cy, minClientSize.cy );

	m_prevClientRect = clientRect;
}

void CLayoutEngine::Initialize( CWnd* pDialog )
{
	REQUIRE( !IsInitialized() );

	m_pDialog = pDialog;

	ENSURE( ::IsWindow( m_pDialog->GetSafeHwnd() ) );
	ENSURE( ui::IsDialogBox( m_pDialog->GetSafeHwnd() ) );

	if ( !HasInitialSize() )
		StoreInitialSize( m_pDialog );

	ENSURE( HasInitialSize() );		// should have a size by this time

	SetupControlStates();
}

void CLayoutEngine::SetupControlStates( void )
{
	ASSERT( IsInitialized() );

	if ( !HasCtrlLayout() )
		return;

	ASSERT( m_controlStates.empty() || !m_controlStates.begin()->second.IsCtrlInit() );		// initialize once

	bool anyRepaintCtrl = false;

	// for each control state:
	for ( std::unordered_map<UINT, layout::CControlState>::const_iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
	{
		if ( HasFlag( itCtrlState->second.GetLayoutStyle( false ), layout::DoRepaint ) )
			anyRepaintCtrl = true;

		if ( HasFlag( itCtrlState->second.GetLayoutStyle( false ), layout::CollapsedMask ) )
			SetupCollapsedState( itCtrlState->first, itCtrlState->second.GetLayoutStyle( false ) );
	}

	if ( anyRepaintCtrl )
	{	// disable SmoothGroups and WS_CLIPCHILDREN when one control requires repaint (layout::DoRepaint)
		ModifyFlags( SmoothGroups, GroupsTransparent | GroupsRepaint );
		m_pDialog->ModifyStyle( WS_CLIPCHILDREN, 0 );
	}

	if ( HasFlag( m_flags, SmoothGroups ) )
		m_pDialog->ModifyStyle( 0, WS_CLIPCHILDREN );		// add the WS_CLIPCHILDREN on dialog for smooth repainting

	// for each control: setup controls' state, and handle group boxes
	for ( HWND hCtrl = ::GetWindow( m_pDialog->GetSafeHwnd(), GW_CHILD ); hCtrl != nullptr; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
		if ( UINT ctrlId = ::GetDlgCtrlID( hCtrl ) )		// ignore child dialogs in a property sheet
			if ( layout::CControlState* pCtrlState = LookupControlState( ctrlId ) )		// only touch controls with layout defined
			{
				//if ( DialogLayout == m_layoutType )		// commented-out: avoid clipping errors on group controls (e.g. in CToolbarImagesDialog)
				if ( ui::IsGroupBox( hCtrl ) )
					SetupGroupBoxState( hCtrl, pCtrlState );

				pCtrlState->ResetCtrl( hCtrl );
			}
}

void CLayoutEngine::SetupGroupBoxState( HWND hGroupBox, layout::CControlState* pCtrlState )
{
	//TRACE( _T(" CLayoutEngine::SetupGroupBoxState() - GroupBox='%s'\n"), ui::GetWindowText( hGroupBox ).c_str() );

	if ( HasFlag( m_flags, SmoothGroups ) && !HasFlag( m_flags, GroupsTransparentEx ) )
	{
		// hide group boxes and clear transparent ex-style; will be rendered on WM_ERASEBKGND
		CWnd::ModifyStyle( hGroupBox, WS_VISIBLE, 0, 0 );
		CWnd::ModifyStyleEx( hGroupBox, WS_EX_TRANSPARENT, 0, 0 );
		m_hiddenGroups.push_back( hGroupBox );
	}
	else
	{
		if ( HasFlag( m_flags, GroupsRepaint ) && pCtrlState != nullptr )
			pCtrlState->ModifyLayoutStyle( 0, layout::DoRepaint );		// force groups repaint

		if ( HasFlag( m_flags, GroupsTransparent | GroupsTransparentEx ) )
		{
			REQUIRE( !HasFlag( m_flags, GroupsTransparentEx ) || HasFlag( m_pDialog->GetStyle(), WS_CLIPCHILDREN ) );	// GroupsTransparentEx mode requires WS_CLIPCHILDREN for parent dialog!

			// make group boxes transparent (z-order fix for proxy controls);
			// prevents the WS_CLIPCHILDREN styled parent window from excluding the groupbox's background region, allowing the background to paint;
			// while this works, it creates annoying flicker on resize.
			if ( !HasFlag( ui::GetStyleEx( hGroupBox ), WS_EX_TRANSPARENT ) )
			{
				CWnd::ModifyStyleEx( hGroupBox, 0, WS_EX_TRANSPARENT, 0 );
			}
		}
	}
}

void CLayoutEngine::SetupCollapsedState( UINT ctrlId, layout::TStyle layoutStyle )
{
	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	CRect anchorRect = ui::GetControlRect( ::GetDlgItem( m_pDialog->m_hWnd, ctrlId ) );

	ASSERT( 0 == m_collapsedDelta.cx && 0 == m_collapsedDelta.cy );					// initialize once
	if ( HasFlag( layoutStyle, layout::CollapsedLeft ) )
		m_collapsedDelta.cx = clientRect.right - anchorRect.left;

	if ( HasFlag( layoutStyle, layout::CollapsedTop ) )
		m_collapsedDelta.cy = clientRect.bottom - anchorRect.top;
}

bool CLayoutEngine::AnyRepaintCtrl( void ) const
{
	for ( std::unordered_map<UINT, layout::CControlState>::const_iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
		if ( HasFlag( itCtrlState->second.GetLayoutStyle( false ), layout::DoRepaint ) )
			return true;

	return false;
}

void CLayoutEngine::SetLayoutEnabled( bool layoutEnabled /*= true*/ )
{
	m_layoutEnabled = layoutEnabled;

	if ( m_layoutEnabled )
		LayoutControls();
	else
	{
		for ( std::unordered_map<UINT, layout::CControlState>::iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
			itCtrlState->second.ResetCtrl();
	}
}

CSize CLayoutEngine::GetMinClientSize( bool collapsed ) const
{
	CSize minClientSize = m_dlgTemplClientRect.Size();

	if ( collapsed )
		minClientSize -= m_collapsedDelta;

	return minClientSize;
}

CSize CLayoutEngine::GetMaxClientSize( void ) const
{
	CSize maxClientSize = m_maxClientSize;

	if ( m_collapsed )
	{
		if ( maxClientSize.cx != INT_MAX )
			maxClientSize.cx -= m_collapsedDelta.cx;

		if ( maxClientSize.cy != INT_MAX )
			maxClientSize.cy -= m_collapsedDelta.cy;
	}

	return maxClientSize;
}

bool CLayoutEngine::SetCollapsed( bool collapsed )
{
	ASSERT( !collapsed || IsCollapsible() );

	if ( m_collapsed == collapsed )
		return false;

	ASSERT( IsInitialized() );
	m_collapsed = collapsed;

	if ( !m_pDialog->IsZoomed() )
	{
		CRect dialogRect = ui::GetControlRect( m_pDialog->m_hWnd );
		if ( m_collapsed )
			dialogRect.BottomRight() -= m_collapsedDelta;
		else
			dialogRect.BottomRight() += m_collapsedDelta;

		if ( !ui::IsChild( m_pDialog->m_hWnd ) )
			ui::EnsureVisibleDesktopRect( dialogRect, !HasFlag( m_pDialog->GetExStyle(), WS_EX_TOOLWINDOW ) ? ui::Workspace : ui::Monitor );

		m_pDialog->MoveWindow( &dialogRect, m_pDialog->IsWindowVisible() );
	}
	else
	{
		m_prevClientRect.SetRectEmpty();
		LayoutControls();
	}

	ui::RedrawDialog( m_pDialog->m_hWnd, 0 );

	if ( ui::ILayoutEngine* pDialogCallback = dynamic_cast<ui::ILayoutEngine*>( m_pDialog ) )
		pDialogCallback->OnCollapseChanged( m_collapsed );
	return true;
}

void CLayoutEngine::GetClientRectangle( OUT CRect* pClientRect ) const
{
	ASSERT_PTR( pClientRect );
	m_pDialog->GetClientRect( pClientRect );
}

bool CLayoutEngine::LayoutControls( void )
{
	ASSERT( IsInitialized() );

	CRect clientRect;
	GetClientRectangle( &clientRect );

	CSize minClientSize = GetMinClientSize();
	CSize clientSize( std::max<long>( clientRect.Width(), minClientSize.cx ), std::max<long>( clientRect.Height(), minClientSize.cy ) );

	ui::SetRectSize( clientRect, clientSize );

	return LayoutControls( clientRect );
}

bool CLayoutEngine::LayoutControls( const CRect& clientRect )
{
	ASSERT( ::IsWindow( m_pDialog->GetSafeHwnd() ) );
	ASSERT( ::GetWindow( m_pDialog->GetSafeHwnd(), GW_CHILD ) );		// controls created from template

	if ( !m_layoutEnabled || !HasCtrlLayout() )
		return false;

	if ( m_prevClientRect == clientRect )
		return false;								// skip when the same origin & size is already layed-out

	if ( GetResizeGripper() != nullptr )
		m_pGripper->Layout();

	layout::CDelta delta( clientRect.TopLeft() - m_dlgTemplClientRect.TopLeft(), clientRect.Size() - GetMinClientSize() );
	CScopedFlag<int> scopedErasing( &m_flags, InLayout );

	m_prevClientRect = clientRect;

	if ( HasFlag( m_flags, SmoothGroups ) )
		return LayoutSmoothly( delta );
	else
		return LayoutNormal( delta );
}

bool CLayoutEngine::LayoutSmoothly( const layout::CDelta& delta )
{
	bool visible = m_pDialog->IsWindowVisible() != FALSE;
	if ( visible )
		m_pDialog->SetRedraw( FALSE );

	int changedCount = 0;
	for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != nullptr; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
	{
		UINT ctrlId = ::GetDlgCtrlID( hCtrl );

		if ( const layout::CControlState* pCtrlState = LookupControlState( ctrlId ) )
			if ( pCtrlState->RepositionCtrl( delta, m_collapsed ) )
			{
				if ( ui::ILayoutFrame* pCtrlFrame = FindControlLayoutFrame( hCtrl ) )
					pCtrlFrame->OnControlResized();		// notify controls having dependent layout

				++changedCount;
			}
	}

	if ( visible )
		m_pDialog->SetRedraw( TRUE );

	if ( 0 == changedCount )
		return false;

	m_pDialog->RedrawWindow( nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN );
	return changedCount != 0;
}

bool CLayoutEngine::LayoutNormal( const layout::CDelta& delta )
{
	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	CRgn clientRegion;
	clientRegion.CreateRectRgnIndirect( &clientRect );

	std::vector<HWND> changedCtrls; changedCtrls.reserve( m_controlStates.size() );

	for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != nullptr; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
	{
		UINT ctrlId = ::GetDlgCtrlID( hCtrl );
		const layout::CControlState* pCtrlState = LookupControlState( ctrlId );

		if ( pCtrlState != nullptr )
			if ( pCtrlState->RepositionCtrl( delta, m_collapsed ) )
				changedCtrls.push_back( hCtrl );

		// clip control out of background erase region (except the ones to be repainted)
		if ( nullptr == pCtrlState || !HasFlag( pCtrlState->GetLayoutStyle( false ), layout::DoRepaint ) )
			if ( ui::IsVisible( hCtrl ) )
			{
				CRect ctrlRect;
				::GetWindowRect( hCtrl, &ctrlRect );				// refresh with the real rect
				m_pDialog->ScreenToClient( &ctrlRect );
				ui::CombineWithRegion( &clientRegion, ctrlRect, RGN_DIFF );
			}
	}

	m_pDialog->InvalidateRgn( &clientRegion, TRUE );				// invalidate background by clipping the children

	for ( std::vector<HWND>::const_iterator itCtrl = changedCtrls.begin(); itCtrl != changedCtrls.end(); ++itCtrl )
	{
		ui::RedrawControl( *itCtrl );

		if ( ui::ILayoutFrame* pCtrlFrame = FindControlLayoutFrame( *itCtrl ) )
			pCtrlFrame->OnControlResized();				// notify controls having dependent layout
	}

	return !changedCtrls.empty();
}

layout::TStyle CLayoutEngine::FindLayoutStyle( UINT ctrlId ) const
{
	std::unordered_map<UINT, layout::CControlState>::const_iterator itFound = m_controlStates.find( ctrlId );
	return itFound != m_controlStates.end() ? itFound->second.GetLayoutStyle( false ) : layout::None;
}

layout::CControlState* CLayoutEngine::LookupControlState( UINT ctrlId )
{
	return utl::FindValuePtr( m_controlStates, ctrlId );
}

bool CLayoutEngine::RefreshControlHandle( UINT ctrlId )
{
	if ( m_pDialog != nullptr )
		if ( layout::CControlState* pCtrlState = LookupControlState( ctrlId ) )
		{
			if ( HWND hControlNew = ::GetDlgItem( m_pDialog->m_hWnd, ctrlId ) )
				return utl::ModifyValue( pCtrlState->m_hControl, hControlNew );

			pCtrlState->ResetCtrl();
			return true;		// changed
		}

	return false;
}

void CLayoutEngine::AdjustControlInitialPosition( UINT ctrlId, const CSize& deltaOrigin, const CSize& deltaSize )
{
	// when stretching content to fit: to retain original layout behaviour
	if ( layout::CControlState* pCtrlState = LookupControlState( ctrlId ) )
		pCtrlState->AdjustInitialPosition( deltaOrigin, deltaSize );
	else
		ASSERT( false );			// no layout info for the control
}

void CLayoutEngine::RegisterBuddyCallback( UINT buddyId, ui::ILayoutFrame* pCallback )
{
	ASSERT_PTR( pCallback );
	ASSERT( m_buddyCallbacks.find( buddyId ) == m_buddyCallbacks.end() );

	m_buddyCallbacks[ buddyId ] = pCallback;
}

ui::ILayoutFrame* CLayoutEngine::FindControlLayoutFrame( HWND hCtrl ) const
{
	ASSERT_PTR( hCtrl );

	if ( ui::ILayoutFrame* pControlLayoutFrame = dynamic_cast<ui::ILayoutFrame*>( CWnd::FromHandlePermanent( hCtrl ) ) )
		return pControlLayoutFrame;

	std::unordered_map<UINT, ui::ILayoutFrame*>::const_iterator itFound = m_buddyCallbacks.find( ::GetDlgCtrlID( hCtrl ) );
	if ( itFound == m_buddyCallbacks.end() )
		return nullptr;

	return itFound->second;
}

void CLayoutEngine::HandleGetMinMaxInfo( MINMAXINFO* pMinMaxInfo ) const
{
	if ( !HasInitialSize() )
		return;

	pMinMaxInfo->ptMinTrackSize = GetMinWindowSize();

	CSize maxClientSize = GetMaxClientSize();

	if ( maxClientSize.cx != INT_MAX )
		pMinMaxInfo->ptMaxTrackSize.x = maxClientSize.cx + m_nonClientSize.cx;

	if ( maxClientSize.cy != INT_MAX )
		pMinMaxInfo->ptMaxTrackSize.y = maxClientSize.cy + m_nonClientSize.cy;
}

LRESULT CLayoutEngine::HandleHitTest( LRESULT hitTest, const CPoint& screenPoint ) const
{
	if ( HTCLIENT == hitTest )
		if ( GetResizeGripper() != nullptr )
		{
			CPoint point = screenPoint; m_pDialog->ScreenToClient( &point );
			if ( m_pGripper->GetRect().PtInRect( point ) )
				return HandleHitTest( HTBOTTOMRIGHT, screenPoint );
		}

	CSize minClientSize = GetMinClientSize(), maxClientSize = GetMaxClientSize();
	bool horiz = maxClientSize.cx > minClientSize.cx, vert = maxClientSize.cy > minClientSize.cy;

	if ( !( horiz && vert ) )
		switch ( hitTest )
		{
			case HTLEFT:
			case HTRIGHT:
				return horiz ? hitTest : HTBORDER;
			case HTTOP:
			case HTBOTTOM:
				return vert ? hitTest : HTBORDER;
			case HTTOPLEFT:
				return horiz ? HTLEFT : ( vert ? HTTOP : HTBORDER );
			case HTTOPRIGHT:
				return horiz ? HTRIGHT : ( vert ? HTTOP : HTBORDER );
			case HTBOTTOMLEFT:
				return horiz ? HTLEFT : ( vert ? HTBOTTOM : HTBORDER );
			case HTGROWBOX:
			case HTBOTTOMRIGHT:
				return horiz ? HTRIGHT : ( vert ? HTBOTTOM : HTBORDER );
		}

	return hitTest;
}

bool CLayoutEngine::HandleEraseBkgnd( CDC* pDC )
{
	if ( !HasCtrlLayout() )
		return false;			// ignore for dialogs with no custom layout

	if ( !HasFlag( m_flags, SmoothGroups ) || !IsInitialized() || HasFlag( m_flags, Erasing ) )
		return false;

	CScopedFlag<int> scopedErasing( &m_flags, Erasing );
	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	// use double buffering only for smaller client areas; it gets slower for large areas
	enum { DoubleBuffer_MaxArea = 300 * 200 };
	bool useDoubleBuffer = false;

	if ( HasFlag( m_flags, UseDoubleBuffer ) )
		if ( HasFlag( m_pDialog->GetStyle(), WS_CLIPCHILDREN ) &&
			 !m_hiddenGroups.empty() )									// don't bother just for the gripper
			useDoubleBuffer = clientRect.Width() * clientRect.Height() <= DoubleBuffer_MaxArea;

	CMemoryDC memDC( *pDC, clientRect, useDoubleBuffer );
	DrawBackground( &memDC.GetDC(), clientRect );
	return true;
}

void CLayoutEngine::HandlePostPaint( void )
{
	if ( !HasCtrlLayout() )
		return;			// ignore for dialogs with no custom layout

	if ( !HasFlag( m_flags, SmoothGroups ) && IsInitialized() )
		if ( layout::CResizeGripper* pGripper = GetResizeGripper() )
		{
			CClientDC dc( m_pDialog );
			m_pGripper->Draw( dc );
		}
}

void CLayoutEngine::DrawBackground( CDC* pDC, const CRect& clientRect )
{
	if ( HBRUSH hBkBrush = ui::SendCtlColor( *m_pDialog, *pDC, WM_CTLCOLORDLG ) )
	{
		CRgn clientRegion;
		clientRegion.CreateRectRgnIndirect( &clientRect );

		// clip other controls from the background erase region
		if ( !is_a<CFormView>( m_pDialog ) )					// prevent background erase issues with checkboxes, etc
			for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != nullptr; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
				if ( CanClip( hCtrl ) )
				{
					CRect ctrlRect;
					::GetWindowRect( hCtrl, &ctrlRect );
					ctrlRect.DeflateRect( 1, 1 );				// shring by one to fill corner pixels not covered by themed background (combos, edits)
					m_pDialog->ScreenToClient( &ctrlRect );
					ui::CombineWithRegion( &clientRegion, ctrlRect, RGN_XOR );
				}

		//CBrush debugBrush( color::PastelPink ); hBkBrush = debugBrush;
		::FillRgn( *pDC, clientRegion, hBkBrush );
	}

	if ( layout::CResizeGripper* pGripper = GetResizeGripper() )
		m_pGripper->Draw( *pDC );

	// draw hidden group boxes smoothly
	for ( std::vector<HWND>::iterator itGroup = m_hiddenGroups.begin(); itGroup != m_hiddenGroups.end(); ++itGroup )
	{
		CRect ctrlRect;
		::GetWindowRect( *itGroup, &ctrlRect );				// refresh with the real rect
		m_pDialog->ScreenToClient( &ctrlRect );

		POINT oldOrigin = pDC->SetWindowOrg( -ctrlRect.left, -ctrlRect.top );
		::SendMessage( *itGroup, WM_PRINT, (WPARAM)pDC->GetSafeHdc(), PRF_CLIENT );
		pDC->SetWindowOrg( oldOrigin );
	}
}

bool CLayoutEngine::CanClip( HWND hCtrl ) const
{
	if ( !ui::IsVisible( hCtrl ) || ui::IsTransparent( hCtrl ) || utl::Contains( m_hiddenGroups, hCtrl ) )
		return false;

	return !HasFlag( FindLayoutStyle( hCtrl ), layout::DoRepaint );
}


// CPaneLayoutEngine implementation

CPaneLayoutEngine::CPaneLayoutEngine( int flags /*= s_defaultFlags*/ )
	: CLayoutEngine( flags )
	, m_pLayoutFrame( nullptr )
	, m_pMasterLayout( nullptr )
{
	m_layoutType = PaneLayout;
}

CPaneLayoutEngine::~CPaneLayoutEngine()
{
}

void CPaneLayoutEngine::Reset( void ) override
{
	__super::Reset();

	m_pLayoutFrame = nullptr;
	m_pMasterLayout = nullptr;
}

void CPaneLayoutEngine::GetClientRectangle( OUT CRect* pClientRect ) const override
{
	ASSERT_PTR( m_pLayoutFrame );

	if ( CWnd* pCtrlFrame = m_pLayoutFrame->GetControl() )
		*pClientRect = ui::GetControlRect( pCtrlFrame->m_hWnd );
	else
		ASSERT( false );
}

void CPaneLayoutEngine::InitializePane( ui::ILayoutFrame* pPaneLayoutFrame )
{
	REQUIRE( !IsInitialized() );
	ASSERT_PTR( pPaneLayoutFrame );

	m_pLayoutFrame = pPaneLayoutFrame;
	__super::m_pDialog = pPaneLayoutFrame->GetDialog();

	if ( ui::ILayoutEngine* pMasterLayout = dynamic_cast<ui::ILayoutEngine*>( m_pDialog ) )
		m_pMasterLayout = &pMasterLayout->GetLayoutEngine();

	ENSURE( ::IsWindow( m_pDialog->GetSafeHwnd() ) );
	ENSURE( ui::IsDialogBox( m_pDialog->GetSafeHwnd() ) );

	if ( !HasInitialSize() )
		StoreInitialPaneSize();

	ENSURE( HasInitialSize() );						// should have a size by this time

	SetupControlStates();

	// initially hide this collapsed pane's controls:  (this is the earliest time to initialize pane controls' hidden state)
	if ( CResizeGripBar* pResizeGripBar = pPaneLayoutFrame->GetSplitterGripBar() )
		if ( pPaneLayoutFrame->GetControl() == pResizeGripBar->GetCollapsiblePane() )		// is this the collapsible pane?
			if ( pResizeGripBar->IsCollapsed() )
				ShowPaneControls( false );
}

bool CPaneLayoutEngine::ShowPaneControls( bool show /*= true*/ )
{
	if ( !IsInitialized()
		 || HasFlag( m_flags, InLayout )			// avoid changing control visibility while the dialog is in SetRedraw( FALSE ) mode!
		 || ( m_pMasterLayout != nullptr && HasFlag( m_pMasterLayout->GetFlags(), InLayout ) ) )
	{
		return false;				// called too early, defer for after initialization
	}

	UINT changeCount = 0;

	for ( std::unordered_map<UINT, layout::CControlState>::const_iterator itCtrl = m_controlStates.begin(); itCtrl != m_controlStates.end(); ++itCtrl )
		if ( itCtrl->second.IsInitialVisible() )
			if ( ui::ShowWindow( itCtrl->second.m_hControl, show ) )
				++changeCount;

	return changeCount != 0;
}

void CPaneLayoutEngine::StoreInitialPaneSize( void )
{	// for layout frame: compute initial size of the bounding rect based on dialog template initial positions of controls:
	ASSERT_PTR( m_pDialog->GetSafeHwnd() );
	ASSERT_PTR( m_pLayoutFrame );

	CRect ctrlBoundsRect( 0, 0, 0, 0 );		// control bounds rect in parent dialog coordinates

	for ( std::unordered_map<UINT, layout::CControlState>::const_iterator itCtrl = m_controlStates.begin(); itCtrl != m_controlStates.end(); ++itCtrl )
		if ( CWnd* pCtrl = m_pDialog->GetDlgItem( itCtrl->first ) )
		{
			CRect ctrlRect = ui::GetControlRect( pCtrl->m_hWnd );

			if ( ctrlBoundsRect.IsRectEmpty() )
				ctrlBoundsRect = ctrlRect;
			else
				ctrlBoundsRect |= ctrlRect;
		}
		else
			ASSERT( false );		// control ID not found?

	// store initial size of the layout frame
	m_dlgTemplClientRect = ctrlBoundsRect;

	if ( CWnd* pFrameCtrl = m_pLayoutFrame->GetControl() )
		m_nonClientSize = ui::GetNonClientSize( pFrameCtrl->m_hWnd );		// not really useful for child frames, but let's keep track of it

	CSize minClientSize = m_dlgTemplClientRect.Size();

	m_maxClientSize.cx = std::max( m_maxClientSize.cx, minClientSize.cx );
	m_maxClientSize.cy = std::max( m_maxClientSize.cy, minClientSize.cy );

	m_prevClientRect = ctrlBoundsRect;
}
