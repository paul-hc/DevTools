
#include "stdafx.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/BaseApp.hxx"


void InitModule( HINSTANCE hInstance );

CApplication CApplication::m_theApp;

CApplication::CApplication( void )
{
}

BOOL CApplication::InitInstance( void )
{
	InitModule( m_hInstance );
	AfxSetResourceHandle( m_hInstance );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CAboutBox::m_appIconId = IDD_RENAME_FILES_DIALOG;				// will use HugeIcon
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );			// register stock images
	CImageStore::SharedStore()->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );

	return TRUE;
}

int CApplication::ExitInstance( void )
{
	_Module.Term();
	return CBaseApp< CWinApp >::ExitInstance();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
