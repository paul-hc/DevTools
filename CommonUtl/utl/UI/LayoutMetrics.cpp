
#include "pch.h"
#include "LayoutMetrics.h"
#include "CtrlInterfaces.h"
#include "ThemeItem.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	bool ScreenToClient( HWND hWnd, RECT& rRect )
	{
		if ( !ui::ScreenToClient( hWnd, rRect ) )
			return false;

		if ( HasFlag( ui::GetStyleEx( hWnd ), WS_EX_LAYOUTRTL ) )
			CRect::SwapLeftRight( &rRect );
		return true;
	}


	// Metrics implementation

	Metrics::Metrics( TStyle layoutStyle /*= 0*/ )
		: m_layoutStyle( layoutStyle )
	{
	}

	Metrics::~Metrics( void )
	{
	}

	bool Metrics::IsValid( void ) const
	{
		return
			m_layoutStyle != UINT_MAX &&
			m_fields.m_moveX >= 0 && m_fields.m_moveX <= 100 &&
			m_fields.m_moveY >= 0 && m_fields.m_moveY <= 100 &&
			m_fields.m_sizeX >= 0 && m_fields.m_sizeX <= 100 &&
			m_fields.m_sizeY >= 0 && m_fields.m_sizeY <= 100;
	}

	bool Metrics::HasEffect( void ) const
	{
		ASSERT( IsValid() );
		return m_fields.m_moveX != 0 || m_fields.m_moveY != 0 || m_fields.m_sizeX != 0 || m_fields.m_sizeY != 0;
	}


	// CControlState implementation

	CControlState::CControlState( const Metrics& metrics /*= Metrics( 0 )*/ )
		: m_metrics( metrics )
		, m_collapsedMetrics( UINT_MAX )
		, m_hControl( nullptr )
		, m_hParent( nullptr )
	{
	}

	CControlState::~CControlState()
	{
	}

	void CControlState::SetLayoutStyle( int layoutStyle, bool collapsed )
	{
		if ( !collapsed )
			m_metrics.m_layoutStyle = layoutStyle;
		else
			m_collapsedMetrics.m_layoutStyle = layoutStyle;
	}

	void CControlState::ModifyLayoutStyle( int clearStyle, int setStyle )
	{
		ModifyFlags( m_metrics.m_layoutStyle, clearStyle, setStyle );

		if ( HasCollapsedState() )
			ModifyFlags( m_collapsedMetrics.m_layoutStyle, clearStyle, setStyle );
	}

	void CControlState::InitCtrl( HWND hControl )
	{
		if ( hControl != nullptr )
		{
			m_hControl = hControl;
			m_hParent = ::GetParent( m_hControl );
			ASSERT( ::IsWindow( m_hControl ) && ::IsWindow( m_hParent ) );

			CRect initialControlRect;
			::GetWindowRect( m_hControl, &initialControlRect );
			layout::ScreenToClient( m_hParent, initialControlRect );

			m_initialOrigin = initialControlRect.TopLeft();
			m_initialSize = initialControlRect.Size();
		}
		else
		{
			m_hControl = m_hParent = nullptr;
			m_initialOrigin = CPoint( 0, 0 );
			m_initialSize = CSize( 0, 0 );
		}
	}

	bool CControlState::ComputeLayout( CRect& rCtrlRect, UINT& rSwpFlags, const CSize& delta, bool collapsed ) const
	{
		const Metrics& metrics = GetMetrics( collapsed );

		ASSERT( metrics.IsValid() );
		ASSERT( ::IsWindow( m_hControl ) );		// controls initialized? was __super::DoDataExchange() called after subclassing all controls?

		if ( !metrics.HasEffect() )
			return false;

		CRect currentControlRect;

		::GetWindowRect( m_hControl, &currentControlRect );
		layout::ScreenToClient( m_hParent, currentControlRect );

		// proportional layout for control's origin
		CPoint origin = m_initialOrigin;

		origin.x += MulDiv( delta.cx, metrics.m_fields.m_moveX, 100 );
		origin.y += MulDiv( delta.cy, metrics.m_fields.m_moveY, 100 );

		// proportional layout for control's size
		CSize size = m_initialSize;

		size.cx += MulDiv( delta.cx, metrics.m_fields.m_sizeX, 100 );
		size.cy += MulDiv( delta.cy, metrics.m_fields.m_sizeY, 100 );

		// set output parameters
		rCtrlRect = CRect( origin, size );

		if ( rCtrlRect == currentControlRect )
			return false;			// layout hasn't changed

		if ( rCtrlRect.TopLeft() == currentControlRect.TopLeft() )
			rSwpFlags |= SWP_NOMOVE;

		if ( rCtrlRect.Size() == currentControlRect.Size() )
			rSwpFlags |= SWP_NOSIZE;

		return true;	// true if origin or size has changed
	}

	bool CControlState::RepositionCtrl( const CSize& delta, bool collapsed ) const
	{
		CRect ctrlRect;
		UINT swpFlags = SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOZORDER;

		if ( !ComputeLayout( ctrlRect, swpFlags, delta, collapsed ) )
			return false;		// hasn't moved or resized

		::SetWindowPos( m_hControl, nullptr, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), swpFlags );		// reposition changed control
		return true;
	}

	void CControlState::AdjustInitialPosition( const CSize& deltaOrigin, const CSize& deltaSize )
	{
		m_initialOrigin += deltaOrigin;
		m_initialSize += deltaSize;
	}


	// CResizeGripper implementation

	CResizeGripper::CResizeGripper( CWnd* pDialog, const CSize& offset /*= CSize( 1, 0 )*/ )
		: m_pDialog( pDialog )
		, m_offset( offset )
	{
		ASSERT_PTR( m_pDialog->GetSafeHwnd() );
		Layout();
	}

	void CResizeGripper::Layout( void )
	{
		m_pDialog->GetClientRect( &m_gripperRect );
		m_gripperRect.TopLeft() = m_gripperRect.BottomRight() - CSize( GetSystemMetrics( SM_CXHSCROLL ), GetSystemMetrics( SM_CYHSCROLL ) );
		m_gripperRect.OffsetRect( m_offset );
	}

	void CResizeGripper::Redraw( void )
	{
		if ( !m_gripperRect.IsRectEmpty() )
			m_pDialog->InvalidateRect( &m_gripperRect, FALSE );
	}

	void CResizeGripper::Draw( HDC hDC )
	{
		static CThemeItem s_resizeBoxItem( L"SCROLLBAR", SBP_SIZEBOX, SZB_RIGHTALIGN );

		s_resizeBoxItem.DrawBackground( hDC, m_gripperRect );
	}

} // namespace layout


namespace layout
{
	void SetControlPos( HWND hCtrl, const CRect& ctrlRect, UINT swpFlags )
	{
		ASSERT_PTR( hCtrl );
		::SetWindowPos( hCtrl, nullptr, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), swpFlags );		// reposition changed control

		if ( ui::ILayoutFrame* pCtrlFrame = dynamic_cast<ui::ILayoutFrame*>( CWnd::FromHandlePermanent( hCtrl ) ) )		// control is a frame?
			pCtrlFrame->OnControlResized( ::GetDlgCtrlID( hCtrl ) );		// notify controls having dependent layout
	}

	void MoveControl( HWND hCtrl, const CRect& ctrlRect, bool repaint /*= true*/ )
	{
		ASSERT_PTR( hCtrl );
		::MoveWindow( hCtrl, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), repaint );

		if ( ui::ILayoutFrame* pCtrlFrame = dynamic_cast<ui::ILayoutFrame*>( CWnd::FromHandlePermanent( hCtrl ) ) )		// control is a frame?
			pCtrlFrame->OnControlResized( ::GetDlgCtrlID( hCtrl ) );		// notify controls having dependent layout
	}
}
