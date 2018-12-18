#ifndef IdeUtilities_h
#define IdeUtilities_h
#pragma once

#include "utl/Path.h"


namespace ide
{
	enum IdeType { VC_60, VC_71to90, VC_110plus };

	IdeType FindIdeType( void );

	CWnd* GetRootWindow( void );
	CWnd* GetFocusWindow( void );

	CPoint GetMouseScreenPos( void );

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


	namespace vs6
	{
		fs::CPath GetCommonDirPath( bool trailSlash = true );
		fs::CPath GetMacrosDirPath( bool trailSlash = true );
		fs::CPath GetVC98DirPath( bool trailSlash = true );
	}
}


#endif // IdeUtilities_h
