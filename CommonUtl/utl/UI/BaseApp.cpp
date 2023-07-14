
#include "pch.h"
#include "BaseApp.h"
#include "EnumTags.h"
#include "MenuUtilities.h"
#include "RuntimeException.h"
#include "ShellTypes.h"
#include "ProcessUtils.h"
#include "WindowDebug.h"
#include "resource.h"
#include "utl/FileEnumerator.h"

#ifdef _DEBUG
#include "utl/test/Test.h"
#include "ImageStore.h"
#include "WndUtils.h"

#define new DEBUG_NEW
#endif


//ATL::CTraceCategory traceThumbs( _T("UTL Thumbnails") );


namespace app
{
	void InitUtlBase( void )
	{
		// inject UTL_UI.lib code into UTL_BASE.lib:
		fs::StoreResolveShortcutProc( &shell::ResolveShortcut );
	}

	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos )
	{
		ui::StdPopup popup = ui::AppMainPopup;
	#ifdef _DEBUG
		popup = ui::AppDebugPopup;
	#endif

		ui::TrackContextMenu( IDR_STD_CONTEXT_MENU, popup, pTargetWnd, screenPos );
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


#ifdef _DEBUG
	void ReportTestResults( void );

	void RunAllTests( void )
	{
		ui::RequestCloseAllBalloons();						// just in case running tests in quick succession

		ut::RunAllTests();

		ReportTestResults();
	}

	void ReportTestResults( void )
	{
		CWnd* pForegroundWnd = CWnd::GetForegroundWindow();

		if ( nullptr == pForegroundWnd )
			pForegroundWnd = AfxGetMainWnd();

		if ( pForegroundWnd != nullptr && proc::InCurrentThread( pForegroundWnd->GetSafeHwnd() ) )
		{
			static HICON s_hToolIcon = ui::GetImageStoresSvc()->RetrieveIcon( ID_RUN_TESTS )->GetHandle();
			std::vector<std::tstring> testNames;
			ut::CTestSuite::Instance().QueryTestNames( testNames );

			ui::ShowBalloonTip(
				pForegroundWnd,
				str::Format( _T("Completed %d Unit Tests!"), testNames.size() ).c_str(),
				str::Join( testNames, _T("\n") ).c_str(),
				s_hToolIcon
			);
		}

		ui::BeepSignal( MB_ICONWARNING );					// last in chain, signal the end
	}
#endif
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
		static const OsVersion s_osVersion = GetOsVersionImpl();
		return s_osVersion;
	}

	OsVersion GetOsVersionImpl( void )
	{
		/* MSDN Note:
			Applications not manifested for Windows 8.1 or Windows 10 will return the Windows 8 OS version value (6.2).
			Once an application is manifested for a given operating system version, GetVersionEx will always return the
			version that the application is manifested for in future releases.
		*/
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
