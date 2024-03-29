#ifndef Application_h
#define Application_h
#pragma once

#include "utl/Logger.h"
#include "utl/Path.h"
#include "utl/UI/BaseApp.h"
#include "Application_fwd.h"


class CApplication : public CBaseApp<CWinApp>
{
public:
	using CBaseApp<CWinApp>::OnRunUnitTests;

	CApplication( void );
	virtual ~CApplication();

	CModuleSession& GetModuleSession( void ) { ASSERT_PTR( m_pModuleSession.get() ); return *m_pModuleSession; }
	bool IsVisualStudio6( void ) const { return m_isVisualStudio6; }

	static CApplication* GetApp( void ) { return checked_static_cast<CApplication*>( AfxGetApp() ); }
private:
	void StoreVisualStudioVersion( void );
private:
	std::auto_ptr<CModuleSession> m_pModuleSession;
	bool m_isVisualStudio6;

	// base overrides
protected:
	virtual void OnInitAppResources( void );

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
private:
	DECLARE_MESSAGE_MAP()
};


#include "utl/PathGroup.h"


namespace reg
{
	inline void LoadPathGroup( fs::CPathGroup& rPathGroup, const TCHAR section[], const TCHAR entry[] )
	{
		rPathGroup.Split( AfxGetApp()->GetProfileString( section, entry, rPathGroup.Join().c_str() ).GetString() );
	}

	inline void SavePathGroup( const fs::CPathGroup& pathGroup, const TCHAR section[], const TCHAR entry[] )
	{
		AfxGetApp()->WriteProfileString( section, entry, pathGroup.Join().c_str() );
	}
}


#endif // Application_h
