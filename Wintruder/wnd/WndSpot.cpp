
#include "stdafx.h"
#include "WndSpot.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#include "utl/Guards.h"
#include "WndUtils.h"

		DWORD CWndSpot::GetStyle( void ) const
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::GetStyle() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			return CWnd::GetStyle();
		}

		DWORD CWndSpot::GetExStyle( void ) const
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::GetExStyle() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			return CWnd::GetExStyle();
		}

		int CWndSpot::GetDlgCtrlID( void ) const
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::GetDlgCtrlID() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			return CWnd::GetDlgCtrlID();
		}

		int CWndSpot::SetDlgCtrlID( int ctrlId )
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::SetDlgCtrlID() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			return CWnd::SetDlgCtrlID( ctrlId );
		}

		BOOL CWndSpot::IsWindowVisible( void ) const
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::IsWindowVisible() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			return CWnd::IsWindowVisible();
		}

		void CWndSpot::GetClientRect( LPRECT lpRect ) const
		{
			utl::CSlowSectionGuard slowGuard( str::Format( _T("CWndSpot::GetClientRect() for window hWnd=%s"), wnd::FormatWindowHandle( m_hWnd ).c_str() ) );
			CWnd::GetClientRect( lpRect );
		}



const CPoint CWndSpot::m_nullPos( INT_MAX, INT_MAX );
const CWndSpot CWndSpot::m_nullWnd;

CWndSpot::CWndSpot( void )
	: m_screenPoint( m_nullPos )
{
}

CWndSpot::CWndSpot( HWND hWnd, const CPoint& screenPoint /*= m_nullPos*/ ) : m_screenPoint( screenPoint )
{
	m_hWnd = hWnd;
}

CWndSpot::CWndSpot( const CWndSpot& right ) : m_screenPoint( right.m_screenPoint )
{
	m_hWnd = right.m_hWnd;
}

CWndSpot::~CWndSpot()
{
	m_hWnd = NULL;
}

CWndSpot& CWndSpot::operator=( const CWndSpot& right )
{
	if ( &right != this )
	{
		m_hWnd = right.m_hWnd;
		m_screenPoint = right.m_screenPoint;
	}
	return *this;
}

void CWndSpot::SetWnd( HWND hWnd, const CPoint& screenPoint /*= m_nullPos*/ )
{
	m_hWnd = hWnd;
	m_screenPoint = screenPoint;
}

bool CWndSpot::Equals( const CWndSpot& right ) const
{
	if ( m_hWnd == right.m_hWnd )
		return
			!IsValid() ||										// both null
			FindMonitorRect() == right.FindMonitorRect();		// test point on the same monitor

	return false;
}

CRect CWndSpot::GetWindowRect( void ) const
{
	ASSERT( IsValid() );
	CRect windowRect;
	::GetWindowRect( m_hWnd, &windowRect );
	return windowRect;
}

CPoint CWndSpot::GetScreenPoint( void ) const
{
	ASSERT( IsValid() );
	return HasValidPoint() ? m_screenPoint : GetWindowRect().TopLeft();
}

CRect CWndSpot::FindMonitorRect( void ) const
{
	ASSERT( IsValid() );
	return HasValidPoint() ? ui::FindMonitorRectAt( m_screenPoint, ui::Monitor ) : ui::FindMonitorRectAt( GetWindowRect(), ui::Monitor );
}

bool CWndSpot::IsChildWindow( void ) const
{
	ASSERT( IsValid() );
	return HasFlag( ui::GetStyle( m_hWnd ), WS_CHILD );
}
