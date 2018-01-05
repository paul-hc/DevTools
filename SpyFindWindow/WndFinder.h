#ifndef WndFinder_h
#define WndFinder_h
#pragma once

#include "WndSpot.h"


class CWndFinder
{
public:
	CWndFinder( void );

	CWndSpot WindowFromPoint( const CPoint& screenPos ) const;
	bool IsValidMatch( HWND hWnd ) const;

	static inline int GetRectArea( const CRect& rect, int minExtent = 1 ) { return std::max< int >( rect.Width(), minExtent ) * std::max< int >( rect.Height(), minExtent ); }
private:
	HWND FindBestFitSibling( HWND hWnd, const CRect& hitRect, const CPoint& screenPos ) const;
	HWND FindChildWindow( HWND hWndParent, const CRect& hitRect, const CPoint& screenPos ) const;
	bool ContainsPoint( HWND hWnd, const CPoint& screenPos ) const;
private:
	DWORD m_appProcessId;
};


#endif // WndFinder_h
