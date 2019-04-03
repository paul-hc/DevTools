#ifndef Application_h
#define Application_h
#pragma once

#include "utl/CommandModel.h"
#include "utl/Logger.h"
#include "utl/UI/BaseApp.h"
#include "Application_fwd.h"


class CAppCmdService;


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );
	virtual ~CApplication();
public:
	svc::ICommandService* GetCommandService( void ) const;
private:
	std::auto_ptr< CAppCmdService > m_pCmdSvc;

	// generated stuff
public:
    virtual BOOL InitInstance( void );
    virtual int ExitInstance( void );
protected:
	DECLARE_MESSAGE_MAP()
};


struct CScopedMainWnd
{
	CScopedMainWnd( HWND hWnd );
	~CScopedMainWnd();

	static bool HasValidParentOwner( void ) { return ::IsWindow( s_pParentOwner->GetSafeHwnd() ) != FALSE; }
	static CWnd* GetParentOwnerWnd( void ) { ASSERT( HasValidParentOwner() ); return s_pParentOwner; }
private:
	CWnd* m_pOldMainWnd;
	static CWnd* s_pParentOwner;
};


extern CComModule g_comModule;	// _Module original name
extern CApplication g_app;


namespace app
{
	inline CApplication& GetApp( void ) { return g_app; }
	inline CWnd* GetMainWnd( void ) { return CScopedMainWnd::GetParentOwnerWnd(); }

	void InitModule( HINSTANCE hInstance );
}


#endif // Application_h
