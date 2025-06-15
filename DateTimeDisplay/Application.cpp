
#include "pch.h"
#include "Application.h"
#include "MainDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


CApplication theApp;		// the one and only CApplication object


CApplication::CApplication( void )
	: CBaseApp<CWinApp>()
{
	// use AFX_IDS_APP_TITLE - same app registry key for 32/64 bit executables
}

BOOL CApplication::InitInstance( void )
{
	if ( !CBaseApp<CWinApp>::InitInstance() )
		return FALSE;

	CAboutBox::s_appIconId = IDD_MAIN_DIALOG;

	// create the shell manager, in case the dialog contains any shell tree view or shell list view controls
//	std::auto_ptr<CShellManager> pShellManager( new CShellManager() );

	CMainDialog dialog;
	m_pMainWnd = &dialog;
	dialog.DoModal();
	return FALSE;			// skip the application's message pump
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp<CWinApp> )
END_MESSAGE_MAP()
