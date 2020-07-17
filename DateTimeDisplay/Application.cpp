
#include "stdafx.h"
#include "Application.h"
#include "MainDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


CApplication theApp;		// the one and only CApplication object


CApplication::CApplication( void )
	: CBaseApp< CWinApp >()
{
	// TODO: add construction code here, place all significant initialization in InitInstance
}

BOOL CApplication::InitInstance( void )
{
	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CAboutBox::s_appIconId = IDD_MAIN_DIALOG;

	// create the shell manager, in case the dialog contains any shell tree view or shell list view controls
	std::auto_ptr< CShellManager > pShellManager( new CShellManager );

	CMainDialog dialog;
	m_pMainWnd = &dialog;
	dialog.DoModal();
	return FALSE;			// skip the application's message pump
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
