
#include "stdafx.h"
#include "Application.h"
#include "MainDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


CApplication theApp;

CApplication::CApplication( void )
	: CBaseApp< CWinApp >()
{
	m_appRegistryKeyName += _T("\\other");
	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

BOOL CApplication::InitInstance()
{
	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CMainDialog mainDialog;

	m_pMainWnd = &mainDialog;
	mainDialog.DoModal();
	m_pMainWnd = NULL;

	return FALSE;
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
