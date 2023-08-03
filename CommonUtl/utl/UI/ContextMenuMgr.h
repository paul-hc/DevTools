#ifndef ContextMenuMgr_h
#define ContextMenuMgr_h
#pragma once

#include <afxcontextmenumanager.h>


namespace mfc
{
	class CContextMenuMgr : public CContextMenuManager
	{
	public:
		CContextMenuMgr( void );
		virtual ~CContextMenuMgr();

		CMFCPopupMenu* GetTrackingPopupMenu( void ) const;
		void ResetTrackingPopup( CMFCPopupMenu* pNewTrackingPopup );		// custom tracking popup menu: used instead of creating a 'new CMFCPopupMenu' in ShowPopupMenu()

		static CContextMenuMgr* Instance( void ) { return s_pInstance; }

		UINT TrackModalPopup( OPTIONAL HMENU hMenuPopup, CWnd* pTargetWnd, bool sendCommand, CPoint screenPos = CPoint( -1, -1 ), bool rightAlign = false );
		UINT TrackModalPopup( CMFCPopupMenu* pPopupMenu, CWnd* pTargetWnd, bool sendCommand, CPoint screenPos = CPoint( -1, -1 ), bool rightAlign = false );

		// base overrides
	public:
		virtual CMFCPopupMenu* ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage = FALSE, BOOL bAutoDestroy = TRUE, BOOL bRightAlign = FALSE ) override;
		virtual UINT TrackPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bRightAlign = FALSE ) override;
	private:
		std::auto_ptr<CMFCPopupMenu> m_pNewTrackingPopup;
		CMFCPopupMenu* m_pTrackingPopupMenu;		// temporary set during tracking

		static CContextMenuMgr* s_pInstance;
	};
}


#endif // ContextMenuMgr_h
