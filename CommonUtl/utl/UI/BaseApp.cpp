
#include "pch.h"
#include "BaseApp.h"
#include "ContextMenuMgr.h"
#include "EnumTags.h"
#include "MenuUtilities.h"
#include "RuntimeException.h"
#include "ShellTypes.h"
#include "ProcessUtils.h"
#include "WindowDebug.h"
#include "resource.h"
#include "utl/FileEnumerator.h"

#include <afxwinappex.h>
#include <afxtoolbar.h>			// for CMFCToolBar::AddToolBarForImageCollection()
#include <afxtooltipmanager.h>

// for CAppLook class
#include <afxvisualmanager.h>
#include <afxvisualmanagerofficexp.h>
#include <afxvisualmanagerwindows.h>
#include <afxvisualmanageroffice2003.h>
#include <afxvisualmanagervs2005.h>
#include <afxvisualmanageroffice2007.h>
#include <afxdockingmanager.h>

#if _MFC_VER > 0x0900		// MFC version 9.00 or less
	#include <afxvisualmanagerwindows7.h>
	#include <afxvisualmanagervs2008.h>
#endif

#ifdef _DEBUG
#include "utl/test/Test.h"
#include "ImageStore.h"
#include "WndUtils.h"

#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_Settings[] = _T("Settings");
	static const TCHAR entry_AppLook[] = _T("AppLook");
}


namespace nosy
{
	struct CWinAppEx_ : public CWinAppEx
	{
		// public access
		using CWinAppEx::m_bContextMenuManagerAutocreated;
	};
}


// CAppLook implementation

CAppLook::CAppLook( app::AppLook appLook )
	: CCmdTarget()
	, m_appLook( static_cast<app::AppLook>( AfxGetApp()->GetProfileInt( reg::section_Settings, reg::entry_AppLook, appLook ) ) )
{
	SetAppLook( m_appLook );
}

CAppLook::~CAppLook()
{
}

void CAppLook::Save( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_Settings, reg::entry_AppLook, m_appLook );
}

void CAppLook::SetAppLook( app::AppLook appLook )
{
	m_appLook = appLook;

	switch ( m_appLook )
	{
		case app::Windows_2000:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManager ) );
			break;
		case app::Office_XP:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOfficeXP ) );
			break;
		case app::Windows_XP:
			CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );
			break;
		case app::Office_2003:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOffice2003 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		case app::VS_2005:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerVS2005 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		case app::VS_2008:
		#if _MFC_VER > 0x0900		// MFC version 9.00 or less
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerVS2008 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		#else
			SetAppLook( app::VS_2005 );
			return;
		#endif
		case app::Windows_7:
		#if _MFC_VER > 0x0900		// MFC version 9.00 or less
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows7 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		#else
			SetAppLook( app::Windows_XP );
			return;
		#endif
		default:
			switch ( m_appLook )
			{
				case app::Office_2007_Blue:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_LunaBlue );
					break;
				case app::Office_2007_Black:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_ObsidianBlack );
					break;
				case app::Office_2007_Silver:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_Silver );
					break;
				case app::Office_2007_Aqua:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_Aqua );
					break;
			}

			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOffice2007 ) );
			CDockingManager::SetDockingMode( DT_SMART );
	}

	if ( AfxGetMainWnd()->GetSafeHwnd() )
		AfxGetMainWnd()->RedrawWindow( nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE );
}

app::AppLook CAppLook::FromId( UINT cmdId )
{
	switch ( cmdId )
	{
		case ID_VIEW_APPLOOK_WINDOWS_2000:			return app::Windows_2000;
		case ID_VIEW_APPLOOK_OFFICE_XP:				return app::Office_XP;
		case ID_VIEW_APPLOOK_WINDOWS_XP:			return app::Windows_XP;
		case ID_VIEW_APPLOOK_OFFICE_2003:			return app::Office_2003;
		case ID_VIEW_APPLOOK_VS_2005:				return app::VS_2005;
		case ID_VIEW_APPLOOK_VS_2008:				return app::VS_2008;
		case ID_VIEW_APPLOOK_WINDOWS_7:				return app::Windows_7;
		default: ASSERT( false );
		case ID_VIEW_APPLOOK_OFFICE_2007_BLUE:		return app::Office_2007_Blue;
		case ID_VIEW_APPLOOK_OFFICE_2007_BLACK:		return app::Office_2007_Black;
		case ID_VIEW_APPLOOK_OFFICE_2007_SILVER:	return app::Office_2007_Silver;
		case ID_VIEW_APPLOOK_OFFICE_2007_AQUA:		return app::Office_2007_Aqua;
	}
}


// command handlers

BEGIN_MESSAGE_MAP( CAppLook, CCmdTarget )
	ON_COMMAND_RANGE( ID_VIEW_APPLOOK_WINDOWS_2000, ID_VIEW_APPLOOK_WINDOWS_7, OnApplicationLook )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_APPLOOK_WINDOWS_2000, ID_VIEW_APPLOOK_WINDOWS_7, OnUpdateApplicationLook )
END_MESSAGE_MAP()

void CAppLook::OnApplicationLook( UINT cmdId )
{
	CWaitCursor wait;

	SetAppLook( FromId( cmdId ) );
	Save();
}

void CAppLook::OnUpdateApplicationLook( CCmdUI* pCmdUI )
{
	app::AppLook appLook = FromId( pCmdUI->m_nID );

	pCmdUI->SetRadio( m_appLook == appLook );

#if _MFC_VER <= 0x0900		// MFC version 9.00 or less
	pCmdUI->Enable( m_appLook != app::VS_2008 && m_appLook != app::Windows_7 );
#endif
}


//ATL::CTraceCategory traceThumbs( _T("UTL Thumbnails") );


namespace app
{
	void InitUtlBase( void )
	{
		// inject UTL_UI.lib code into UTL_BASE.lib:
		fs::StoreResolveShortcutProc( &shell::ResolveShortcut );
	}

	bool InitMfcControlBars( CWinApp* pWinApp )
	{
		if ( !is_a<CWinAppEx>( pWinApp ) )
			return false;

		nosy::CWinAppEx_* pWinAppEx = mfc::nosy_cast<nosy::CWinAppEx_>( pWinApp );

		TRACE( _T("* Initializing MFC Ribbon resources for application '%s'\n"), pWinAppEx->m_pszAppName );

		// superseeds CWinAppEx::InitContextMenuManager()
		if ( afxContextMenuManager != NULL )
		{
			ASSERT( false );		// already initialized
			return false;
		}

		afxContextMenuManager = new mfc::CContextMenuMgr();		// replace base singleton CContextMenuManager with ui::CContextMenuMgr, that has custom functionality
		pWinAppEx->m_bContextMenuManagerAutocreated = true;

		pWinAppEx->InitKeyboardManager();
		pWinAppEx->InitTooltipManager();

		CMFCToolTipInfo ttInfo;
		ttInfo.m_bVislManagerTheme = TRUE;
		pWinAppEx->GetTooltipManager()->SetTooltipParams( AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS( CMFCToolTipCtrl ), &ttInfo );	// multi-line nice looking tooltips

		if ( afxContextMenuManager != nullptr )
		{	// feed afxCommandManager [class CCommandManager] with images from the strip
			CMFCToolBar::AddToolBarForImageCollection( IDR_STD_STATUS_STRIP );
			CMFCToolBar::AddToolBarForImageCollection( IDR_STD_BUTTONS_STRIP );
			//CMFCToolBar::AddToolBarForImageCollection( IDR_LIST_EDITOR_STRIP );
		}

		return true;
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
