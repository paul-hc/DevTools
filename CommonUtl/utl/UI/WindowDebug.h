#ifndef WindowDebug_h
#define WindowDebug_h
#pragma once


namespace dbg
{
	void TraceWindow( HWND hWnd, const TCHAR tag[] );

	void TraceTrayNotifyCode( UINT msgNotifyCode );
}


#endif // WindowDebug_h
