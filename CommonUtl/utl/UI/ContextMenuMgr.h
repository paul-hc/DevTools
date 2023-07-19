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

		void ResetNewTrackingPopupMenu( mfc::CTrackingPopupMenu* pNewTrackingPopupMenu );

		static CContextMenuMgr* Instance( void ) { return s_pInstance; }

		// base overrides
		virtual CMFCPopupMenu* ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage = FALSE, BOOL bAutoDestroy = TRUE, BOOL bRightAlign = FALSE );
	private:
		std::auto_ptr<mfc::CTrackingPopupMenu> m_pNewTrackingPopupMenu;
		static CContextMenuMgr* s_pInstance;
	};
}


#endif // ContextMenuMgr_h
