
#include "utl/FileSystem.h"
#include "utl/Logger.h"
#include "utl/ResourcePool.h"
#include "AboutBox.h"
#include "ImageStore.h"
#include "CmdUpdate.h"
#include "PopupDlgBase.h"
#include "ToolStrip.h"
#include "WndUtils.h"
#include "resource.h"
#include <afxvisualmanageroffice2007.h>		// MFC support for ribbons and control bars

#ifdef USE_UT
#include "utl/test/Test.h"
#include "utl/test/UtlConsoleTests.h"
#include "test/UtlUserInterfaceTests.h"
#endif


// CBaseApp template code

template< typename BaseClass >
CBaseApp<BaseClass>::CBaseApp( void )
	: BaseClass()
	, CAppTools()
	, m_isInteractive( true )
	, m_lazyInitAppResources( false )
	, m_appRegistryKeyName( _T("Paul Cocoveanu") )
{
#if _MSC_VER >= 1800	// Visual C++ 2013
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;	// support Restart Manager
#endif
}

template< typename BaseClass >
CBaseApp<BaseClass>::~CBaseApp()
{
	ASSERT_NULL( m_pLogger.get() );			// should've been released on ExitInstance
	ASSERT_NULL( m_pSharedImageStore.get() );
}

template< typename BaseClass >
void CBaseApp<BaseClass>::SetUseAppLook( app::AppLook appLook )
{
	if ( nullptr == m_pAppLook.get() )			// not yet explicitly set up?
		m_pAppLook.reset( new CAppLook( appLook ) );
	else
		m_pAppLook->SetAppLook( appLook );
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

	if ( m_pAppLook.get() != nullptr )
		m_pAppLook->Save();

	return BaseClass::ExitInstance();
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnInitAppResources( void )
{
	ASSERT_NULL( m_pSharedResources.get() );		// init once

	// init MFC control bars:
	if ( app::InitMfcControlBars( this ) )
		if ( nullptr == m_pAppLook.get() )			// not yet explicitly set up?
			m_pAppLook.reset( new CAppLook( app::Office_2007_Blue ) );

	m_pSharedResources.reset( new utl::CResourcePool() );
	m_pLogger.reset( new CLogger() );
	m_appAccel.Load( IDR_APP_SHARED_ACCEL );

	m_pSharedResources->AddAutoPtr( &m_pLogger );

	// Rely on CLogger::m_addSessionNewLine to add a delayed new line on first log entry
	//GetLogger().LogLine( _T(""), false );					// new-line as session separator

	// register stock images
	GetSharedImageStore()->RegisterToolbarImages( IDR_STD_BUTTONS_STRIP );
	GetSharedImageStore()->RegisterToolbarImages( IDR_LIST_EDITOR_STRIP );

#ifdef USE_UT
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
	return nullptr == m_pMainWnd;
}

template< typename BaseClass >
bool CBaseApp<BaseClass>::IsInteractive( void ) const
{
	return m_isInteractive;
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
CImageStore* CBaseApp<BaseClass>::GetSharedImageStore( void )
{
	if ( nullptr == m_pSharedImageStore.get() )
	{	// lazy creation of the shared image store
		m_pSharedImageStore.reset( new CImageStore() );
		m_pSharedResources->AddAutoPtr( &m_pSharedImageStore );
	}

	return m_pSharedImageStore.get();
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

template< typename BaseClass >
BOOL CBaseApp<BaseClass>::OnIdle( LONG count )
{
	if ( !m_isInteractive )
		m_isInteractive = true;

	if ( count <= 0 )
		CPopupWndPool::Instance()->OnIdle();

	return __super::OnIdle( count );
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
	pCmdUI->Enable( pForegroundWnd != nullptr && !is_a<CAboutBox>( pForegroundWnd ) );
	ui::ExpandVersionInfoTags( pCmdUI );		// expand "[InternalName]"
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnRunUnitTests( void )
{
#ifdef USE_UT
	CWaitCursor wait;
	app::RunAllTests();
#endif
}

template< typename BaseClass >
void CBaseApp<BaseClass>::OnUpdateRunUnitTests( CCmdUI* pCmdUI )
{
#ifdef USE_UT
	pCmdUI->Enable( !ut::CTestSuite::Instance().IsEmpty() );
#else
	pCmdUI->Enable( FALSE );
#endif
}
