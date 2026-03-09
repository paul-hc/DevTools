#ifndef WndSpot_h
#define WndSpot_h
#pragma once


// wraps any window, it can be copied;
// accounts for windows that span on multiple monitors, such as destop window.

class CWndSpot : public CWnd
{
public:
	CWndSpot( void ) : m_screenPoint( m_nullPos ) {}
	CWndSpot( HWND hWnd, const CPoint& screenPoint = m_nullPos ) : m_screenPoint( screenPoint ) { m_hWnd = hWnd; }
	CWndSpot( const CWndSpot& right ) : m_screenPoint( right.m_screenPoint ) { m_hWnd = right.m_hWnd; }
	~CWndSpot() { m_hWnd = nullptr; }		// release the window handle

	CWndSpot& operator=( const CWndSpot& right ) { m_hWnd = right.m_hWnd; m_screenPoint = right.m_screenPoint; return *this; }
	void SetWnd( HWND hWnd, const CPoint& screenPoint = m_nullPos ) { m_hWnd = hWnd; m_screenPoint = screenPoint; }

	bool IsValid( void ) const { return m_hWnd != nullptr && ::IsWindow( m_hWnd ); }
	bool HasValidPoint( void ) const { return !( m_screenPoint == m_nullPos ); }

	bool Equals( const CWndSpot& right ) const;

	CRect GetWindowRect( void ) const;
	CPoint GetScreenPoint( void ) const;
	CRect FindMonitorRect( void ) const;			// monitor corresponding to m_screenPoint

	bool IsChildWindow( void ) const;
public:
	CPoint m_screenPoint;
	static const CPoint m_nullPos;
};


#endif // WndSpot_h
