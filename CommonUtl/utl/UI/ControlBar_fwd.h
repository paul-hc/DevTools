#ifndef ControlBar_fwd_h
#define ControlBar_fwd_h
#pragma once


class CBasePane;
class CMFCStatusBar;
class CMFCStatusBarPaneInfo;


namespace mfc
{
	// CBasePane protected access:
	void BasePane_SetIsDialogControl( CBasePane* pBasePane, bool isDlgControl = true );		// getter IsDialogControl() is public


	// CMFCStatusBar protected access:
	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index );
	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo );
	inline int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, int index ) { return StatusBar_CalcPaneTextWidth( pStatusBar, StatusBar_GetPaneInfo( pStatusBar, index ) ); }
	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index );
}


#endif // ControlBar_fwd_h
