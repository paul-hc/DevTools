#ifndef Application_h
#define Application_h
#pragma once

#include "utl/BaseApp.h"
#include "utl/Logger.h"


class CApplication : public CBaseApp< CWinApp >
{
	CApplication( void );
public:
	static CApplication m_theApp;
public:
    virtual BOOL InitInstance( void );
    virtual int ExitInstance( void );
protected:
	DECLARE_MESSAGE_MAP()
};


namespace app
{
	inline CLogger& GetLogger( void ) { return CApplication::m_theApp.GetLogger(); }
}


#endif // Application_h
