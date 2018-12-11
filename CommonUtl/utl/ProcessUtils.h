#ifndef ProcessUtils_h
#define ProcessUtils_h
#pragma once

#include <process.h>


namespace utl
{
	class CProcessCmd
	{
	public:
		CProcessCmd( const TCHAR* pExePath ) : m_exePath( pExePath ) { ASSERT( !m_exePath.empty() ); }

		template< typename ValueT >
		void AddParam( const ValueT& value ) { m_params.push_back( arg::AutoEnquote( value ) ); }

		int Execute( void ) { return static_cast< int >( ExecuteProcess( _P_WAIT ) ); }			// waits for completion
		HANDLE Spawn( int mode = _P_NOWAIT ) { return reinterpret_cast< HANDLE >( ExecuteProcess( mode ) ); }
	private:
		intptr_t ExecuteProcess( int mode );
		const TCHAR* const* MakeArgList( std::vector< const TCHAR* >& rArgList ) const;
	private:
		std::tstring m_exePath;
		std::vector< std::tstring > m_params;
	};
}


#ifndef _CONSOLE

namespace proc
{
	bool IsProcessElevated( HANDLE hProcess = ::GetCurrentProcess() );
	bool IsProcessElevated( DWORD processId, DWORD access = SYNCHRONIZE );

	bool HasSameElevation( HANDLE hProcess1, HANDLE hProcess2 );
	inline bool HasCurrentElevation( HANDLE hProcess ) { return HasSameElevation( hProcess, ::GetCurrentProcess() ); }
	bool HasCurrentElevation( HWND hWnd, DWORD access = SYNCHRONIZE );
}


// attach this thread to hWnd's thread in order to acces the focused window, active window, etc
struct CScopedAttachThreadInput
{
	CScopedAttachThreadInput( HWND hWnd )
		: m_currThreadId( ::GetCurrentThreadId() )
		, m_wndThreadId( ::GetWindowThreadProcessId( hWnd, NULL ) )
		, m_attached( m_currThreadId != m_wndThreadId && ::AttachThreadInput( m_currThreadId, m_wndThreadId, TRUE ) != FALSE )
	{
		ASSERT_PTR( ::IsWindow( hWnd ) );
	}

	~CScopedAttachThreadInput()
	{
		if ( m_attached )
			::AttachThreadInput( m_currThreadId, m_wndThreadId, FALSE );		// un-attach this thread from hWnd's thread
	}
private:
	DWORD m_currThreadId;
	DWORD m_wndThreadId;
	bool m_attached;
};

#endif //_CONSOLE


#endif // ProcessUtils_h
