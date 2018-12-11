
#include "stdafx.h"
#include "ProcessUtils.h"
#include "FileSystem_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	intptr_t CProcessCmd::ExecuteProcess( int mode )
	{
		::_flushall();				// flush i/o streams so the console is synched for child process output (if any)

		std::vector< const TCHAR* > argList;
		return ::_tspawnv( mode, m_exePath.c_str(), MakeArgList( argList ) );
	}

	const TCHAR* const* CProcessCmd::MakeArgList( std::vector< const TCHAR* >& rArgList ) const
	{
		rArgList.push_back( m_exePath.c_str() );	// first arg is always the executable itself

		for ( std::vector< std::tstring >::const_iterator itParam = m_params.begin(); itParam != m_params.end(); ++itParam )
			if ( !itParam->empty() )
				rArgList.push_back( itParam->c_str() );

		rArgList.push_back( NULL );					// terminating NULL
		return &rArgList.front();
	}
}


#ifndef _CONSOLE

#include "Utilities.h"

namespace proc
{
	bool IsProcessElevated( HANDLE hProcess /*= ::GetCurrentProcess()*/ )
	{
		ASSERT_PTR( hProcess );
		fs::CHandle token;
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
		fs::CHandle process( ::OpenProcess( access, FALSE, processId ) );
		return IsProcessElevated( process.Get() );
	}

	bool HasSameElevation( HANDLE hProcess1, HANDLE hProcess2 )
	{
		return hProcess1 != NULL && hProcess2 != NULL && IsProcessElevated( hProcess1 ) == IsProcessElevated( hProcess2 );
	}

	bool HasCurrentElevation( HWND hWnd, DWORD access /*= SYNCHRONIZE*/ )
	{
		ASSERT( ui::IsValidWindow( hWnd ) );

		fs::CHandle wndProcess( ::OpenProcess( access, FALSE, ui::GetWindowProcessId( hWnd ) ) );
		return HasCurrentElevation( wndProcess.Get() );
	}
}

#endif //_CONSOLE
