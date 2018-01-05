
#include "AboutBox.h"
#include "UtlTests.h"
#include "FileSystemTests.h"
#include "PathGeneratorTests.h"
#include "ResequenceTests.h"
#include "ImageStore.h"
#include "ResourcePool.h"
#include "Logger.h"
#include "ToolStrip.h"
#include "Utilities.h"
#include "VersionInfo.h"
#include "resource.h"
#include <afxcontrolbars.h>			// MFC support for ribbons and control bars


// CBaseApp template code

template< typename BaseClass >
CBaseApp< BaseClass >::CBaseApp( const TCHAR* pAppName /*= NULL*/ )
	: BaseClass( pAppName )
{
#if _MSC_VER >= 1800	// Visual C++ 2013
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;	// support Restart Manager
#endif
}

template< typename BaseClass >
CBaseApp< BaseClass >::~CBaseApp()
{
	ASSERT_NULL( m_pLogger.get() );			// should've been released on ExitInstance
	ASSERT_NULL( m_pImageStore.get() );
}

template< typename BaseClass >
BOOL CBaseApp< BaseClass >::InitInstance( void )
{
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

	SetRegistryKey( _T("Paul Cocoveanu") );			// change the registry key under which our settings are stored

	if ( !m_profileSuffix.empty() )
	{
		std::tstring profileName = m_pszProfileName + m_profileSuffix;

		free( (void*)m_pszProfileName );
		m_pszProfileName = _tcsdup( profileName.c_str() );
	}

	m_pSharedResources.reset( new utl::CResourcePool );
	m_pLogger.reset( new CLogger );
	m_pImageStore.reset( new CImageStore( true ) );
	m_appAccel.Load( IDR_APP_SHARED_ACCEL );

	m_pSharedResources->AddAutoPtr( &m_pLogger );
	m_pSharedResources->AddAutoPtr( &m_pImageStore );

	CToolStrip::RegisterStripButtons( IDR_STD_STRIP );		// register stock images

	// activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );

#ifdef _DEBUG
	// register UTL tests
	CUtlTests::Instance();
	CFileSystemTests::Instance();
	CPathGeneratorTests::Instance();
	CResequenceTests::Instance();
#endif

	return BaseClass::InitInstance();
}

template< typename BaseClass >
int CBaseApp< BaseClass >::ExitInstance( void )
{
	m_pSharedResources.reset();					// release all shared resource
	return BaseClass::ExitInstance();
}

template< typename BaseClass >
BOOL CBaseApp< BaseClass >::PreTranslateMessage( MSG* pMsg )
{
	if ( CWnd* pActiveWnd = dynamic_cast< CWnd* >( CWnd::GetForegroundWindow() ) )
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
void CBaseApp< BaseClass >::OnAppAbout( void )
{
	CAboutBox dialog( CWnd::GetForegroundWindow() );
	dialog.DoModal();
}

template< typename BaseClass >
void CBaseApp< BaseClass >::OnUpdateAppAbout( CCmdUI* pCmdUI )
{
	CWnd* pForegroundWnd = CWnd::GetForegroundWindow();
	pCmdUI->Enable( pForegroundWnd != NULL && !is_a< CAboutBox >( pForegroundWnd ) );
}

template< typename BaseClass >
void CBaseApp< BaseClass >::OnRunUnitTests( void )
{
#ifdef _DEBUG
	ut::CTestSuite::Instance().RunUnitTests();
	ui::BeepSignal( MB_ICONWARNING );					// last in chain, signal the end
#endif
}

template< typename BaseClass >
void CBaseApp< BaseClass >::OnUpdateRunUnitTests( CCmdUI* pCmdUI )
{
#ifdef _DEBUG
	pCmdUI->Enable( !ut::CTestSuite::Instance().IsEmpty() );
#else
	pCmdUI->Enable( FALSE );
#endif
}
