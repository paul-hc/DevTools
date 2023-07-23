#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once


class CMFCPopupMenu;
class CMFCToolBar;
class CMFCToolBarButton;


namespace ui
{
	interface ICustomPopupMenu
	{
		virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) = 0;
	};
}


namespace mfc
{
	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu );
	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId );
	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	void* GetItemData( const CMFCToolBarButton* pButton );
	void* GetButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );
}


#endif // PopupMenus_fwd_h
