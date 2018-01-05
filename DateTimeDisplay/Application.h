#ifndef Application_h
#define Application_h
#pragma once

#include "utl/BaseApp.h"


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );

	// generated overrides
	public:
	virtual BOOL InitInstance( void );
private:
	// generated message map

	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;


#endif // Application_h
