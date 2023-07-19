#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once


namespace mfc { class CTrackingPopupMenu; }


namespace ui
{
	interface ICustomPopupMenu
	{
		virtual void OnCustomizeMenuBar( mfc::CTrackingPopupMenu* pMenuPopup ) = 0;
	};
}


#endif // PopupMenus_fwd_h
