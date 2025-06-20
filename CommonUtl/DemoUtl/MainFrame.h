#pragma once

#include "utl/UI/BaseFrameWnd.h"
#include "utl/UI/ToolbarStrip.h"


class CDemoSysTray;
typedef CBaseFrameWnd<CMDIFrameWnd> TMDIFrameWnd;


class CMainFrame : public TMDIFrameWnd
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();
private:
	void InitSystemTray( void );
protected:
	CStatusBar m_wndStatusBar;
	CToolbarStrip m_wndToolBar;
	std::auto_ptr<CDemoSysTray> m_pDemoSysTray;

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override;
protected:
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg LRESULT OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};
