#ifndef Application_h
#define Application_h
#pragma once

#include "utl/BaseApp.h"
#include "utl/CommandModel.h"
#include "utl/Logger.h"
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


extern CComModule g_comModule;	// _Module original name
extern CApplication g_app;


namespace app
{
	inline CApplication& GetApp( void ) { return g_app; }
	inline CLogger& GetLogger( void ) { return g_app.GetLogger(); }

	void InitModule( HINSTANCE hInstance );
}


struct CScopedMainWnd
{
	CScopedMainWnd( HWND hWnd );
	~CScopedMainWnd();

	bool HasValidParentOwner( void ) const { return ::IsWindow( m_pParentOwner->GetSafeHwnd() ) != FALSE; }
public:
	CWnd* m_pParentOwner;
	CWnd* m_pOldMainWnd;
};


#endif // Application_h
