
#include "stdafx.h"
#include "Application.h"
#include "IncludePaths.h"
#include "ModuleSession.h"
#include "SafeForScripting.h"
#include "resource.h"
#include "utl/VersionInfo.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/BaseApp.hxx"


// CApplication implementation
//	NOTE:
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

CApplication theApp;

CApplication::CApplication( void )
	: CBaseApp< CWinApp >()
{
	// TODO: add construction code here, place all significant initialization in InitInstance
}

CApplication::~CApplication()
{
	ASSERT_NULL( m_pModuleSession.get() );
}

BOOL CApplication::InitInstance( void )
{
	// modify profile name from "IDETools" to "IDETools_v5"
	StoreProfileSuffix( str::Format( _T("_v%d"), HIWORD( CVersionInfo().GetFileInfo().dwProductVersionMS ) ) );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );		// register command images
	CAboutBox::m_appIconId = IDR_IDE_TOOLS_APP;

	m_pModuleSession.reset( new CModuleSession );
	m_pModuleSession->LoadFromRegistry();

	// register all OLE server (factories) as running; this enables the OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();
	return TRUE;
}

int CApplication::ExitInstance( void )
{
	m_pModuleSession->SaveToRegistry();
	m_pModuleSession.reset();

	return CBaseApp< CWinApp >::ExitInstance();
}

BOOL CApplication::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pModuleSession.get() != NULL && m_pModuleSession->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	return CBaseApp< CWinApp >::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// command handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()


// app service

namespace app
{
	CIncludePaths& GetIncludePaths( void )
	{
		static CIncludePaths singleton;
		static bool firstInit = true;
		if ( firstInit )
		{
			singleton.InitFromIde();
			firstInit = false;
		}
		return singleton;
	}

	bool IsDebugBreakEnabled( void )
	{
		return CModuleSession::IsDebugBreakEnabled();
	}

	void TraceMenu( HMENU hMenu, size_t indentLevel /*= 0*/ )
	{
		if ( !::IsMenu( hMenu ) )
		{
			TRACE( _T("Invalid menu handle: 0x%08X\n"), hMenu );
			return;
		}

		static const TCHAR indentSegment[] = _T("   ");
		std::tstring indentPreffix( 3 * indentLevel, _T(' ') );

		int itemCount = GetMenuItemCount( hMenu );

		for ( int i = 0; i < itemCount; ++i )
		{
			int itemId = GetMenuItemID( hMenu, i );

			if ( 0 == itemId )
				TRACE( _T("%s----------\n"), indentPreffix.c_str() );
			else
			{
				TCHAR text[ 128 ];
				if ( 0 == GetMenuString( hMenu, i, text, COUNT_OF( text ), MF_BYPOSITION ) )
					TRACE( _T("%s**********\n"), indentPreffix.c_str() );
				else
					TRACE( _T("%s%s\n"), indentPreffix.c_str(), text );

				if ( -1 == itemId )
					TraceMenu( GetSubMenu( hMenu, i ), indentLevel + 1 );
			}
		}
	}

} //namespace app


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

	if ( !COleObjectFactory::UpdateRegistryAll( TRUE ) )
		return E_FAIL;

	// register automation objects in component categories in order to make them safe for scripting
	scripting::registerAllScriptObjects( scripting::Register );
	return S_OK;
}

// by exporting DllUnregisterServer, you can use regsvr.exe to unregister COM server
STDAPI DllUnregisterServer( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// the call below is just for convenience, MFC doesn't actually implement COM server unregistration
	COleObjectFactory::UpdateRegistryAll( FALSE );

	// we have to do it explicitly
	scripting::registerAllScriptObjects( scripting::Unregister );
	return S_OK;
}
