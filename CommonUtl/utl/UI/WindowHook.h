#ifndef WindowHook_h
#define WindowHook_h
#pragma once

#include <hash_map>


// Provides a convenient way of hooking windows (sub-classing the window procedure).
// It works potentially in non-window classes and for non-CWnd wrapped windows.
// It's used typically by deriving from CWindowHook and overriding WindowProc method.
// Note:
//		Many such objects might be instantiated simultaneously for the same window handle.

class CWindowHook : private utl::noncopyable
{
public:
	CWindowHook( bool autoDelete = false );
	virtual ~CWindowHook();

	operator HWND() const { return m_hWnd; }
	HWND GetHwnd() const { return m_hWnd; }

	bool IsWindow( void ) const { return m_hWnd != NULL && ::IsWindow( m_hWnd ); }
	bool IsHooked( void ) const { ASSERT( NULL == m_hWnd || ::IsWindow( m_hWnd ) ); return m_hWnd != NULL; }

	virtual void HookWindow( HWND hWndToHook );
	virtual bool UnhookWindow( void );

	// CWnd-like overrides for message processing
	LRESULT Default( void );
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
private:
	// hooked windows callback
	static LRESULT CALLBACK HookedWindowsProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
private:
	void RegisterHook( HWND hWndToHook );
	void UnregisterHook( void );
	static void UnregisterAllHooksForWindow( HWND hWndHooked );
private:
	typedef stdext::hash_map< HWND, CWindowHook* > HookMap;

	static CWindowHook* FindHook( HWND hWndHooked );
	static HookMap& GetHookMap( void );
public:
	bool m_autoDelete;			// automatically deletes this when the window is unhooked
protected:
	HWND m_hWnd;				// handle of the hooked (sub-classed) window
private:
	WNDPROC m_pOrgWndProc;		// hooked window original window callback procedure (replaced by HookedWindowsProc)
	CWindowHook* m_pNextHook;	// next in the hook chain for m_hWnd
};


#endif // WindowHook_h