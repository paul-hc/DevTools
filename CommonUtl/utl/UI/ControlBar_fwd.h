#ifndef ControlBar_fwd_h
#define ControlBar_fwd_h
#pragma once


class CBasePane;
class CMFCToolBar;
class CMFCStatusBar;
class CMFCStatusBarPaneInfo;
class CToolTipCtrl;
class CDockingManager;


namespace mfc
{
	// CBasePane protected access:
	void BasePane_SetIsDialogControl( CBasePane* pBasePane, bool isDlgControl = true );		// getter IsDialogControl() is public


	// CMFCToolBar protected access:
	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar );


	// CMFCStatusBar protected access:
	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index );
	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo );
	inline int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, int index ) { return StatusBar_CalcPaneTextWidth( pStatusBar, StatusBar_GetPaneInfo( pStatusBar, index ) ); }
	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index );
}


namespace mfc
{
	// global control bars:

	void QueryAllCustomizableControlBars( OUT std::vector<CMFCToolBar*>& rToolbars, CWnd* pFrameWnd = AfxGetMainWnd() );
	void ResetAllControlBars( CWnd* pFrameWnd = AfxGetMainWnd() );


	// FrameWnd:

	enum FrameToolbarStyle
	{
		FirstToolbarStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
		AdditionalToolbarStyle = FirstToolbarStyle | CBRS_HIDE_INPLACE | CBRS_BORDER_3D
	};


	void DockPanesOnRow( CDockingManager* pFrameDocManager, size_t barCount, CMFCToolBar* pFirstToolBar, ... );
}


#endif // ControlBar_fwd_h
