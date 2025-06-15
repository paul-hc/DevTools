
#include "pch.h"
#include "Application.h"
#include "Options.h"
#include "ThemeStore.h"
#include "MainDialog.h"
#include "resource.h"
#include "utl/UI/GpUtilities.h"
#include <afxshellmanager.h>
#include <afxvisualmanagerwindows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


CApplication theApp;		// the one and only CApplication object


CApplication::CApplication()
	: app::TBaseApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// use AFX_IDS_APP_TITLE - same app registry key for 32/64 bit executables
}

BOOL CApplication::InitInstance( void )
{
	m_pGdiPlusInit.reset( new CScopedGdiPlusInit() );

	if ( !__super::InitInstance() )
		return FALSE;

	std::auto_ptr<CShellManager> pShellManager;		// no longer needed with inheritance from CWinAppEx

	if ( !is_a<CWinAppEx>( ::AfxGetApp() ) )		// note: CWinAppEx already creates a CShellManager in afxShellManager global variable
		pShellManager.reset( new CShellManager() );	// create the shell manager, in case the dialog contains any shell tree view or shell list view controls

	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );		// activate "Windows Native" visual manager for enabling themes in MFC controls
	GetSharedImageStore()->RegisterToolbarImages( IDR_IMAGE_STRIP );		// register stock images
	GetSharedImageStore()->RegisterLoadIconGroup( IDI_MULTI_FRAME_TEST_ICON );

	COptions options;
	CThemeStore themeStore;
	themeStore.SetupNotImplementedThemes();			// mark not implemented themes as NotImplemented

	CMainDialog mainDlg( &options, &themeStore );
	m_pMainWnd = &mainDlg;
	mainDlg.DoModal();

	pShellManager.reset();

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	::ControlBarCleanUp();
#endif

	return FALSE;		// we exit the application, rather than start the application's message pump
}

int CApplication::ExitInstance( void )
{
	m_pGdiPlusInit.reset();
	return __super::ExitInstance();
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, app::TBaseApp )
END_MESSAGE_MAP()
