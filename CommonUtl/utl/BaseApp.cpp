
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
	CLogger* GetLoggerPtr( void )
	{
		static CLogger* s_pLogger = NULL;
		static bool firstInit = true;
		if ( firstInit )
		{
			if ( IGlobalResources* pGlobalRes = GetGlobalResources() )
				s_pLogger = &pGlobalRes->GetLogger();

			firstInit = false;
		}
		return s_pLogger;
	}

	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos )
	{
	#ifdef _DEBUG
		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, ui::DebugPopup );
		ui::TrackPopupMenu( contextMenu, pTargetWnd, screenPos, TPM_RIGHTBUTTON );
	#else
		pTargetWnd;
		screenPos;
	#endif
	}

	void TraceException( const std::exception& exc )
	{
		exc;
		TRACE( _T("* STL exception: %s\n"), CRuntimeException::MessageOf( exc ).c_str() );
	}

	void TraceException( const CException& exc )
	{
		exc;
		TRACE( _T("* MFC exception: %s\n"), mfc::CRuntimeException::MessageOf( exc ).c_str() );
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
		static const CEnumTags osTags( _T("n/a|Windows 2000|Windows XP|Windows Vista|Windows 7|Windows 8|Windows 10|WinBeyond++") );
		return osTags;
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
