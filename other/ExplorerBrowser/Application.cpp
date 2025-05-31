// ExplorerBrowser.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "Application.h"
#include "MainFrame.h"
#include "ChildFrame.h"
#include "BrowserDoc.h"
#include "BrowserView.h"
#include "resource.h"
#include <initguid.h>
#include "gen/ExplorerBrowser_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CApplicationModule : public CAtlMfcModule
{
public:
	DECLARE_LIBID( LIBID_ExplorerBrowserLib );
	DECLARE_REGISTRY_APPID_RESOURCEID( IDR_EXPLORERBROWSER_REGISTRY, "{522D862B-C396-4C42-B9D4-1A188C9E9189}" );
};


CApplicationModule g_atlModule;
CApplication g_theApp;		// the one and only CApplication object


BOOL CApplication::InitInstance( void )
{
	// InitCommonControlsEx() is required on Windows XP+ if an application manifest specifies use of ComCtl32.dll version 6 or later to enable visual styles.
	// Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX initCtrls;
	initCtrls.dwSize = sizeof( initCtrls );
	initCtrls.dwICC = ICC_WIN95_CLASSES;		// set this to include all the common control classes you want to use in your application
	InitCommonControlsEx( &initCtrls );

	__super::InitInstance();

	if ( !AfxOleInit() )		// initialize OLE libraries
	{
		AfxMessageBox( _T("OLE initialization failed.  Make sure that the OLE libraries are the correct version.") );
		return FALSE;
	}
	AfxEnableControlContainer();

	SetRegistryKey( _T("Paul Cocoveanu\\other") );			// change the registry key under which our settings are stored
	LoadStdProfileSettings( 10 );  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	AddDocTemplate( new CMultiDocTemplate( IDR_XBrowserTYPE,
		RUNTIME_CLASS( CBrowserDoc ),
		RUNTIME_CLASS( CChildFrame ),
		RUNTIME_CLASS( CBrowserView ) ) );

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame();
	if ( NULL == pMainFrame || !pMainFrame->LoadFrame( IDR_MAINFRAME ) )
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
	// register class factories via CoRegisterClassObject()
	if ( FAILED( g_atlModule.RegisterClassObjects( CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE ) ) )
		return FALSE;
#endif // !defined(_WIN32_WCE) || defined(_CE_DCOM)

	// App was launched with /Embedding or /Automation switch.
	// Run app as automation server.
	if ( cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated )
		return TRUE;		// don't show the main window

	switch ( cmdInfo.m_nShellCommand )
	{
		case CCommandLineInfo::AppRegister:				// app was launched with /Register or /Regserver switch
			g_atlModule.UpdateRegistryAppId(TRUE);
			g_atlModule.RegisterServer(TRUE);
			return FALSE;
		case CCommandLineInfo::AppUnregister:			// app was launched with /Unregserver or /Unregister switch
			g_atlModule.UpdateRegistryAppId( FALSE );
			g_atlModule.UnregisterServer( TRUE );
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

BOOL CApplication::ExitInstance( void )
{
#if !defined(_WIN32_WCE) || defined(_CE_DCOM)
	g_atlModule.RevokeClassObjects();
#endif
	return __super::ExitInstance();
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CWinApp )
	ON_COMMAND( ID_ESCAPE_EXIT, OnEscapeExit )
	ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
	ON_COMMAND( ID_FILE_NEW, OnFileNew )
	ON_COMMAND( ID_FILE_PRINT_SETUP, OnFilePrintSetup )
	ON_COMMAND( ID_VIEW_EXPLORER_SHOW_FRAME, OnToggleShowFrames )
	ON_UPDATE_COMMAND_UI( ID_VIEW_EXPLORER_SHOW_FRAME, OnUpdateShowFrames )
	ON_COMMAND( ID_FILE_NEW, OnFileNew )
END_MESSAGE_MAP()

void CApplication::OnEscapeExit( void )
{
	if ( HWND hFocusWnd = ::GetFocus() )
	{
		TCHAR className[ 256 ] = _T("");
		::GetClassName( hFocusWnd, className, _countof( className ) );

		if ( 0 == _tcsicmp( className, _T("Edit") ) )
		{
			::SetFocus( ::GetParent( hFocusWnd ) );		// exit inline edit
			return;										// avoid closing the app
		}
	}

	OnAppExit();
}

void CApplication::OnAppAbout( void )
{
	CDialog dlg( IDD_ABOUTBOX );
	dlg.DoModal();
}

void CApplication::OnToggleShowFrames( void )
{
	m_showFrames = !m_showFrames;
}

void CApplication::OnUpdateShowFrames( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_showFrames );
}
