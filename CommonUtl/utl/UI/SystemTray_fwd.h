#ifndef SystemTray_fwd_h
#define SystemTray_fwd_h
#pragma once

#include "utl/AppTools.h"


namespace sys_tray
{
	bool HasSystemTray( void );

	// baloon tips on the message tray icon (usually auto-hide)
	bool ShowBalloonMessage( const std::tstring& text, const TCHAR* pTitle = nullptr, app::MsgType msgType = app::Info, UINT timeoutSecs = 0 );
	bool HideBalloonMessage( void );
}


class CTrayIcon;
namespace ui { interface ISystemTrayCallback; }


class CScopedTrayIconBalloon
{
public:
	CScopedTrayIconBalloon( CTrayIcon* pTrayIcon /*= nullptr*/, const std::tstring& text, const TCHAR* pTitle = nullptr, app::MsgType msgType = app::Info, UINT timeoutSecs = 30 );
	~CScopedTrayIconBalloon();
private:
	CTrayIcon* m_pTrayIcon;
};


#endif // SystemTray_fwd_h
