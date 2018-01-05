
#include "stdafx.h"
#include "Application.h"
#include "MainDialog.h"
#include "resource.h"
#include "utl/GpUtilities.h"
#include "utl/utl.h"		// link to UTL

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/BaseApp.hxx"


CApplication theApp;		// the one and only CApplication object


CApplication::CApplication()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

BOOL CApplication::InitInstance( void )
{
	m_pGdiPlusInit.reset( new CScopedGdiPlusInit );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	// create the shell manager, in case the dialog contains any shell tree view or shell list view controls
	std::auto_ptr< CShellManager > pShellManager( new CShellManager );

	// activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );

	CMainDialog dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return FALSE;
}

int CApplication::ExitInstance( void )
{
	m_pGdiPlusInit.reset();
	return CBaseApp< CWinApp >::ExitInstance();
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
