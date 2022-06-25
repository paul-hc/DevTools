
#include "stdafx.h"
#include "Application.h"
#include "AppCmdService.h"
#include "GeneralOptions.h"
#include "test/TextAlgorithmsTests.h"
#include "test/RenameFilesTests.h"
#include "utl/EnumTags.h"
#include "utl/UI/SystemTray.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef USE_UT
		CTextAlgorithmsTests::Instance();
		CRenameFilesTests::Instance();
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
	: CBaseApp<CWinApp>()
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

BOOL CApplication::InitInstance( void ) override
{
	// called once when the user right-clicks on selected files in Explorer for the first time.

	app::InitModule( m_hInstance );
	AfxSetResourceHandle( m_hInstance );

	return __super::InitInstance();
}

int CApplication::ExitInstance( void ) override
{
	if ( IsInitAppResources() )
	{
		if ( m_pCmdSvc->IsDirty() )
			m_pCmdSvc->SaveCommandModel();

		m_pCmdSvc.reset();
		m_pSystemTray.reset();

		CGeneralOptions::Instance().SaveToRegistry();
	}

	g_comModule.Term();
	return __super::ExitInstance();
}

void CApplication::OnInitAppResources( void ) override
{
	__super::OnInitAppResources();

	app::GetLogger()->m_logFileMaxSize = 1 * MegaByte;			// was -1: unlimited log size
	CAboutBox::s_appIconId = IDR_SHELL_GOODIES_APP;				// will use HugeIcon_48

	CImageStore* pSharedStore = GetSharedImageStore();

	pSharedStore->RegisterToolbarImages( IDR_IMAGE_STRIP );		// register stock images
	pSharedStore->RegisterToolbarImages( IDR_TOOL_STRIP );		// register additional tool images
	pSharedStore->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );
	pSharedStore->RegisterAlias( IDD_RENAME_FILES_DIALOG, ID_RENAME_ITEM );

	ut::RegisterAppUnitTests();

	CGeneralOptions::Instance().LoadFromRegistry();

	m_pCmdSvc.reset( new CAppCmdService() );
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

CTrayIcon* CApplication::GetMessageTrayIcon( void )
{
	if ( NULL == CSystemTray::Instance() && NULL == m_pSystemTray.get() )		// no system tray created by another UI component and no shared application message tray icon?
	{	// create once the system-tray message icon
		m_pSystemTray.reset( new CSystemTrayWnd() );							// hidden popup tray icon host
		return m_pSystemTray->CreateTrayIcon( IDR_MESSAGE_TRAY_ICON, false );	// auto-hide message tray icon
	}

	return CSystemTray::Instance() != NULL ? CSystemTray::Instance()->FindIcon( IDR_MESSAGE_TRAY_ICON ) : NULL;
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp<CWinApp> )
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
