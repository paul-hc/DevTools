#ifndef CmdUpdate_h
#define CmdUpdate_h
#pragma once


namespace ui
{
	std::tstring GetCmdText( const CCmdUI* pCmdUI );

	void SetRadio( CCmdUI* pCmdUI, BOOL checked = BST_CHECKED );
	bool ExpandVersionInfoTags( CCmdUI* pCmdUI );				// based on CVersionInfo

	void UpdateMenuUI( CWnd* pWindow, CMenu* pPopupMenu, bool autoMenuEnable = true );

	bool UpdateControlUI( CWnd* pCtrl, CWnd* pTargetWnd = NULL );
	inline bool UpdateDlgItemUI( CWnd* pDlg, UINT ctrlId ) { return UpdateControlUI( pDlg->GetDlgItem( ctrlId ), pDlg ); }

	void UpdateControlsUI( CWnd* pParent, CWnd* pTargetWnd = NULL );
	void UpdateControlsUI( CWnd* pParent, const UINT ctrlIds[], size_t count, CWnd* pTargetWnd = NULL );
}


#endif // CmdUpdate_h
