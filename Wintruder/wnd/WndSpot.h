#ifndef WndSpot_h
#define WndSpot_h
#pragma once


// wraps any window, it can be copied;
// works with NULL pattern;
// accounts for windows that span on multiple monitors, such as destop window.

class CWndSpot : public CWnd
{
public:
	CWndSpot( void );
	CWndSpot( HWND hWnd, const CPoint& screenPoint = m_nullPos );
	CWndSpot( const CWndSpot& right );
	~CWndSpot();

	CWndSpot& operator=( const CWndSpot& right );
	void SetWnd( HWND hWnd, const CPoint& screenPoint = m_nullPos );

	bool IsNull( void ) const { return NULL == m_hWnd; }
	bool IsValid( void ) const { return m_hWnd != NULL && ::IsWindow( m_hWnd ); }
	bool IsDesktopWnd( void ) const { return m_hWnd != NULL && ::GetDesktopWindow() == m_hWnd; }
	bool HasValidPoint( void ) const { return !( m_screenPoint == m_nullPos ); }

	bool Equals( const CWndSpot& right ) const;

	CRect GetWindowRect( void ) const;
	CPoint GetScreenPoint( void ) const;
	CRect FindMonitorRect( void ) const;			// monitor corresponding to m_screenPoint

	bool IsChildWindow( void ) const;
public:
	CPoint m_screenPoint;
	static const CPoint m_nullPos;
	static const CWndSpot m_nullWnd;
};


#endif // WndSpot_h
