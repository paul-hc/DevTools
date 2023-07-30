
#include "pch.h"
#include "WindowHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWindowHook::CWindowHook( bool autoDelete /*= false*/ )
	: m_autoDelete( autoDelete )
	, m_hWnd( nullptr )
	, m_pOrgWndProc( nullptr )
	, m_pNextHook( nullptr )
	, m_pHookHandler( nullptr )
{
}

CWindowHook::~CWindowHook()
{
	ASSERT( nullptr == m_hWnd && nullptr == m_pOrgWndProc );			// can't destroy while still hooked
}

void CWindowHook::HookWindow( HWND hWndToHook )
{
	ASSERT( ::IsWindow( hWndToHook ) );

	if ( IsHooked() )
	{	// unhook previous hooked window
		UnhookWindow();
		TRACE( _T("@ CWindowHook::HookWindow(): window already hooked to 0x%08X, replacing with 0x%08X !\n"), m_hWnd, hWndToHook );
	}

	RegisterHook( hWndToHook );
}

bool CWindowHook::UnhookWindow( void )
{
	ASSERT( !m_autoDelete || IsHooked() );

	if ( !IsHooked() )
		return false;

	UnregisterHook();
	m_hWnd = nullptr;
	m_pOrgWndProc = nullptr;
	m_pNextHook = nullptr;

	if ( m_autoDelete )
		delete this;
	return true;
}

// just like CWnd::WindowProc(), will be overridden by sub-classes;
// default processing passes the message to the next hook,
// the last hook passes the message to the original window-proc.

LRESULT CWindowHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	ASSERT_PTR( m_pOrgWndProc );

	if ( m_pHookHandler != nullptr )
	{
		LRESULT lResult = -333;

		if ( m_pHookHandler->Handle_HookMessage( lResult, AfxGetThreadState()->m_lastSentMsg, this ) )
		{
			ENSURE( lResult != -333 );		// custom handler must have explicitly set the result
			return lResult;
		}
	}

	return m_pNextHook != nullptr
		? m_pNextHook->WindowProc( message, wParam, lParam )
		: ::CallWindowProc( m_pOrgWndProc, m_hWnd, message, wParam, lParam );
}

LRESULT CWindowHook::Default( void )
{
	// MFC stores current MSG in thread state
	MSG& rLastMsg = AfxGetThreadState()->m_lastSentMsg;

	// non-virtual call to avoid infinte recursion on virtual function
	return CWindowHook::WindowProc( rLastMsg.message, rLastMsg.wParam, rLastMsg.lParam );
}

// shared window-proc for registered hook objects

LRESULT CALLBACK CWindowHook::HookedWindowsProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
#ifdef _USRDLL
	// if this is a DLL, need to set up MFC state
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
#endif

	// Set up MFC message state just in case anyone wants it.
	// This is just like AfxCallWindowProc, but we can't use that because a CWindowHook is not a CWnd.
	MSG& rLastMsg = AfxGetThreadState()->m_lastSentMsg;
	MSG oldMsg = rLastMsg;		// save it for nesting

	rLastMsg.hwnd = hWnd;
	rLastMsg.message = message;
	rLastMsg.wParam = wParam;
	rLastMsg.lParam = lParam;

	CWindowHook* pHookedWindow = FindHook( hWnd );
	LRESULT result;

	ASSERT_PTR( pHookedWindow );
	if ( WM_NCDESTROY == message )
	{
		WNDPROC pOrgWndProc = pHookedWindow->m_pOrgWndProc;

		// window is being destroyed: unhook all hooks (for this window) and pass message to original window proc
		UnregisterAllHooksForWindow( hWnd );
		result = ::CallWindowProc( pOrgWndProc, hWnd, message, wParam, lParam );
	}
	else
		result = pHookedWindow->WindowProc( message, wParam, lParam ); // call the message hook

	rLastMsg = oldMsg;			// pop previous state
	return result;
}

CWindowHook::THookMap& CWindowHook::GetHookMap( void )
{
	static THookMap s_mapOfWindowHooks;

	return s_mapOfWindowHooks;
}

CWindowHook* CWindowHook::FindHook( HWND hWndHooked )
{
	THookMap::const_iterator itHook = GetHookMap().find( hWndHooked );
	return itHook != GetHookMap().end() ? itHook->second : nullptr;
}

void CWindowHook::RegisterHook( HWND hWndToHook )
{
	ASSERT( ::IsWindow( hWndToHook ) );

	m_pNextHook = FindHook( hWndToHook );

	GetHookMap()[ hWndToHook ] = this;
	if ( nullptr == m_pNextHook )
		// this is the first hook added -> subclass the window
		m_pOrgWndProc = (WNDPROC)::SetWindowLongPtr( hWndToHook, GWLP_WNDPROC, (LONG_PTR)&CWindowHook::HookedWindowsProc );
	else
		m_pOrgWndProc = m_pNextHook->m_pOrgWndProc;		// just copy window-proc from the next hook

	ASSERT_PTR( m_pOrgWndProc );
	m_hWnd = hWndToHook;
}

void CWindowHook::UnregisterHook( void )
{
	CWindowHook* pTheHook = FindHook( m_hWnd );

	ASSERT( m_hWnd != nullptr && ::IsWindow( m_hWnd ) );
	ASSERT( pTheHook != nullptr );
	if ( pTheHook == this )
		if ( pTheHook->m_pNextHook != nullptr )
			GetHookMap()[ m_hWnd ] = pTheHook->m_pNextHook;
		else
		{	// the last hook for this window, restore original window-proc
			GetHookMap().erase( m_hWnd );
			::SetWindowLongPtr( m_hWnd, GWLP_WNDPROC, (LONG_PTR)pTheHook->m_pOrgWndProc );
		}
	else
	{	// remove in the middle
		while ( pTheHook->m_pNextHook != this )
			pTheHook = pTheHook->m_pNextHook;

		ASSERT( pTheHook != nullptr && pTheHook->m_pNextHook == this );
		pTheHook->m_pNextHook = m_pNextHook;
	}
}

void CWindowHook::UnregisterAllHooksForWindow( HWND hWndHooked )
{	// remove all the hooks for the specified window
	while ( CWindowHook* pHookedWindow = FindHook( hWndHooked ) )
		pHookedWindow->UnhookWindow();
}
