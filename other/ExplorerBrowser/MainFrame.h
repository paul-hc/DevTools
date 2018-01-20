#pragma once


class CMainFrame : public CMDIFrameWnd
{
public:
	CMainFrame( void );
	virtual ~CMainFrame();
private:
	// embedded control bars
	CStatusBar m_wndStatusBar;
	CToolBar m_wndToolBar;
	CReBar m_wndReBar;
	CDialogBar m_wndDlgBar;

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );

	DECLARE_MESSAGE_MAP()
};
