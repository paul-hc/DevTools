#pragma once

#include "utl/UI/ToolbarStrip.h"


class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();
protected:
	CStatusBar  m_wndStatusBar;
	CToolbarStrip m_wndToolBar;

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
protected:
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg LRESULT OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};
