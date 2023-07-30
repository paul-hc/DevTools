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
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
protected:
	bool OnTtnNeedText( NMHDR* pNmHdr );
protected:
	ui::ICustomCmdInfo* m_pCustomCmdInfo;
};


namespace ui { interface IToolTipsHandler; }


// hook created to redirect TTN_NEEDTEXT notifications to objects implementing IToolTipsHandler, for windows that have their own default tooltip processing
//
class CToolTipsHandlerHook : public CWindowHook
{
	CToolTipsHandlerHook( ui::IToolTipsHandler* pToolTipsHandler, CToolTipCtrl* pToolTip );
public:
	virtual ~CToolTipsHandlerHook();

	static CWindowHook* CreateHook( CWnd* pToolTipOwnerWnd, ui::IToolTipsHandler* pToolTipsHandler, CToolTipCtrl* pToolTip );		// by default managed by the client (m_autoDelete = false)
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
protected:
	bool OnTtnNeedText( NMHDR* pNmHdr );		// handles only TTN_NEEDTEXT (which is TTN_NEEDTEXTW on Unicode builds)
protected:
	ui::IToolTipsHandler* m_pToolTipsHandler;
	CToolTipCtrl* m_pToolTip;
};


#endif // TooltipsHook_h
