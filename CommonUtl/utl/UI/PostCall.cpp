
#include "StdAfx.h"
#include "PostCall.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const UINT CBasePostCall::WM_DELAYED_CALL = RegisterWindowMessageA( "ui::WM_DELAYED_CALL" );


CBasePostCall::CBasePostCall( CWnd* pWnd )
	: CWindowHook( true )
{
	HookWindow( pWnd->GetSafeHwnd() );
	PostCall( pWnd );						// post the delayed method call
}

CBasePostCall::~CBasePostCall()
{
}

LRESULT CBasePostCall::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
{
	if ( WM_DELAYED_CALL == message )
		if ( (void*)wParam == this )		// is this our call in a multiple posted sequence?
		{
			OnCall();
			UnhookWindow();
			return 0L;
		}

	return __super::WindowProc( message, wParam, lParam );
}

bool CBasePostCall::PostCall( CWnd* pWnd )
{
	// pass this pointer as wParam so we can identify this call in a multiple posted sequence
	if ( !pWnd->PostMessage( WM_DELAYED_CALL, (WPARAM)(void*)this ) )
	{
		TRACE( _T(" * Failed PostCall on %s\n"), str::mfc::GetTypeName( pWnd ).GetString() );
		delete this; // failed
		return false;
	}

	return true;
}
