
#include "stdafx.h"
#include "Application.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/BaseApp.hxx"


CApplication theApp;

CApplication::CApplication( void )
	: CBaseApp< CWinApp >()
{
	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

BOOL CApplication::InitInstance()
{
	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	pFrame->LoadFrame( IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL );

	pFrame->ShowWindow( SW_SHOW );
	pFrame->UpdateWindow();
	return TRUE;
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
	ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
END_MESSAGE_MAP()

void CApplication::OnAppAbout( void )
{
	CDialog dlg( IDD_ABOUTBOX );
	dlg.DoModal();
}
