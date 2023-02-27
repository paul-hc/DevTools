#ifndef TooltipsHook_h
#define TooltipsHook_h
#pragma once

#include "WindowHook.h"


namespace ui { interface ICustomCmdInfo; }


class CTooltipsHook : public CWindowHook
{
public:
	CTooltipsHook( HWND hWndToHook = nullptr );

	void HookControl( CWnd* pCtrlToHook );
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
protected:
	bool OnTtnNeedText( NMHDR* pNmHdr );
protected:
	ui::ICustomCmdInfo* m_pCustomCmdInfo;
};


#endif // TooltipsHook_h
