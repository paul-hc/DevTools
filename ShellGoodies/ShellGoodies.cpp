// ShellGoodies.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//	  To merge the proxy/stub code into the object DLL, add the file
//	  dlldatax.c to the project.  Make sure precompiled headers
//	  are turned off for this file, and add _MERGE_PROXYSTUB to the
//	  defines for the project.
//
//	  If you are not running WinNT4.0 or Win95 with DCOM, then you
//	  need to remove the following define from dlldatax.c
//	  #define _WIN32_WINNT 0x0400
//
//	  Further, if you are running MIDL without /Oicf switch, you also
//	  need to remove the following define from dlldatax.c.
//	  #define USE_STUBLESS_PROXY
//
//	  Modify the custom build rule for ShellGoodies.idl by adding the following
//	  files to the Outputs.
//		  ShellGoodies_p.c
//		  dlldata.c
//	  To build a separate proxy/stub DLL,
//	  run nmake -f ShellGoodiesps.mk in the project directory.

#include "stdafx.h"
#include "ShellGoodies.h"
#include "Application.h"
#include "utl/Registry.h"
#include <initguid.h>

#include "ShellGoodies_i.c"
#include "ShellGoodiesCom.h"


BEGIN_OBJECT_MAP( s_objectMap )
	OBJECT_ENTRY( CLSID_ShellGoodiesCom, CShellGoodiesCom )
END_OBJECT_MAP()


namespace app
{
	void InitModule( HINSTANCE hInstance )
	{
		g_comModule.Init( s_objectMap, hInstance, &LIBID_SHELL_GOODIES_Lib );
	}

	bool AddAsApprovedShellExtension( void );
	bool RemoveAsApprovedShellExtension( void );
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return ( S_OK == AfxDllCanUnloadNow() && 0 == g_comModule.GetLockCount() ) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
	return g_comModule.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer( void )
{
	if ( !app::AddAsApprovedShellExtension() )			// add ourselves to the list of approved shell extensions
		return E_ACCESSDENIED;

	// OBSOLETE: the type library is not necessary for a shell extension DLL
	//return g_comModule.RegisterServer( TRUE );		// registers object, typelib and all interfaces in typelib

	return g_comModule.RegisterServer( FALSE );			// registers object, NO TYPELIB (read ProjectNotes.txt)
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer( void )
{
	app::RemoveAsApprovedShellExtension();				// remove ourselves from the list of approved shell extensions; don't bail out on error since we do the normal ATL unregistration stuff too

	// OBSOLETE: the type library is not necessary for a shell extension DLL
	//return g_comModule.UnregisterServer( TRUE );

	return g_comModule.UnregisterServer( FALSE );		// NO TYPELIB (read ProjectNotes.txt)
}


namespace app
{
	// Michael Dunn's article:	https://www.codeproject.com/Articles/441/The-Complete-Idiot-s-Guide-to-Writing-Shell-Extens

	static const TCHAR s_shellExtClassIdName[] = _T("{1D4EA504-89A1-11D5-A57D-0050BA0E2E4A}");
	static const TCHAR regKey_ShellExtensions_Approved[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");

	bool AddAsApprovedShellExtension( void )
	{
		// NT+: add ourselves to the list of approved shell extensions.

		// Note that you should *NEVER* use the overload of CRegKey::SetValue with 4 parameters.
		// It lets you set a value in one call, without having to call CRegKey::Open() first.
		// However, that version of SetValue() has a bug in that it requests KEY_ALL_ACCESS to the key. That will fail if the user is not an administrator.
		// (The code should request KEY_WRITE, which is all that's necessary.)

		reg::CKey key;
		if ( key.Open( HKEY_LOCAL_MACHINE, regKey_ShellExtensions_Approved, KEY_SET_VALUE ) )
			return key.WriteStringValue( _T("SimpleShlExt extension"), s_shellExtClassIdName );

		return false;
	}

	bool RemoveAsApprovedShellExtension( void )
	{
		// NT+: remove ourselves from the list of approved shell extensions.

		reg::CKey key;
		if ( key.Open( HKEY_LOCAL_MACHINE, regKey_ShellExtensions_Approved, KEY_SET_VALUE ) )
			return key.DeleteValue( s_shellExtClassIdName );

		return false;
	}
}
