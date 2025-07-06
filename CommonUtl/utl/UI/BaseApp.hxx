
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

template< typename BaseClassT >
CBaseApp<BaseClassT>::CBaseApp( void )
	: BaseClassT()
	, CAppTools()
	, m_appNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) )		// identify the primary target platform
	, m_isInteractive( true )
	, m_lazyInitAppResources( false )
	, m_appRegistryKeyName( _T("Paul Cocoveanu") )
{
#if _MSC_VER >= 1800	// Visual C++ 2013
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;			// support Restart Manager
#endif
}

template< typename BaseClassT >
CBaseApp<BaseClassT>::~CBaseApp()
{
	ASSERT_NULL( m_pLogger.get() );			// should've been released on ExitInstance
	ASSERT_NULL( m_pSharedImageStore.get() );
}

template< typename BaseClassT >
void CBaseApp<BaseClassT>::SetUseAppLook( app::AppLook appLook )
{
	if ( nullptr == m_pAppLook.get() )			// not yet explicitly set up?
		m_pAppLook.reset( new CAppLook( appLook ) );
	else
		m_pAppLook->SetAppLook( appLook );
}

template< typename BaseClassT >
BOOL CBaseApp<BaseClassT>::InitInstance( void )
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

	return __super::InitInstance();
}

template< typename BaseClassT >
int CBaseApp<BaseClassT>::ExitInstance( void )
{
	m_pSharedResources.reset();					// release all shared resource

	if ( m_pAppLook.get() != nullptr )
		m_pAppLook->Save();

	return __super::ExitInstance();
}

template< typename BaseClassT >
void CBaseApp<BaseClassT>::OnInitAppResources( void )
{
	ASSERT_NULL( m_pSharedResources.get() );		// init once

	// init MFC control bars:
	if ( app::InitMfcControlBars( this ) )
	{
		// App Look support requires inclusion of afxribbon.rc (ribbon and control bars) in project's RC file.
		// It's best to call explicitly SetUseAppLook() in concrete application OnInitAppResources(), where visual managers are actually used.
		if ( false )
			if ( nullptr == m_pAppLook.get() )		// not yet explicitly set up?
				SetUseAppLook( app::Office_2007_Blue );
	}

	m_pSharedResources.reset( new utl::CResourcePool() );
	m_pLogger.reset( new CLogger() );
	m_appAccel.Load( IDR_APP_SHARED_ACCEL );

	m_pSharedResources->AddAutoPtr( &m_pLogger );

	// Rely on CLogger::m_addSessionNewLine to add a delayed new line on first log entry
	//GetLogger().LogLine( _T(""), false );					// new-line as session separator

	// register stock images
	if ( CImageStore* pSharedImageStore = GetSharedImageStore() )
	{	// to prevent ID_TRANSPARENT image displaying as a black rectangle, add a pixel to the bottom-right of the image with RGB(12, 12, 12) and ALPHA=3, which is practically invisible.
		pSharedImageStore->RegisterToolbarImages( IDR_STD_STATUS_STRIP, color::Auto, true );		// (!) always need the ID_TRANSPARENT button image (e.g. for CColorPickerButton system-colors table)
		pSharedImageStore->RegisterToolbarImages( IDR_STD_BUTTONS_STRIP );
		pSharedImageStore->RegisterToolbarImages( IDR_LIST_EDITOR_STRIP );
	}

#ifdef USE_UT
	ut::RegisterUtlConsoleTests();
	ut::RegisterUtlUserInterfaceTests();
#endif
}

template< typename BaseClassT >
bool CBaseApp<BaseClassT>::LazyInitAppResources( void )
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

template< typename BaseClassT >
inline bool CBaseApp<BaseClassT>::IsConsoleApp( void ) const
{
	return nullptr == m_pMainWnd;
}

template< typename BaseClassT >
bool CBaseApp<BaseClassT>::IsInteractive( void ) const
{
	return m_isInteractive;
}

template< typename BaseClassT >
inline CLogger& CBaseApp<BaseClassT>::GetLogger( void )
{
	return *safe_ptr( m_pLogger.get() );
}

