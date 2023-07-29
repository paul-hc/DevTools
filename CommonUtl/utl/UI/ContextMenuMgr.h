#ifndef ContextMenuMgr_h
#define ContextMenuMgr_h
#pragma once

#include <afxcontextmenumanager.h>


namespace mfc
{
	class CTrackingPopupMenu;


	class CContextMenuMgr : public CContextMenuManager
	{
	public:
		CContextMenuMgr( void );
		virtual ~CContextMenuMgr();

		CMFCPopupMenu* GetTrackingPopupMenu( void ) const;
		void ResetNewTrackingPopupMenu( mfc::CTrackingPopupMenu* pNewTrackingPopupMenu );		// custom tracking popup menu: used instead of creating a 'new CMFCPopupMenu' in ShowPopupMenu()

		static CContextMenuMgr* Instance( void ) { return s_pInstance; }

		// base overrides
		virtual CMFCPopupMenu* ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage = FALSE, BOOL bAutoDestroy = TRUE, BOOL bRightAlign = FALSE );
		virtual UINT TrackPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bRightAlign = FALSE );
	private:
		std::auto_ptr<mfc::CTrackingPopupMenu> m_pNewTrackingPopupMenu;
		CMFCPopupMenu* m_pTrackingPopupMenu;		// temporary set during tracking

		static CContextMenuMgr* s_pInstance;
	};
}


#endif // ContextMenuMgr_h
