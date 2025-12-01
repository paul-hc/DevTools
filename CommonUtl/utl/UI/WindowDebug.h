#ifndef WindowDebug_h
#define WindowDebug_h
#pragma once


class CFlagTags;


#ifdef _DEBUG

namespace dbg
{
	const CFlagTags& GetTags_WndStyle( void );
	const CFlagTags& GetTags_WndStyleEx( void );
}

#endif //_DEBUG


class CTrayIcon;


namespace dbg
{
	void TraceWindow( HWND hWnd, const TCHAR tag[] );

	void TraceTrayNotifyCode( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos );


	class CScopedTrayIconDiagnostics
	{
	public:
		CScopedTrayIconDiagnostics( const CTrayIcon* pTrayIcon, UINT msgNotifyCode );
		~CScopedTrayIconDiagnostics();
	private:
		const CTrayIcon* m_pTrayIcon;
		std::tstring m_preMsg;
	};

	std::tstring FormatWndInfo( HWND hWnd, const TCHAR tag[] );
}


#endif // WindowDebug_h
