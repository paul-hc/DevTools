#ifndef Application_h
#define Application_h
#pragma once

#include "utl/UI/BaseApp.h"


class CApplication : public CBaseApp<CWinApp>
{
public:
	CApplication( void );

	// generated stuff
public:
	virtual BOOL InitInstance( void );
private:
	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;


#endif // Application_h
