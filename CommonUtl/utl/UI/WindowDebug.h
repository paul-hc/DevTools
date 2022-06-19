#ifndef WindowDebug_h
#define WindowDebug_h
#pragma once


class CTrayIcon;


namespace dbg
{
	void TraceWindow( HWND hWnd, const TCHAR tag[] );

	void TraceTrayNotifyCode( UINT msgNotifyCode );


	class CScopedTrayIconDiagnostics
	{
	public:
		CScopedTrayIconDiagnostics( const CTrayIcon* pTrayIcon, UINT msgNotifyCode );
		~CScopedTrayIconDiagnostics();
	private:
		const CTrayIcon* m_pTrayIcon;
		std::tstring m_preMsg;
	};
}


#endif // WindowDebug_h
