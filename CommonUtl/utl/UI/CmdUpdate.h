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


namespace ui
{
	class CSmartCmdUI : public ::CCmdUI		// does incremental updates: only if the state actually changes
	{
	public:
		CSmartCmdUI( void ) {}

		virtual void Enable( BOOL on = TRUE );
		virtual void SetCheck( int check = 1 );		// 0, 1 or 2 (indeterminate)
		virtual void SetRadio( BOOL on = TRUE );
		virtual void SetText( LPCTSTR pText );

		bool IsCtrlUpdate( void ) const { return nullptr == m_pMenu && m_pOther != nullptr; }
	};
}


#endif // CmdUpdate_h
