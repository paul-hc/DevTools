#pragma once

#include "utl/BaseApp.h"


class CScopedGdiPlusInit;


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication();
private:
	std::auto_ptr< CScopedGdiPlusInit > m_pGdiPlusInit;
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );

	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;
