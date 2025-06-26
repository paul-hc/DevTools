
#include "pch.h"
#include "Application.h"
#include "IdeUtilities.h"
#include "IncludeDirectories.h"
#include "ModuleSession.h"
#include "PathSortOrder.h"
#include "resource.h"
#include "utl/AppTools.h"
#include "utl/FileSystem.h"
#include "utl/RegAutomationSvr.h"
#include "utl/UI/ShellDialogs_fwd.h"
#include "utl/UI/VersionInfo.h"
#include "utl/UI/resource.h"

#include "utl/test/LanguageTests.h"
#include "utl/test/StringTests.h"
#include "utl/test/StringCompareTests.h"
#include "test/CppCodeTests.h"
#include "test/MethodPrototypeTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef USE_UT
		// just a few UTL_BASE tests:
		ut::CTestSuite::Instance().ClearTests();		// clear UTL tests, since we want to focus narrowly on this applications' tests

		ut::CTestSuite::Instance().RegisterTestCase( &CStringTests::Instance() );
		ut::CTestSuite::Instance().RegisterTestCase( &CStringCompareTests::Instance() );
		ut::CTestSuite::Instance().RegisterTestCase( &CLanguageTests::Instance() );

		CCppCodeTests::Instance();
		CMethodPrototypeTests::Instance();
	#endif
	}
}


// From IDETools.idl:
//	[ uuid(690D31A0-1AC3-11D2-A26A-006097B8DD84), version(1.0) ]
//	library IDETools
const GUID CDECL g_tlid = {0x690D31A0, 0x1AC3, 0x11D2, {0xA2, 0x6A, 0x00, 0x60, 0x97, 0xB8, 0xDD, 0x84 } };
const WORD g_wVerMajor = 1;
const WORD g_wVerMinor = 0;


// special entry points required for inproc servers

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllCanUnloadNow();
}

// by exporting DllRegisterServer, you can use regsvr.exe to register COM server
STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( !AfxOleRegisterTypeLib( AfxGetInstanceHandle(), g_tlid ) )
		return SELFREG_E_TYPELIB;

	if ( !COleObjectFactory::UpdateRegistryAll( TRUE ) )
		return E_FAIL;

	// register all automation objects as safe for scripting (in component categories)
	ole::CSafeForScripting::UpdateRegistryAll( ole::Register );
	return S_OK;
}

// by exporting DllUnregisterServer, you can use regsvr.exe to unregister COM server
STDAPI DllUnregisterServer( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( !AfxOleUnregisterTypeLib( g_tlid, g_wVerMajor, g_wVerMinor ) )
		return SELFREG_E_TYPELIB;

	// the call below is just for convenience, MFC doesn't actually implement COM server unregistration
	COleObjectFactory::UpdateRegistryAll( FALSE );

	// unregister all automation objects as safe for scripting (in component categories)
	ole::CSafeForScripting::UpdateRegistryAll( ole::Unregister );
	return S_OK;
}


/* CApplication implementation note:

	If this DLL is dynamically linked against the MFC DLLs, any functions exported from this DLL which call into MFC must have the AFX_MANAGE_STATE macro added at the very beginning of the function.

	For example:

	extern "C" BOOL PASCAL EXPORT ExportedFunction()
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		// normal function body here
	}

	It is very important that this macro appear in each function, prior to any calls into MFC.  This means that it must appear as the first statement within the function,
	even before any object variable declarations as their constructors may generate calls into the MFC DLL.

	Please see MFC Technical Notes 33 and 58 for additional details.
*/

CApplication g_theApp;

CApplication::CApplication( void )
	: CBaseApp<CWinApp>()
	, m_isVisualStudio6( false )
{
	// Extension DLLs: prevent heavy resource initialization when the dll gets registered by regsvr32.exe.
	// Will initialize application resources later, when any automation object based on app::CLazyInitAppResources gets instantiated.
	SetLazyInitAppResources();

	// use AFX_IDS_APP_TITLE - same app registry key for 32/64 bit executables

	// Prevent deadlocks on GDI+ initialization caused by usage of ATL::CImage class.
	// This is because GDI+ is not thread-safe, and must only be initialized by the main thread of the Visual C++ IDE host.
	CImageStore::SetSkipMfcToolBarImages();
}

CApplication::~CApplication()
{
	ASSERT_NULL( m_pModuleSession.get() );
}

BOOL CApplication::InitInstance( void )
{	// called by DllMain()
	// modify profile name from "IDETools" to "IDETools_v7"
	StoreProfileSuffix( str::Format( _T("_v%d"), HIWORD( CVersionInfo().GetFileInfo().dwProductVersionMS ) ) );

	if ( !__super::InitInstance() )
		return FALSE;

	// register all OLE server (factories) as running; this enables the OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();
	return TRUE;
}

int CApplication::ExitInstance( void )
{
	if ( CIncludeDirectories::Created() )			// was it loaded?
		CIncludeDirectories::Instance().Save();

	if ( m_pModuleSession.get() != nullptr )
	{
		m_pModuleSession->SaveToRegistry();
		m_pModuleSession.reset();
	}

	return __super::ExitInstance();
}

void CApplication::OnInitAppResources( void )
{
	__super::OnInitAppResources();

	GetSharedImageStore()->RegisterToolbarImages( IDR_IMAGE_STRIP );		// register command images
	CAboutBox::s_appIconId = IDR_IDE_TOOLS_APP;

	fs::CExtCustomOrder::Instance().RegisterCustomOrder();

	StoreVisualStudioVersion();

	m_pModuleSession.reset( new CModuleSession() );
	m_pModuleSession->LoadFromRegistry();

#ifdef USE_UT
	ut::RegisterAppUnitTests();
#endif
}

BOOL CApplication::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pModuleSession.get() != nullptr && m_pModuleSession->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CApplication::StoreVisualStudioVersion( void )
{
	static const fs::CPath s_filenameVS6( _T("MSDEV.EXE") );
	fs::CPath exePath = fs::GetModuleFilePath( nullptr );

	m_isVisualStudio6 = s_filenameVS6 == exePath.GetFilename();

	if ( m_isVisualStudio6 )
	{
		shell::s_useVistaStyle = false;		// switch to legacy file dialogs to prevent problems with using Vista-style file dialogs in VS6
	}
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp<CWinApp> )
END_MESSAGE_MAP()


// app service

namespace app
{
	CModuleSession& GetModuleSession( void )
	{
		return CApplication::GetApp()->GetModuleSession();
	}

	const code::CFormatterOptions& GetCodeFormatterOptions( void )
	{
		return GetModuleSession().GetCodeFormatterOptions();
	}

	const CIncludePaths* GetIncludePaths( void )
	{
		return CIncludeDirectories::Instance().GetCurrentPaths();
	}

	UINT GetMenuVertSplitCount( void )
	{
		return GetModuleSession().m_menuVertSplitCount;
	}

} //namespace app
