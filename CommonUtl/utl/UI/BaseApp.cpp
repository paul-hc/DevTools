
#include "stdafx.h"
#include "BaseApp.h"
#include "EnumTags.h"
#include "MenuUtilities.h"
#include "RuntimeException.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//ATL::CTraceCategory traceThumbs( _T("UTL Thumbnails") );


namespace app
{
	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos )
	{
		ui::StdPopup popup = ui::AppMainPopup;
	#ifdef _DEBUG
		popup = ui::AppDebugPopup;
	#endif

		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, popup );
		ui::TrackPopupMenu( contextMenu, pTargetWnd, screenPos, TPM_RIGHTBUTTON );
	}

	UINT ToMsgBoxFlags( app::MsgType msgType )
	{
		switch ( msgType )
		{
			case app::Error:	return MB_ICONERROR;
			case app::Warning:	return MB_ICONWARNING;
			default:
				ASSERT( false );
			case app::Info:		return MB_ICONINFORMATION;
		}
	}

	void TraceOsVersion( void )
	{
		TRACE( _T(" > Running on OS: %s\n"), win::GetTags_OsVersion().FormatUi( win::GetOsVersion() ).c_str() );
	}
}


namespace win
{
	OsVersion GetOsVersionImpl( void );

	const CEnumTags& GetTags_OsVersion( void )
	{
		static const CEnumTags s_tags( _T("n/a|Windows 2000|Windows XP|Windows Vista|Windows 7|Windows 8|Windows 10|WinBeyond++") );
		return s_tags;
	}

	OsVersion GetOsVersion( void )
	{
		static const OsVersion osVersion = GetOsVersionImpl();
		return osVersion;
	}

	OsVersion GetOsVersionImpl( void )
	{
		OSVERSIONINFO osvi;
		ZeroMemory( &osvi, sizeof( OSVERSIONINFO ) );
		osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
		if ( !::GetVersionEx( &osvi ) )
			return NotAvailable;

		switch ( osvi.dwMajorVersion )
		{
			case 5:
				switch ( osvi.dwMinorVersion )
				{
					case 0:		return Win2K;
					case 1:
					default:	return WinXP;			// 5.1=Windows XP, 5.2=Windows Server 2003
				}
			case 6:
				switch ( osvi.dwMinorVersion )
				{
					case 0:		return WinVista;
					case 1:		return Win7;
					case 2:								// 6.2=Windows 8	(!) this will be returned for Windows 10 when the application is not targeted (manifested) for Windows 10
					case 3:		return Win8;			// 6.3=Windows 8.1
					default:	return WinXP;			// 5.1=Windows XP, 5.2=Windows Server 2003
				}
			case 10:
				switch ( osvi.dwMinorVersion )
				{
					case 0:
					case 1:
					default:	return Win10;			// application must be targeted (manifested) for Windows 10
				}
		}
		return WinBeyond;
	}
}
