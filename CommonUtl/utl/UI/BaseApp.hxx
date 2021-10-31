
#include "utl/FileSystem.h"
#include "utl/Logger.h"
#include "utl/ResourcePool.h"
#include "AboutBox.h"
#include "ImageStore.h"
#include "CmdUpdate.h"
#include "ToolStrip.h"
#include "Utilities.h"
#include "resource.h"
#include <afxcontrolbars.h>			// MFC support for ribbons and control bars

#ifdef _DEBUG
#include "utl/test/Test.h"
#include "utl/test/UtlConsoleTests.h"
#include "test/UtlUserInterfaceTests.h"
#endif


// CBaseApp template code

template< typename BaseClass >
CBaseApp<BaseClass>::CBaseApp( const TCHAR* pAppName /*= NULL*/ )
	: BaseClass( pAppName )
	, CAppTools()
	, m_appRegistryKeyName( _T("Paul Cocoveanu") )
	, m_lazyInitAppResources( false )
{
#if _MSC_VER >= 1800	// Visual C++ 2013
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;	// support Restart Manager
#endif
}

template< typename BaseClass >
CBaseApp<BaseClass>::~CBaseApp()
{
	ASSERT_NULL( m_pLogger.get() );			// should've been released on ExitInstance
	ASSERT_NULL( m_pImageStore.get() );
}

template< typename BaseClass >
BOOL CBaseApp<BaseClass>::InitInstance( void )
{
	app::TraceOsVersion();
	m_modulePath = fs::GetModuleFilePath( m_hInstance );

	// InitCommonControlsEx() is required on Windows XP if an application manifest specifies use of ComCtl32.dll version 6 or later to enable visual styles.
	// Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX info;
	info.dwSize = sizeof( info );
	info.dwICC = ICC_WIN95_CLASSES;					// to include all the common control classes you want to use in your application
	InitCommonControlsEx( &info );

	if ( !AfxOleInit() )							// initialize OLE libraries (except if a MFC DLL, in which case OLE is initialized by the exe app)
	{
		AfxMessageBox( _T("OLE initialization failed.  Make sure that the OLE libraries are the correct version.") );
		return FALSE;
	}
	AfxEnableControlContainer();

	SetRegistryKey( m_appRegistryKeyName.c_str() );			// change the registry key under which our settings are stored

	app::InitUtlBase();

	if ( !m_appNameSuffix.empty() )
		AfxGetAppModuleState()->m_lpszCurrentAppName = AssignStringCopy( m_pszAppName, m_pszAppName + m_appNameSuffix );

	if ( !m_profileSuffix.empty() )
		AssignStringCopy( m_pszProfileName, m_pszProfileName + m_profileSuffix );

	if ( !m_lazyInitAppResources )
		OnInitAppResources();

	return BaseClass::InitInstance();
}

template< typename BaseClass >
int CBaseApp<BaseClass>::ExitInstance( void )
{
	m_pSharedResources.reset();					// release all shared resource
	return BaseClass::ExitInstance();
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnInitAppResources( void )
{
	ASSERT_NULL( m_pSharedResources.get() );		// init once

	m_pSharedResources.reset( new utl::CResourcePool );
	m_pLogger.reset( new CLogger );
	m_pImageStore.reset( new CImageStore( true ) );
	m_appAccel.Load( IDR_APP_SHARED_ACCEL );

	m_pSharedResources->AddAutoPtr( &m_pLogger );
	m_pSharedResources->AddAutoPtr( &m_pImageStore );

	// Rely on CLogger::m_addSessionNewLine to add a delayed new line on first log entry
	//GetLogger().LogLine( _T(""), false );					// new-line as session separator

	CToolStrip::RegisterStripButtons( IDR_LIST_STRIP );		// register stock images
	CToolStrip::RegisterStripButtons( IDR_STD_STRIP );		// register stock images

	// activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );

#ifdef _DEBUG
	ut::RegisterUtlConsoleTests();
	ut::RegisterUtlUserInterfaceTests();
#endif
}

template< typename BaseClass >
bool CBaseApp<BaseClass>::LazyInitAppResources( void )
{
	if ( m_lazyInitAppResources )
	{
		ASSERT_NULL( m_pSharedResources.get() );
		TRACE( _T("\n *** Lazy initialize application resources of %s ***\n"), m_pszAppName );
		OnInitAppResources();

		if ( m_pSharedResources.get() )
		{
			m_lazyInitAppResources = false;		// done init
			return true;
		}
	}
	return false;
}

template< typename BaseClass >
inline bool CBaseApp<BaseClass>::IsConsoleApp( void ) const
{
	return NULL == m_pMainWnd;
}

template< typename BaseClass >
inline CLogger& CBaseApp<BaseClass>::GetLogger( void )
{
	return *safe_ptr( m_pLogger.get() );
}

template< typename BaseClass >
inline utl::CResourcePool& CBaseApp<BaseClass>::GetSharedResources( void )
{
	return *safe_ptr( m_pSharedResources.get() );
}

template< typename BaseClass >
bool CBaseApp<BaseClass>::BeepSignal( app::MsgType msgType /*= app::Info*/ )
{
	return ui::BeepSignal( app::ToMsgBoxFlags( msgType ) );
}

template< typename BaseClass >
bool CBaseApp<BaseClass>::ReportError( const std::tstring& message, app::MsgType msgType /*= app::Error*/ )
{
	return ui::ReportError( message, app::ToMsgBoxFlags( msgType ) );
}

template< typename BaseClass >
int CBaseApp<BaseClass>::ReportException( const std::exception& exc )
{
	return ui::ReportException( exc );
}

template< typename BaseClass >
int CBaseApp<BaseClass>::ReportException( const CException* pExc )
{
	return ui::ReportException( pExc );
}

template< typename BaseClass >
BOOL CBaseApp<BaseClass>::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( CWnd* pActiveWnd = CWnd::FromHandlePermanent( ::GetForegroundWindow() ) )		// prevent crash in ShellGoodies due to Explorer.exe being multi-threaded
			if ( m_appAccel.Translate( pMsg, pActiveWnd->GetSafeHwnd() ) )
				return TRUE;

	return BaseClass::PreTranslateMessage( pMsg );
}


// command handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseApp, BaseClass, BaseClass )
	ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
	ON_UPDATE_COMMAND_UI( ID_APP_ABOUT, OnUpdateAppAbout )
	ON_COMMAND( ID_RUN_TESTS, OnRunUnitTests )
	ON_UPDATE_COMMAND_UI( ID_RUN_TESTS, OnUpdateRunUnitTests )
END_MESSAGE_MAP()

template< typename BaseClass >
void CBaseApp<BaseClass>::OnAppAbout( void )
{
	CAboutBox dialog( CWnd::GetForegroundWindow() );
	dialog.DoModal();
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnUpdateAppAbout( CCmdUI* pCmdUI )
{
	CWnd* pForegroundWnd = CWnd::GetForegroundWindow();
	pCmdUI->Enable( pForegroundWnd != NULL && !is_a< CAboutBox >( pForegroundWnd ) );
	ui::ExpandVersionInfoTags( pCmdUI );		// expand "[InternalName]"
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnRunUnitTests( void )
{
#ifdef _DEBUG
	app::RunAllTests();
#endif
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnUpdateRunUnitTests( CCmdUI* pCmdUI )
{
#ifdef _DEBUG
	pCmdUI->Enable( !ut::CTestSuite::Instance().IsEmpty() );
#else
	pCmdUI->Enable( FALSE );
#endif
}
