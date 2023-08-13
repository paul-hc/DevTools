#pragma once

#include "utl/UI/BaseApp.h"
#include <afxwinappex.h>


class CScopedGdiPlusInit;


class CApplication : public CBaseApp<CWinAppEx>
{
public:
	CApplication( void );
	virtual ~CApplication();
private:
	bool HasCommandLineOptions( void );
private:
	std::auto_ptr<CScopedGdiPlusInit> m_pGdiPlusInit;
protected:
	// base overrides
	virtual void OnInitAppResources( void );

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
private:
	DECLARE_MESSAGE_MAP()
};


extern CApplication g_theApp;


class CEnumTags;


enum ResizeStyle { ResizeHV, ResizeH, ResizeV, ResizeMaxLimit };
const CEnumTags& GetTags_ResizeStyle( void );

enum ChangeCase { LowerCase, UpperCase, FnameLowerCase, FnameUpperCase, ExtLowerCase, ExtUpperCase, NoExt };
const CEnumTags& GetTags_ChangeCase( void );


namespace app
{
	enum MenuPopup { ImageDialogPopup, TestColorsPopup };		// IDR_CONTEXT_MENU
}
