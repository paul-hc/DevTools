// FileRenShell.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for FileRenShell.idl by adding the following
//      files to the Outputs.
//          FileRenShell_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f FileRenShellps.mk in the project directory.

#include "stdafx.h"
#include "FileRenShell.h"
#include "Application.h"
#include <initguid.h>

#include "FileRenShell_i.c"
#include "FileRenameShell.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif


BEGIN_OBJECT_MAP( s_objectMap )
	OBJECT_ENTRY( CLSID_FileRenameShell, CFileRenameShell )
END_OBJECT_MAP()


namespace app
{
	void InitModule( HINSTANCE hInstance )
	{
	#ifdef _MERGE_PROXYSTUB
		hProxyDll = hInstance;
	#endif
		g_comModule.Init( s_objectMap, hInstance, &LIBID_FILERENSHELLLib );
	}
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow( void )
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return ( S_OK == AfxDllCanUnloadNow() && 0 == g_comModule.GetLockCount() ) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
#ifdef _MERGE_PROXYSTUB
	if ( S_OK == PrxDllGetClassObject( rclsid, riid, ppv ) )
        return S_OK;
#endif
    return g_comModule.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer( void )
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
	if ( FAILED( hRes ) )
        return hRes;
#endif
	return g_comModule.RegisterServer( TRUE );		// registers object, typelib and all interfaces in typelib
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer( void )
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
	return g_comModule.UnregisterServer( TRUE );
}
