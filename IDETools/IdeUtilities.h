#ifndef IdeUtilities_h
#define IdeUtilities_h
#pragma once


namespace ide
{
	enum IdeType { VC_60, VC_71to90, VC_110plus };

	IdeType FindIdeType( void );

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


	std::tstring GetRegistryPath_VC6( const TCHAR entry[] );
	std::tstring GetRegistryPath_VC71( const TCHAR entry[] );
	const std::tstring& GetVC71InstallDir( void );
}


#endif // IdeUtilities_h
