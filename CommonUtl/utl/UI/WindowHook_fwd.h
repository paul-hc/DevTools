#ifndef WindowHook_fwd_h
#define WindowHook_fwd_h
#pragma once


class CWindowHook;


namespace ui
{
	// implemented by an object that overrides the default handling of TTN_NEEDTEXT notifications
	//
	interface IWindowHookHandler
	{
		virtual bool Handle_HookMessage( OUT LRESULT& rResult, const MSG& msg, const CWindowHook* pHook ) = 0;		// returns true if message handled, and false to continue default handling
	};
}


#endif // WindowHook_fwd_h
