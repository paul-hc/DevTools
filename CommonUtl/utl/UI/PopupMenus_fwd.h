#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once


class CMFCPopupMenu;
class CMFCToolBar;
class CMFCToolBarButton;

class CMFCColorPopupMenu;
class CMFCColorBar;


namespace ui
{
	interface ICustomPopupMenu
	{
		virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) = 0;
	};
}


namespace mfc
{
	void* GetItemData( const CMFCToolBarButton* pButton );
	void* GetButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu );
	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId );
	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCColorBar* GetColorMenuBar( CMFCColorPopupMenu* pColorPopupMenu );
}


#endif // PopupMenus_fwd_h
