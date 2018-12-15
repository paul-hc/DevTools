
#include "stdafx.h"
#include "Application.h"
#include "CommandService.h"
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
	m_pCmdSvc.reset( new CCommandService );
	m_pCmdSvc->LoadCommandModel();

	CAboutBox::m_appIconId = IDD_RENAME_FILES_DIALOG;			// will use HugeIcon_48
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );		// register stock images
	CImageStore::SharedStore()->RegisterAlias( ID_EDIT_CLEAR, ID_REMOVE_ITEM );

	ut::RegisterAppUnitTests();

	return TRUE;
}

int CApplication::ExitInstance( void )
{
	if ( m_pCmdSvc->IsDirty() )
		m_pCmdSvc->SaveCommandModel();

	m_pCmdSvc.reset();
	CGeneralOptions::Instance().SaveToRegistry();

	g_comModule.Term();
	return CBaseApp< CWinApp >::ExitInstance();
}

svc::ICommandService* CApplication::GetCommandService( void ) const
{
	ASSERT_PTR( m_pCmdSvc.get() );
	return m_pCmdSvc.get();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()


// CScopedMainWnd implementation

CScopedMainWnd::CScopedMainWnd( HWND hWnd )
	: m_pParentOwner( NULL )
	, m_pOldMainWnd( NULL )
{
	CWnd* pMainWnd = AfxGetMainWnd();
	bool fromThisModule = ui::IsWndPermanent( pMainWnd->GetSafeHwnd() );

	if ( hWnd != NULL && ::IsWindow( hWnd ) )
		if ( fromThisModule )
			m_pParentOwner = ui::FindTopParentPermanent( hWnd );
		else
			m_pParentOwner = CWnd::FromHandle( hWnd )->GetTopLevelParent();

	if ( ::IsWindow( m_pParentOwner->GetSafeHwnd() ) )
		if ( pMainWnd != NULL )
			if ( NULL == pMainWnd->m_hWnd )							// it happens sometimes, kind of transitory state when invoking from Explorer.exe
				if ( CWinThread* pCurrThread = AfxGetThread() )
				{
					m_pOldMainWnd = pMainWnd;
					pCurrThread->m_pMainWnd = m_pParentOwner;		// temporarily substitute main window
				}
}

CScopedMainWnd::~CScopedMainWnd()
{
	if ( m_pOldMainWnd != NULL )
		if ( CWinThread* pCurrThread = AfxGetThread() )
			pCurrThread->m_pMainWnd = m_pOldMainWnd;				// restore original main window
}
