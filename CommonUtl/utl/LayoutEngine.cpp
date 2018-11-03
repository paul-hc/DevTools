
#include "stdafx.h"
#include "LayoutEngine.h"
#include "MemoryDC.h"
#include "ScopedValue.h"
#include "ContainerUtilities.h"
#include "Utilities.h"
#include <afxext.h>				// for CFormView

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


int CLayoutEngine::m_defaultFlags = Smooth;


CLayoutEngine::CLayoutEngine( int flags /*= m_defaultFlags*/ )
	: m_flags( flags )
	, m_layoutEnabled( true )
	, m_pDialog( NULL )
	, m_minClientSize( 0, 0 )
	, m_maxClientSize( INT_MAX, INT_MAX )
	, m_nonClientSize( 0, 0 )
	, m_previousSize( 0, 0 )
	, m_collapsedDelta( 0, 0 )
	, m_collapsed( false )
{
}

CLayoutEngine::~CLayoutEngine()
{
}

void CLayoutEngine::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	for ( unsigned int i = 0; i != count; ++i )
	{
		ASSERT( layoutStyles[ i ].m_ctrlId != 0 );
		m_controlStates[ layoutStyles[ i ].m_ctrlId ].SetLayoutStyle( layoutStyles[ i ].m_layoutStyle, false );
	}
}

void CLayoutEngine::RegisterDualCtrlLayout( const CDualLayoutStyle dualLayoutStyles[], unsigned int count )
{
	for ( unsigned int i = 0; i != count; ++i )
	{
		ASSERT( dualLayoutStyles[ i ].m_ctrlId != 0 );
		layout::CControlState& rCtrlState = m_controlStates[ dualLayoutStyles[ i ].m_ctrlId ];

		rCtrlState.SetLayoutStyle( dualLayoutStyles[ i ].m_expandedStyle, false );
		rCtrlState.SetLayoutStyle( dualLayoutStyles[ i ].m_collapsedStyle != layout::UseExpanded
			? dualLayoutStyles[ i ].m_collapsedStyle : dualLayoutStyles[ i ].m_expandedStyle, true );
	}
}

void CLayoutEngine::StoreInitialSize( CWnd* pDialog )
{
	CRect clientRect, windowRect;
	pDialog->GetClientRect( &clientRect );
	pDialog->GetWindowRect( &windowRect );

	// store initial size of the dialog
	m_minClientSize = m_previousSize = clientRect.Size();
	m_nonClientSize = windowRect.Size() - m_minClientSize;

	m_maxClientSize.cx = std::max< long >( m_maxClientSize.cx, m_minClientSize.cx );
	m_maxClientSize.cy = std::max< long >( m_maxClientSize.cy, m_minClientSize.cy );
}

void CLayoutEngine::Initialize( CWnd* pDialog )
{
	ASSERT( !IsInitialized() );

	m_pDialog = pDialog;
	ASSERT( ::IsWindow( m_pDialog->GetSafeHwnd() ) );

	if ( !HasInitialSize() )
		StoreInitialSize( m_pDialog );

	ASSERT( HasInitialSize() );		// should have a size by this time

	SetupControlStates();
}

