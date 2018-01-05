#ifndef TooltipsHook_h
#define TooltipsHook_h
#pragma once

#include "WindowHook.h"


class CTooltipsHook : public CWindowHook
{
public:
	CTooltipsHook( HWND hWndToHook ) { if ( hWndToHook != NULL ) HookWindow( hWndToHook ); }
	virtual ~CTooltipsHook() {}
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
protected:
	bool OnTtnNeedText( NMHDR* pNmHdr );
};


#endif // TooltipsHook_h
