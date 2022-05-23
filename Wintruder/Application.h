#ifndef Wintruder_h
#define Wintruder_h
#pragma once

#include "utl/UI/BaseApp.h"


enum MainPage { GeneralTopPage, WindowsTopPage, OptionsTopPage };
enum DetailPage { CaptionDetail, IdMenuDetail, StyleDetail, ExtendedStyleDetail, PlacementDetail };


class CApplication : public CBaseApp<CWinApp>
{
public:
	CApplication( void );
	virtual ~CApplication();

	CToolTipCtrl* GetMainTooltip( void );

	void ShowAppPopups( bool show );
private:
	bool RelayTooltipEvent( MSG* pMsg );
private:
	CAccelTable m_sharedAccel;
	CToolTipCtrl m_mainTooltip;

	// generated overrides
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnFileClose( void );
	afx_msg void CmRestore( void );
	afx_msg void CmMinimize( void );
	afx_msg void CmRefresh( void );
	afx_msg void CmRefreshBranch( void );
	afx_msg void CmActivateWindow( void );
	afx_msg void OnUpdateActivateWindow( CCmdUI* pCmdUI );
	afx_msg void CmShowWindow( void );
	afx_msg void OnUpdateShowWindow( CCmdUI* pCmdUI );
	afx_msg void CmHideWindow( void );
	afx_msg void OnUpdateHideWindow( CCmdUI* pCmdUI );
	afx_msg void CmToggleTopmostWindow( void );
	afx_msg void OnUpdateTopmostWindow( CCmdUI* pCmdUI );
	afx_msg void CmToggleEnableWindow( void );
	afx_msg void OnUpdateEnableWindow( CCmdUI* pCmdUI );
	afx_msg void CmMoveWindow( UINT cmdId );
	afx_msg void OnUpdateMoveWindow( CCmdUI* pCmdUI );
	afx_msg void CmRedrawDesktop( void );

	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;


class CMainDialog;
class CWindowTimer;


namespace app
{
	enum ContextPopup { SysTrayPopup, TreePopup, HighlightSplitButton, FindSplitButton, ResetSplitButton, FlagsListPopup };

	inline CApplication* GetApplication( void ) { return &theApp; }
	inline CMainDialog* GetMainDialog( void ) { return (CMainDialog*)AfxGetMainWnd(); }
	inline CToolTipCtrl* GetMainTooltip( void ) { return GetApplication()->GetMainTooltip(); }

	void DrillDownDetail( DetailPage detailPage );
}


#endif // Wintruder_h