void CLayoutEngine::Reset( void )
{
	m_pDialog = NULL;
	m_minClientSize.cx = m_minClientSize.cy = 0;
	m_maxClientSize.cx = m_maxClientSize.cy = INT_MAX;
	m_nonClientSize.cx = m_nonClientSize.cy = 0;
	m_previousSize.cx = m_previousSize.cy = 0;

	for ( stdext::hash_map< UINT, layout::CControlState >::iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
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
	return IsInitialized() && !m_pDialog->IsZoomed() ? m_pGripper.get() : NULL;
}

void CLayoutEngine::SetupControlStates( void )
{
	ASSERT( IsInitialized() );
	ASSERT( m_controlStates.empty() || !m_controlStates.begin()->second.IsCtrlInit() );		// initialize once

	bool anyRepaintCtrl = false;
	for ( stdext::hash_map< UINT, layout::CControlState >::const_iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
	{
		if ( HasFlag( itCtrlState->second.GetLayoutStyle( false ), layout::DoRepaint ) )
			anyRepaintCtrl = true;

		if ( HasFlag( itCtrlState->second.GetLayoutStyle( false ), layout::CollapsedMask ) )
			SetupCollapsedState( itCtrlState->first, itCtrlState->second.GetLayoutStyle( false ) );
	}

	if ( anyRepaintCtrl )
	{
		ModifyFlag( m_flags, SmoothGroups, GroupsTransparent | GroupsRepaint );		// disable SmoothGroups and WS_CLIPCHILDREN when one control requires repaint (layout::DoRepaint)
		m_pDialog->ModifyStyle( WS_CLIPCHILDREN, 0 );
	}

	if ( HasFlag( m_flags, SmoothGroups ) )
		m_pDialog->ModifyStyle( 0, WS_CLIPCHILDREN );								// force WS_CLIPCHILDREN on dialog

	HWND hCtrl = ::GetWindow( m_pDialog->GetSafeHwnd(), GW_CHILD );
	ASSERT_PTR( hCtrl );		// controls created from dialog template?

	for ( ; hCtrl != NULL; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
		if ( UINT ctrlId = ::GetDlgCtrlID( hCtrl ) )								// ignore child dialogs in a property sheet
		{
			stdext::hash_map< UINT, layout::CControlState >::iterator itCtrlState = m_controlStates.find( ctrlId );
			layout::CControlState* pControlState = itCtrlState != m_controlStates.end() ? &itCtrlState->second : NULL;

			if ( ui::IsGroupBox( hCtrl ) )
				SetupGroupBoxState( hCtrl, pControlState );

			if ( pControlState )
				pControlState->InitCtrl( hCtrl );
		}
}

void CLayoutEngine::SetupCollapsedState( UINT ctrlId, layout::Style style )
{
	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	CRect anchorRect = ui::GetControlRect( ::GetDlgItem( m_pDialog->m_hWnd, ctrlId ) );

	ASSERT( 0 == m_collapsedDelta.cx && 0 == m_collapsedDelta.cy );					// initialize once
	if ( HasFlag( style, layout::CollapsedLeft ) )
		m_collapsedDelta.cx = clientRect.right - anchorRect.left;

	if ( HasFlag( style, layout::CollapsedTop ) )
		m_collapsedDelta.cy = clientRect.bottom - anchorRect.top;
}

void CLayoutEngine::SetupGroupBoxState( HWND hGroupBox, layout::CControlState* pControlState )
{
	if ( HasFlag( m_flags, SmoothGroups ) )
	{
		// hide group boxes and clear transparent ex-style; will be rendered on WM_ERASEBKGND
		CWnd::ModifyStyle( hGroupBox, WS_VISIBLE, 0, 0 );
		CWnd::ModifyStyleEx( hGroupBox, WS_EX_TRANSPARENT, 0, 0 );
		m_hiddenGroups.push_back( hGroupBox );
	}
	else
	{
		if ( HasFlag( m_flags, GroupsRepaint ) && pControlState != NULL )
			pControlState->ModifyLayoutStyle( 0, layout::DoRepaint );		// force groups repaint

		if ( HasFlag( m_flags, GroupsTransparent ) )
		{
			// make group boxes transparent (z-order fix for proxy controls);
			// prevents the WS_CLIPCHILDREN styled parent window from excluding the groupbox's background region, allowing the background to paint;
			// while this works, it creates annoying flicker on resize.
			if ( !HasFlag( ui::GetStyleEx( hGroupBox ), WS_EX_TRANSPARENT ) )
				CWnd::ModifyStyleEx( hGroupBox, 0, WS_EX_TRANSPARENT, 0 );
		}
	}
}

bool CLayoutEngine::AnyRepaintCtrl( void ) const
{
	for ( stdext::hash_map< UINT, layout::CControlState >::const_iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
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
		for ( stdext::hash_map< UINT, layout::CControlState >::iterator itCtrlState = m_controlStates.begin(); itCtrlState != m_controlStates.end(); ++itCtrlState )
			itCtrlState->second.ResetCtrl();
	}
}

CSize CLayoutEngine::GetMinClientSize( bool collapsed ) const
{
	CSize minClientSize = m_minClientSize;
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
		m_previousSize.cx = m_previousSize.cy = 0;
		LayoutControls();
	}

	ui::RedrawDialog( m_pDialog->m_hWnd, 0 );

	if ( ui::ILayoutEngine* pDialogCallback = dynamic_cast< ui::ILayoutEngine* >( m_pDialog ) )
		pDialogCallback->OnCollapseChanged( m_collapsed );
	return true;
}

bool CLayoutEngine::LayoutControls( void )
{
	ASSERT( IsInitialized() );

	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	CSize minClientSize = GetMinClientSize();
	CSize clientSize( std::max< int >( clientRect.Width(), minClientSize.cx ), std::max< int >( clientRect.Height(), minClientSize.cy ) );
	return LayoutControls( clientSize );
}

bool CLayoutEngine::LayoutControls( const CSize& clientSize )
{
	ASSERT( ::IsWindow( m_pDialog->GetSafeHwnd() ) );
	ASSERT( ::GetWindow( m_pDialog->GetSafeHwnd(), GW_CHILD ) );		// controls created from template

	if ( !m_layoutEnabled || !HasCtrlLayout() )
		return false;

	if ( m_previousSize == clientSize )
		return false;							// skip when same size and already layed-out

	if ( GetResizeGripper() != NULL )
		m_pGripper->Layout();

	CSize delta = clientSize - GetMinClientSize();
	m_previousSize = clientSize;

	return HasFlag( m_flags, SmoothGroups )
		? LayoutSmoothly( delta )
		: LayoutNormal( delta );
}

bool CLayoutEngine::LayoutSmoothly( const CSize& delta )
{
	bool visible = m_pDialog->IsWindowVisible() != FALSE;
	if ( visible )
		m_pDialog->SetRedraw( FALSE );

	int changedCount = 0;
	for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != NULL; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
	{
		UINT ctrlId = ::GetDlgCtrlID( hCtrl );
		stdext::hash_map< UINT, layout::CControlState >::const_iterator itCtrlState = m_controlStates.find( ctrlId );

		if ( const layout::CControlState* pCtrlState = itCtrlState != m_controlStates.end() ? &itCtrlState->second : NULL )
			if ( pCtrlState->RepositionCtrl( delta, m_collapsed ) )
			{
				if ( ui::ILayoutFrame* pControlFrame = FindControlLayoutFrame( hCtrl ) )
					pControlFrame->OnControlResized( ctrlId );		// notify controls having dependent layout
				++changedCount;
			}
	}

	if ( visible )
		m_pDialog->SetRedraw( TRUE );

	if ( 0 == changedCount )
		return false;
	m_pDialog->RedrawWindow( NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN );
	return changedCount != 0;
}

bool CLayoutEngine::LayoutNormal( const CSize& delta )
{
	CRect clientRect;
	m_pDialog->GetClientRect( &clientRect );

	CRgn clientRegion;
	clientRegion.CreateRectRgnIndirect( &clientRect );

	std::vector< HWND > changedCtrls; changedCtrls.reserve( m_controlStates.size() );

	for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != NULL; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
	{
		UINT ctrlId = ::GetDlgCtrlID( hCtrl );
		stdext::hash_map< UINT, layout::CControlState >::const_iterator itCtrlState = m_controlStates.find( ctrlId );
		const layout::CControlState* pCtrlState = itCtrlState != m_controlStates.end() ? &itCtrlState->second : NULL;
		if ( pCtrlState != NULL )
			if ( pCtrlState->RepositionCtrl( delta, m_collapsed ) )
				changedCtrls.push_back( hCtrl );

		// clip control out of background erase region (except the ones to be repainted)
		if ( NULL == pCtrlState || !HasFlag( pCtrlState->GetLayoutStyle( false ), layout::DoRepaint ) )
			if ( ui::IsVisible( hCtrl ) )
			{
				CRect ctrlRect;
				::GetWindowRect( hCtrl, &ctrlRect );				// refresh with the real rect
				m_pDialog->ScreenToClient( &ctrlRect );
				ui::CombineWithRegion( &clientRegion, ctrlRect, RGN_DIFF );
			}
	}

	m_pDialog->InvalidateRgn( &clientRegion, TRUE );				// invalidate background by clipping the children

	for ( std::vector< HWND >::const_iterator itCtrl = changedCtrls.begin(); itCtrl != changedCtrls.end(); ++itCtrl )
	{
		ui::RedrawControl( *itCtrl );

		if ( ui::ILayoutFrame* pControlFrame = FindControlLayoutFrame( *itCtrl ) )
			pControlFrame->OnControlResized( ::GetDlgCtrlID( *itCtrl ) );		// notify controls having dependent layout
	}

	return !changedCtrls.empty();
}

layout::Style CLayoutEngine::FindLayoutStyle( UINT ctrlId ) const
{
	stdext::hash_map< UINT, layout::CControlState >::const_iterator itFound = m_controlStates.find( ctrlId );
	return itFound != m_controlStates.end() ? itFound->second.GetLayoutStyle( false ) : layout::None;
}

layout::CControlState* CLayoutEngine::LookupControlState( UINT ctrlId )
{
	return utl::FindValuePtr( m_controlStates, ctrlId );
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

	if ( ui::ILayoutFrame* pControlLayoutFrame = dynamic_cast< ui::ILayoutFrame* >( CWnd::FromHandlePermanent( hCtrl ) ) )
		return pControlLayoutFrame;

	stdext::hash_map< UINT, ui::ILayoutFrame* >::const_iterator itFound = m_buddyCallbacks.find( ::GetDlgCtrlID( hCtrl ) );
	if ( itFound != m_buddyCallbacks.end() )
		return itFound->second;

	return NULL;
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
		if ( GetResizeGripper() != NULL )
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
	if ( !HasFlag( m_flags, SmoothGroups ) || !IsInitialized() || HasFlag( m_flags, Erasing ) )
		return false;

	CScopedFlag< int > scopedErasing( &m_flags, Erasing );
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
		if ( !is_a< CFormView >( m_pDialog ) )					// prevent background erase issues with checkboxes, etc
			for ( HWND hCtrl = ::GetWindow( m_pDialog->m_hWnd, GW_CHILD ); hCtrl != NULL; hCtrl = ::GetWindow( hCtrl, GW_HWNDNEXT ) )
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
	for ( std::vector< HWND >::iterator itGroup = m_hiddenGroups.begin(); itGroup != m_hiddenGroups.end(); ++itGroup )
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
