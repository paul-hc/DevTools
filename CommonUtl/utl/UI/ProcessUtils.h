#ifndef ProcessUtils_h
#define ProcessUtils_h
#pragma once


namespace proc
{
	inline bool InCurrentThread( HWND hWnd ) { ASSERT_PTR( hWnd ); return ::GetWindowThreadProcessId( hWnd, nullptr ) == ::GetCurrentThreadId(); }
	inline bool InDifferentThread( HWND hWnd ) { return !InCurrentThread( hWnd ); }

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
		, m_wndThreadId( ::GetWindowThreadProcessId( hWnd, nullptr ) )
		, m_differentThread( m_currThreadId != m_wndThreadId )
		, m_attached( m_differentThread && ::AttachThreadInput( m_currThreadId, m_wndThreadId, TRUE ) != FALSE )
	{
		ASSERT( ::IsWindow( hWnd ) );
	}

	~CScopedAttachThreadInput()
	{
		if ( m_attached )
			::AttachThreadInput( m_currThreadId, m_wndThreadId, FALSE );		// un-attach this thread from hWnd's thread
	}

	bool DifferentThread( void ) const { return m_differentThread; }
private:
	DWORD m_currThreadId;
	DWORD m_wndThreadId;
	bool m_differentThread;
	bool m_attached;
};


#endif // ProcessUtils_h
