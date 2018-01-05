#ifndef IdeUtilities_h
#define IdeUtilities_h
#pragma once


namespace ide
{
	bool isVC6( void );
	bool isVC71( void );

	CWnd* getRootWindow( void );
	CWnd* getFocusWindow( void );

	CPoint getCaretScreenPos( void );
	CPoint getMouseScreenPos( void );

	std::pair< HMENU, int > findPopupMenuWithCommand( HWND hWnd, UINT commandID );

	CWnd* getRootParentWindow( CWnd* pWindow );
	std::tstring getWindowInfo( HWND hWnd );
	CString	getWindowTitle( HWND hWnd );
	CString	getWindowClassName( HWND hWnd );

	int setFocusWindow( HWND hWnd );
	CWnd* createThreadTrackingWindow( CWnd* pParent );

	UINT trackPopupMenu( CMenu& rMenu, const CPoint& screenPos, CWnd* pWindow,
						 UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON );
}


#endif // IdeUtilities_h
