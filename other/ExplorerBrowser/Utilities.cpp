
#include "stdafx.h"
#include "Utilities.h"
#include <comdef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath )
	{
		DWORD attr = ::GetFileAttributes( pFilePath );
		return attr != INVALID_FILE_ATTRIBUTES && !HasFlag( attr, FILE_ATTRIBUTE_DIRECTORY );
	}

	bool IsValidDirectory( const TCHAR* pDirPath )
	{
		DWORD attr = ::GetFileAttributes( pDirPath );
		return attr != INVALID_FILE_ATTRIBUTES && HasFlag( attr, FILE_ATTRIBUTE_DIRECTORY );
	}
}


namespace utl
{
	HRESULT Audit( HRESULT hResult, const char* pFuncName )
	{
		pFuncName;
		if ( !SUCCEEDED( hResult ) )
			TRACE( " * %s: hResult=0x%08x: '%s' in function %s\n", FAILED( hResult ) ? "FAILED" : "ERROR", hResult, CStringA( _com_error( hResult ).ErrorMessage() ).GetString(), pFuncName );
		return hResult;
	}
}


namespace ui
{
	bool EnsureVisibleRect( CRect& rDest, const CRect& anchor, bool horiz /*= true*/, bool vert /*= true*/ )
	{
		if ( rDest.left >= anchor.left && rDest.top >= anchor.top &&
			 rDest.right <= anchor.right && rDest.bottom <= anchor.bottom )
			return false;		// no overflow, no change

		CPoint offset( 0, 0 );

		if ( horiz )
			if ( rDest.Width() > anchor.Width() )
				offset.x = anchor.left - rDest.left;
			else
				if ( rDest.left < anchor.left )
					offset.x = anchor.left - rDest.left;
				else if ( rDest.right > anchor.right )
					offset.x = anchor.right - rDest.right;

		if ( vert )
			if ( rDest.Height() > anchor.Height() )
				offset.y = anchor.top - rDest.top;
			else
				if ( rDest.top < anchor.top )
					offset.y = anchor.top - rDest.top;
				else if ( rDest.bottom > anchor.bottom )
					offset.y = anchor.bottom - rDest.bottom;

		if ( 0 == offset.x && 0 == offset.y )
			return false;		// no change

		rDest += offset;
		return true;
	}


	CRect FindMonitorRect( HWND hWnd, MonitorArea area )
	{
		if ( HMONITOR hMonitor = ::MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi; mi.cbSize = sizeof( MONITORINFO );
			if ( ::GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );			// fallback to the main screen desktop rect
		return desktopRect;
	}

	CRect FindMonitorRectAt( const POINT& screenPoint, MonitorArea area )
	{
		if ( HMONITOR hMonitor = ::MonitorFromPoint( screenPoint, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi; mi.cbSize = sizeof( MONITORINFO );
			if ( ::GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );				// fallback to the main screen desktop rect
		return desktopRect;
	}

	CRect FindMonitorRectAt( const RECT& screenRect, MonitorArea area )
	{
		if ( HMONITOR hMonitor = MonitorFromRect( &screenRect, MONITOR_DEFAULTTONEAREST ) )
		{
			MONITORINFO mi = { sizeof( MONITORINFO ) };
			if ( GetMonitorInfo( hMonitor, &mi ) )
				return Workspace == area ? mi.rcWork : mi.rcMonitor;		// work area excludes the taskbar
		}

		CRect desktopRect;
		GetWindowRect( ::GetDesktopWindow(), &desktopRect );				// fallback to the main screen desktop rect
		return desktopRect;
	}


	void SetRadio( CCmdUI* pCmdUI, BOOL checked )
	{
		ASSERT( pCmdUI != NULL );
		if ( NULL == pCmdUI->m_pMenu )
		{
			pCmdUI->SetRadio( checked );		// normal processing for toolbar buttons, etc
			return;
		}

		// CCmdUI::SetRadio() uses an ugly radio checkmark;
		// we put the standard nice radio checkmark using CheckMenuRadioItem()
		if ( !checked )
			pCmdUI->SetCheck( checked );
		else
		{
			if ( pCmdUI->m_pSubMenu != NULL )
				return;							// don't change popup submenus indirectly

			UINT pos = pCmdUI->m_nIndex;
			pCmdUI->m_pMenu->CheckMenuRadioItem( pos, pos, pos, MF_BYPOSITION );		// place radio checkmark
		}
	}
}
