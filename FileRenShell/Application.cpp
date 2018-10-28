
#include "stdafx.h"
#include "Application.h"
#include "GeneralOptions.h"
#include "ut/TextAlgorithmsTests.h"
#include "ut/RenameFilesTests.h"
#include "ut/CommandModelSerializerTests.h"
#include "utl/EnumTags.h"
#include "utl/BaseApp.hxx"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef _DEBUG
		CTextAlgorithmsTests::Instance();
		CRenameFilesTests::Instance();
		CCommandModelSerializerTests::Instance();
	#endif
	}
}


CComModule g_comModule;
CApplication g_app;


CApplication::CApplication( void )
{
	// use AFX_IDS_APP_TITLE="FileRenShell" - same app registry key for 32/64 bit executables
	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
	// Called once when the user right-clicks on selected files in Explorer for the first time.

	app::InitModule( m_hInstance );
	AfxSetResourceHandle( m_hInstance );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	app::GetLogger().m_logFileMaxSize = -1;						// unlimited log size

	CGeneralOptions::Instance().LoadFromRegistry();

	CAboutBox::m_appIconId = IDD_RENAME_FILES_DIALOG;			// will use HugeIcon_48
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );		// register stock images
	CImageStore::SharedStore()->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );

	ut::RegisterAppUnitTests();

	return TRUE;
}

int CApplication::ExitInstance( void )
{
	CGeneralOptions::Instance().SaveToRegistry();

	g_comModule.Term();
	return CBaseApp< CWinApp >::ExitInstance();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
