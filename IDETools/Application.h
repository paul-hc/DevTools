#ifndef Application_h
#define Application_h
#pragma once

#include "utl/BaseApp.h"
#include "utl/Logger.h"
#include "Application_fwd.h"


class CModuleSession;


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );
	virtual ~CApplication();

	CModuleSession& GetModuleSession( void ) { ASSERT_PTR( m_pModuleSession.get() ); return *m_pModuleSession; }

	static CApplication* GetApp( void ) { return checked_static_cast< CApplication* >( AfxGetApp() ); }
private:
	std::auto_ptr< CModuleSession > m_pModuleSession;
public:
	// generated overrides
	public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
private:
	DECLARE_MESSAGE_MAP()
};


class CIncludePaths;


namespace app
{
	inline CLogger& GetLogger( void ) { return CApplication::GetApp()->GetLogger(); }
	inline CModuleSession& GetModuleSession( void ) { return CApplication::GetApp()->GetModuleSession(); }

	CIncludePaths& GetIncludePaths( void );

	bool IsDebugBreakEnabled( void );

	void TraceMenu( HMENU hMenu, size_t indentLevel = 0 );
}


#ifdef _DEBUG
	#define DEBUG_LOG app::GetLogger().Log
#else
	#define DEBUG_LOG __noop
#endif


#define CHECK_DEBUG_BREAK	ASSERT( !app::IsDebugBreakEnabled() )


#endif // Application_h