template< typename BaseClassT >
inline utl::CResourcePool& CBaseApp<BaseClassT>::GetSharedResources( void )
{
	return *safe_ptr( m_pSharedResources.get() );
}

template< typename BaseClassT >
CImageStore* CBaseApp<BaseClassT>::GetSharedImageStore( void )
{
	if ( nullptr == m_pSharedImageStore.get() )
	{	// lazy creation of the shared image store
		m_pSharedImageStore.reset( new CImageStore() );
		m_pSharedResources->AddAutoPtr( &m_pSharedImageStore );
	}

	return m_pSharedImageStore.get();
}

template< typename BaseClassT >
bool CBaseApp<BaseClassT>::BeepSignal( app::MsgType msgType /*= app::Info*/ )
{
	return ui::BeepSignal( app::ToMsgBoxFlags( msgType ) );
}

template< typename BaseClassT >
bool CBaseApp<BaseClassT>::ReportError( const std::tstring& message, app::MsgType msgType /*= app::Error*/ )
{
	return ui::ReportError( message, app::ToMsgBoxFlags( msgType ) );
}

template< typename BaseClassT >
int CBaseApp<BaseClassT>::ReportException( const std::exception& exc )
{
	return ui::ReportException( exc );
}

template< typename BaseClassT >
int CBaseApp<BaseClassT>::ReportException( const CException* pExc )
{
	return ui::ReportException( pExc );
}

template< typename BaseClassT >
BOOL CBaseApp<BaseClassT>::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( CWnd* pActiveWnd = CWnd::FromHandlePermanent( ::GetForegroundWindow() ) )		// prevent crash in ShellGoodies due to Explorer.exe being multi-threaded
			if ( m_appAccel.Translate( pMsg, pActiveWnd->GetSafeHwnd() ) )
				return TRUE;

	return __super::PreTranslateMessage( pMsg );
}

template< typename BaseClassT >
BOOL CBaseApp<BaseClassT>::OnCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pAppLook.get() != nullptr )
		if ( m_pAppLook->OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) )
			return TRUE;

	return __super::OnCmdMsg( cmdId, code, pExtra, pHandlerInfo );
}

template< typename BaseClassT >
BOOL CBaseApp<BaseClassT>::OnIdle( LONG count )
{
	if ( !m_isInteractive )
		m_isInteractive = true;

	if ( count <= 0 )
		CPopupWndPool::Instance()->OnIdle();

	return __super::OnIdle( count );
}


// command handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseApp, BaseClassT, BaseClassT )
	ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
	ON_UPDATE_COMMAND_UI( ID_APP_ABOUT, OnUpdateAppAbout )
	ON_COMMAND( ID_RUN_TESTS, OnRunUnitTests )
	ON_UPDATE_COMMAND_UI( ID_RUN_TESTS, OnUpdateRunUnitTests )
END_MESSAGE_MAP()

template< typename BaseClassT >
void CBaseApp<BaseClassT>::OnAppAbout( void )
{
	CAboutBox dialog( CWnd::GetForegroundWindow() );
	dialog.DoModal();
}

template< typename BaseClassT >
void CBaseApp<BaseClassT>::OnUpdateAppAbout( CCmdUI* pCmdUI )
{
	CWnd* pForegroundWnd = CWnd::GetForegroundWindow();
	pCmdUI->Enable( pForegroundWnd != nullptr && !is_a<CAboutBox>( pForegroundWnd ) );
	ui::ExpandVersionInfoTags( pCmdUI );		// expand "[InternalName]"
}

template< typename BaseClassT >
void CBaseApp<BaseClassT>::OnRunUnitTests( void )
{
#ifdef USE_UT
	CWaitCursor wait;
	app::RunAllTests();
#endif
}

template< typename BaseClassT >
void CBaseApp<BaseClassT>::OnUpdateRunUnitTests( CCmdUI* pCmdUI )
{
#ifdef USE_UT
	pCmdUI->Enable( !ut::CTestSuite::Instance().IsEmpty() );
#else
	pCmdUI->Enable( FALSE );
#endif
}
