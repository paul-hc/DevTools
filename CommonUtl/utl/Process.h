#ifndef Process_h
#define Process_h
#pragma once


class CHandle : private utl::noncopyable
{
public:
	CHandle( HANDLE handle = NULL ) : m_handle( handle ) {}
	~CHandle() { Close(); }

	HANDLE Get( void ) const { return m_handle; }
	HANDLE* GetPtr( void ) { return &m_handle; }

	void Reset( HANDLE handle )
	{
		Close();
		m_handle = handle;
	}

	HANDLE Release( void )
	{
		HANDLE handle = m_handle;
		m_handle = NULL;
		return handle;
	}

	void Close( void )
	{
		if ( m_handle != NULL )
			::CloseHandle( m_handle );
	}
private:
	HANDLE m_handle;
};


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


#endif // Process_h
