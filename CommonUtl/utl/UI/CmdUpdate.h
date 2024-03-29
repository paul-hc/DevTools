#ifndef CmdUpdate_h
#define CmdUpdate_h
#pragma once


namespace ui
{
	std::tstring GetCmdText( const CCmdUI* pCmdUI );

	void SetRadio( CCmdUI* pCmdUI, BOOL checked = BST_CHECKED );
	bool ExpandVersionInfoTags( CCmdUI* pCmdUI );				// based on CVersionInfo

	void UpdateMenuUI( CWnd* pTargetWnd, CMenu* pMenu, bool autoMenuEnable = true, bool isTracking = false, RecursionDepth depth = Shallow );

	void UpdateDlgControlsUI( HWND hDlg, CCmdTarget* pTarget = nullptr, bool disableIfNoHandler = false );
	void UpdateDlgControlsUI( HWND hDlg, const UINT ctrlIds[], size_t count, CCmdTarget* pTarget = nullptr, bool disableIfNoHandler = false );

	bool UpdateControlUI( HWND hCtrl, CCmdTarget* pTarget = nullptr, bool disableIfNoHandler = false );
	inline bool UpdateDlgItemUI( CWnd* pDlg, UINT ctrlId ) { return UpdateControlUI( ::GetDlgItem( pDlg->GetSafeHwnd(), ctrlId ), pDlg ); }
}


namespace ui
{
	// standard message handlers

	void HandleInitMenuPopup( CWnd* pTargetWnd, CMenu* pPopupMenu, bool isUserMenu );		// WM_INITMENUPOPUP: send CCmdUI updates
}


#endif // CmdUpdate_h
