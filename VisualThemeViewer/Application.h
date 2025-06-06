#pragma once

#include "utl/UI/BaseApp.h"
#include <afxwinappex.h>


class CScopedGdiPlusInit;


namespace app
{
	enum Popup { ListTreePopup };

	typedef CBaseApp<CWinAppEx> TBaseApp;	// originally <CWinApp>, use CWinAppEx for enabling CToolbarImagesDialog image inspection
}


class CApplication : public app::TBaseApp
{
public:
	CApplication();
private:
	std::auto_ptr<CScopedGdiPlusInit> m_pGdiPlusInit;
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );

	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;
