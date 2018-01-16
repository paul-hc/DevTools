
#include "stdafx.h"
#include "Process.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace proc
{
	bool IsProcessElevated( HANDLE hProcess /*= ::GetCurrentProcess()*/ )
	{
		ASSERT_PTR( hProcess );
		CHandle token;
		if ( ::OpenProcessToken( hProcess, TOKEN_QUERY, token.GetPtr() ) )
		{
			TOKEN_ELEVATION elevation;
			DWORD cbSize = sizeof( TOKEN_ELEVATION );
			if ( ::GetTokenInformation( token.Get(), TokenElevation, &elevation, sizeof( elevation ), &cbSize ) )
				return elevation.TokenIsElevated != 0;
		}
		return false;
	}

	bool IsProcessElevated( DWORD processId, DWORD access /*= SYNCHRONIZE*/ )
	{
		CHandle process( ::OpenProcess( access, FALSE, processId ) );
		return IsProcessElevated( process.Get() );
	}

	bool HasSameElevation( HANDLE hProcess1, HANDLE hProcess2 )
	{
		return hProcess1 != NULL && hProcess2 != NULL && IsProcessElevated( hProcess1 ) == IsProcessElevated( hProcess2 );
	}

	bool HasCurrentElevation( HWND hWnd, DWORD access /*= SYNCHRONIZE*/ )
	{
		ASSERT( ui::IsValidWindow( hWnd ) );

		CHandle wndProcess( ::OpenProcess( access, FALSE, ui::GetWindowProcessId( hWnd ) ) );
		return HasCurrentElevation( wndProcess.Get() );
	}
}
