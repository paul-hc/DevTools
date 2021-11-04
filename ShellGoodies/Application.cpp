
#include "stdafx.h"
#include "Application.h"
#include "AppCmdService.h"
#include "GeneralOptions.h"
#include "test/TextAlgorithmsTests.h"
#include "test/DuplicateFilesTests.h"
#include "test/RenameFilesTests.h"
#include "test/CommandModelSerializerTests.h"
#include "utl/EnumTags.h"
#include "utl/UI/BaseApp.hxx"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef USE_UT
		CTextAlgorithmsTests::Instance();
		CRenameFilesTests::Instance();
		CDuplicateFilesTests::Instance();
		CCommandModelSerializerTests::Instance();
	#endif
	}
}


namespace app
{
	svc::ICommandService* GetCmdSvc( void )
	{
		return g_app.GetCommandService();
	}
}


CComModule g_comModule;
CApplication g_app;


CApplication::CApplication( void )
{
	// Extension DLLs: prevent heavy resource initialization when the dll gets registered by regsvr32.exe.
	// Will initialize application resources later, when CShellGoodiesCom COM object gets instantiated.
	SetLazyInitAppResources();

	// use AFX_IDS_APP_TITLE="ShellGoodies" - same app registry key for 32/64 bit executables
	StoreAppNameSuffix( str::Format( _T(" [%d-bit]"), utl::GetPlatformBits() ) );		// identify the primary target platform
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
	// called once when the user right-clicks on selected files in Explorer for the first time.

	app::InitModule( m_hInstance );
	AfxSetResourceHandle( m_hInstance );

	return CBaseApp< CWinApp >::InitInstance();
}

int CApplication::ExitInstance( void )
{
	if ( IsInitAppResources() )
	{
		if ( m_pCmdSvc->IsDirty() )
			m_pCmdSvc->SaveCommandModel();

		m_pCmdSvc.reset();
		CGeneralOptions::Instance().SaveToRegistry();
	}

	g_comModule.Term();
	return CBaseApp< CWinApp >::ExitInstance();
}

void CApplication::OnInitAppResources( void )
{
	CBaseApp< CWinApp >::OnInitAppResources();

	app::GetLogger()->m_logFileMaxSize = -1;					// unlimited log size

	CAboutBox::s_appIconId = IDD_RENAME_FILES_DIALOG;			// will use HugeIcon_48
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );		// register stock images
	CToolStrip::RegisterStripButtons( IDR_TOOL_STRIP );			// register additional tool images
	CImageStore::SharedStore()->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );

	ut::RegisterAppUnitTests();

	CGeneralOptions::Instance().LoadFromRegistry();

	m_pCmdSvc.reset( new CAppCmdService );
	m_pCmdSvc->LoadCommandModel();
}

svc::ICommandService* CApplication::GetCommandService( void ) const
{
	ASSERT_PTR( m_pCmdSvc.get() );
	return m_pCmdSvc.get();
}

const CCommandModel* CApplication::GetCommandModel( void ) const
{
	ASSERT_PTR( m_pCmdSvc.get() );
	return m_pCmdSvc->GetCommandModel();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()


// CScopedMainWnd implementation

CWnd* CScopedMainWnd::s_pParentOwner = NULL;

CScopedMainWnd::CScopedMainWnd( HWND hWnd )
	: m_pOldMainWnd( NULL )
	, m_inEffect( false )
{
	if ( NULL == s_pParentOwner )
	{
		m_inEffect = true;

		CWnd* pMainWnd = AfxGetMainWnd();
		bool fromThisModule = ui::IsPermanentWnd( pMainWnd->GetSafeHwnd() );

		if ( hWnd != NULL && ::IsWindow( hWnd ) )
			if ( fromThisModule )
				s_pParentOwner = CWnd::FromHandlePermanent( ui::GetTopLevelParent( hWnd ) );
			else
				s_pParentOwner = CWnd::FromHandle( hWnd )->GetTopLevelParent();

		if ( ::IsWindow( s_pParentOwner->GetSafeHwnd() ) )
			if ( pMainWnd != NULL )
				if ( NULL == pMainWnd->m_hWnd )							// it happens sometimes, kind of transitory state when invoking from Explorer.exe
					if ( CWinThread* pCurrThread = AfxGetThread() )
					{
						m_pOldMainWnd = pMainWnd;
						pCurrThread->m_pMainWnd = s_pParentOwner;		// temporarily substitute main window
					}
	}

	// note: m_inEffect is false if created in an embedded scope (e.g. with root scope from ExplorerBrowser32.exe)
}

CScopedMainWnd::~CScopedMainWnd()
{
	if ( m_inEffect )
	{
		if ( m_pOldMainWnd != NULL )
			if ( CWinThread* pCurrThread = AfxGetThread() )
				pCurrThread->m_pMainWnd = m_pOldMainWnd;				// restore original main window

		s_pParentOwner = NULL;
	}
}
