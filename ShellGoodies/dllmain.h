#ifndef dllmain_h
#define dllmain_h
#pragma once

#include "gen/ShellGoodies_i.h"
#include "resource.h"				// for IDR_SHELL_GOODIES_APP
#include <atlbase.h>


class CShellGoodiesModule : public ATL::CAtlDllModuleT<CShellGoodiesModule>
{
public:
	DECLARE_LIBID(LIBID_ShellGoodiesLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SHELL_GOODIES_APP, "1D4EA4F3-89A1-11D5-A57D-0050BA0E2E4A")		// not sure is really required for a shell-extension DLL
};


extern class CShellGoodiesModule g_atlModule;


#endif // dllmain_h
