// wrapper for dlldata.c
//	Borrowed from ATL App Wizard (Visual C++ 2022).

#ifdef _MERGE_PROXYSTUB			// merge proxy stub DLL (optional)
	#define REGISTER_PROXY_DLL			// DllRegisterServer, etc.
	#define USE_STUBLESS_PROXY			// defined only with MIDL switch /Oicf

	#pragma comment(lib, "rpcns4.lib")
	#pragma comment(lib, "rpcrt4.lib")

	#define ENTRY_PREFIX	Prx

	#include "gen/dlldata.c"
	#include "gen/ShellGoodies_p.c"
#else
	// this is the default case (no _MERGE_PROXYSTUB defined in C/C++ > Preprocessor Definitions)
	#pragma warning( disable: 4206 )	// warning C4206: nonstandard extension used: translation unit is empty
#endif //_MERGE_PROXYSTUB
