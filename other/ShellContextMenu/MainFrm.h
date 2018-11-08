#pragma once

#include "ChildView.h"


class CMainFrame : public CFrameWnd
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();
public:
	CChildView m_wndView;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg void OnSetFocus( CWnd *pOldWnd );
	afx_msg void OnViewMode( UINT cmdId );
	afx_msg void OnUpdateViewMode( CCmdUI* pCmdUI );
	afx_msg void OnUseCustomMenu( void );
	afx_msg void OnUpdateUseCustomMenu( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};
