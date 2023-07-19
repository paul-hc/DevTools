#ifndef WndFinder_h
#define WndFinder_h
#pragma once

#include "WndSpot.h"


namespace opt { enum UpdateTarget; }
struct CWndSearchPattern;


class CWndFinder
{
public:
	CWndFinder( void );

	// position searching
	CWndSpot WindowFromPoint( const CPoint& screenPos ) const;
	bool IsValidMatch( HWND hWnd ) const;
	bool IsValidMatchIgnore( HWND hWnd ) const;		// uses ignore options

	static inline int GetRectArea( const CRect& rect, int minExtent = 1 ) { return std::max<int>( rect.Width(), minExtent ) * std::max<int>( rect.Height(), minExtent ); }

	// pattern searching
	HWND FindWindow( const CWndSearchPattern& pattern, HWND hStartWnd = nullptr );

	// auto-update target
	CWndSpot FindUpdateTarget( void ) const;
private:
	HWND FindBestFitSibling( HWND hWnd, const CRect& hitRect, const CPoint& screenPos ) const;
	HWND FindChildWindow( HWND hWndParent, const CRect& hitRect, const CPoint& screenPos ) const;
	bool ContainsPoint( HWND hWnd, const CPoint& screenPos ) const;

	HWND FindActiveWnd( void ) const;
	HWND FindFocusedWnd( void ) const;
	HWND FindCapturedWnd( void ) const;
	HWND FindTopmostWnd( opt::UpdateTarget topmostTarget ) const;
private:
	DWORD m_appProcessId, m_thisThreadId;
};


#endif // WndFinder_h
