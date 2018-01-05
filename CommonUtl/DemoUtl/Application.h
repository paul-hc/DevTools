#pragma once

#include "utl/BaseApp.h"


class CScopedGdiPlusInit;


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );
	virtual ~CApplication();
private:
	std::auto_ptr< CScopedGdiPlusInit > m_pGdiPlusInit;
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
private:
	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;


class CEnumTags;


enum ResizeStyle { ResizeHV, ResizeH, ResizeV, ResizeMaxLimit };
const CEnumTags& GetTags_ResizeStyle( void );

enum ChangeCase { LowerCase, UpperCase, FnameLowerCase, FnameUpperCase, ExtLowerCase, ExtUpperCase, NoExt };
const CEnumTags& GetTags_ChangeCase( void );
