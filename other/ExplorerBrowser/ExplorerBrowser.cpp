// ExplorerBrowser.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ExplorerBrowser.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "ExplorerBrowserDoc.h"
#include "ExplorerBrowserView.h"
#include <initguid.h>
#include "ExplorerBrowser_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CExplorerBrowserApp

class CExplorerBrowserModule : public CAtlMfcModule
{
public:
	DECLARE_LIBID(LIBID_ExplorerBrowserLib);
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_EXPLORERBROWSER, "{522D862B-C396-4C42-B9D4-1A188C9E9189}");
};

CExplorerBrowserModule g_atlModule;

CExplorerBrowserApp g_theApp;		// the one and only CExplorerBrowserApp object


// CExplorerBrowserApp class

BOOL CExplorerBrowserApp::InitInstance( void )
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey( _T("Local AppWizard-Generated Applications") );
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	AddDocTemplate( new CMultiDocTemplate( IDR_ExplorerBrowserTYPE,
		RUNTIME_CLASS( CExplorerBrowserDoc ),
		RUNTIME_CLASS( CChildFrame ),
		RUNTIME_CLASS( CExplorerBrowserView ) ) );

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine( cmdInfo );
	#if !defined( _WIN32_WCE ) || defined( _CE_DCOM )
	// Register class factories via CoRegisterClassObject().
	if ( FAILED( g_atlModule.RegisterClassObjects( CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE ) ) )
		return FALSE;
	#endif // !defined(_WIN32_WCE) || defined(_CE_DCOM)
	// App was launched with /Embedding or /Automation switch.
	// Run app as automation server.
	if ( cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated )
		return TRUE;		// don't show the main window

	if ( cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister )		// app was launched with /Unregserver or /Unregister switch
	{
		g_atlModule.UpdateRegistryAppId( FALSE );
		g_atlModule.UnregisterServer( TRUE );
		return FALSE;
	}

	if ( cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister )			// app was launched with /Register or /Regserver switch
	{
		g_atlModule.UpdateRegistryAppId(TRUE);
		g_atlModule.RegisterServer(TRUE);
		return FALSE;
	}

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if ( !ProcessShellCommand( cmdInfo ) )
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow( m_nCmdShow );
	pMainFrame->UpdateWindow();

	m_maximizeFirst = false;			// from this point on use default behaviour
	return TRUE;
}

BOOL CExplorerBrowserApp::ExitInstance( void )
{
#if !defined(_WIN32_WCE) || defined(_CE_DCOM)
	g_atlModule.RevokeClassObjects();
#endif
	return CWinApp::ExitInstance();
}


// command handlers

BEGIN_MESSAGE_MAP( CExplorerBrowserApp, CWinApp )
	ON_COMMAND( ID_FILE_NEW, OnFileNew )
	ON_COMMAND( ID_FILE_PRINT_SETUP, OnFilePrintSetup )
	ON_COMMAND( ID_VIEW_EXPLORER_SHOW_FRAME, OnToggleShowFrames )
	ON_UPDATE_COMMAND_UI( ID_VIEW_EXPLORER_SHOW_FRAME, OnUpdateShowFrames )
	ON_COMMAND( ID_FILE_NEW, OnFileNew )
	ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
END_MESSAGE_MAP()

void CExplorerBrowserApp::OnToggleShowFrames( void )
{
	m_showFrames = !m_showFrames;
}

void CExplorerBrowserApp::OnUpdateShowFrames( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_showFrames );
}

void CExplorerBrowserApp::OnAppAbout( void )
{
	CDialog dlg( IDD_ABOUTBOX );
	dlg.DoModal();
}
