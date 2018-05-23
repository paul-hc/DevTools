#pragma once


class CMainFrame : public CMDIFrameWnd
{
public:
	CMainFrame( void );
	virtual ~CMainFrame();
private:
	void SaveWindowPlacement( void );
	void LoadWindowPlacement( CREATESTRUCT& rCreateStruct );
private:
	// embedded control bars
	CStatusBar m_wndStatusBar;
	CToolBar m_wndToolBar;
	CReBar m_wndReBar;
	CDialogBar m_wndDlgBar;

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& rCreateStruct );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnClose( void );

	DECLARE_MESSAGE_MAP()
};
