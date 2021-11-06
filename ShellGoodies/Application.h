#ifndef Application_h
#define Application_h
#pragma once

#include "utl/CommandModel.h"
#include "utl/Logger.h"
#include "utl/UI/BaseApp.h"
#include "Application_fwd.h"


class CAppCmdService;


class CApplication : public CBaseApp<CWinApp>
{
public:
	CApplication( void );
	virtual ~CApplication();
public:
	svc::ICommandService* GetCommandService( void ) const;
	const CCommandModel* GetCommandModel( void ) const;
private:
	std::auto_ptr<CAppCmdService> m_pCmdSvc;

	// generated stuff
public:
    virtual BOOL InitInstance( void );
    virtual int ExitInstance( void );
	virtual void OnInitAppResources( void );
protected:
	DECLARE_MESSAGE_MAP()
};


struct CScopedMainWnd
{
	CScopedMainWnd( HWND hWnd );
	~CScopedMainWnd();

	bool InEffect( void ) const { return m_inEffect; }

	static bool HasValidParentOwner( void ) { return ::IsWindow( s_pParentOwner->GetSafeHwnd() ) != FALSE; }
	static CWnd* GetParentOwnerWnd( void ) { return s_pParentOwner != NULL ? s_pParentOwner : AfxGetMainWnd(); }		// during DLL registration there is no parent owner
private:
	CWnd* m_pOldMainWnd;
	bool m_inEffect;
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
