
#include "stdafx.h"
#include "Application.h"
#include "ut/PathAlgorithmsTests.h"
#include "ut/RenameFilesTests.h"
#include "ut/CommandModelSerializerTests.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/BaseApp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CComModule g_comModule;
CApplication g_app;


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef _DEBUG
		CPathAlgorithmsTests::Instance();
		CRenameFilesTests::Instance();
		CCommandModelSerializerTests::Instance();
	#endif
	}
}


CApplication::CApplication( void )
{
	// use AFX_IDS_APP_TITLE="FileRenShell" - same app registry key for 32/64 bit executables
	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

BOOL CApplication::InitInstance( void )
{
	app::InitModule( m_hInstance );
	AfxSetResourceHandle( m_hInstance );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	app::GetLogger().m_logFileMaxSize = -1;		// unlimited log size

	CAboutBox::m_appIconId = IDD_RENAME_FILES_DIALOG;				// will use HugeIcon
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );			// register stock images
	CImageStore::SharedStore()->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );

	ut::RegisterAppUnitTests();

	return TRUE;
}

int CApplication::ExitInstance( void )
{
	g_comModule.Term();
	return CBaseApp< CWinApp >::ExitInstance();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
