#ifndef Application_h
#define Application_h
#pragma once

#include "utl/BaseApp.h"
#include "utl/Logger.h"
#include "Application_fwd.h"


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );

    virtual BOOL InitInstance( void );
    virtual int ExitInstance( void );
protected:
	DECLARE_MESSAGE_MAP()
};


extern CComModule g_comModule;	// _Module original name
extern CApplication g_app;


namespace app
{
	inline CLogger& GetLogger( void ) { return g_app.GetLogger(); }

	void InitModule( HINSTANCE hInstance );
}


#endif // Application_h
