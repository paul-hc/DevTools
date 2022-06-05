#ifndef ISystemTrayCallback_h
#define ISystemTrayCallback_h
#pragma once


namespace ui
{
	interface ISystemTrayCallback
	{
		// window that handles the context menu commands
		virtual CWnd* GetOwnerWnd( void ) = 0;

		// gets default command for handling icon double-click
		virtual CMenu* GetTrayIconContextMenu( void ) = 0;

		// receives tray notifications, returns true if was event handled by the owner
		virtual bool OnTrayIconNotify( UINT msgNotifyCode, UINT iconId, const CPoint& screenPos ) = 0;
	};
}


#endif // ISystemTrayCallback_h
