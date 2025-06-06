
#include "pch.h"
#include "WndSpot.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CPoint CWndSpot::m_nullPos( INT_MAX, INT_MAX );
const CWndSpot CWndSpot::m_nullWnd;

CWndSpot::~CWndSpot()
{
	m_hWnd = nullptr;
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
