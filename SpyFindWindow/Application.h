#pragma once

#include "utl/UI/BaseApp.h"


namespace opt
{
	enum FrameStyle { Frame, NonClient, EntireWindow };
}


struct COptions
{
	COptions( void );

	void Load( void );
	void Save( void ) const;
public:
	opt::FrameStyle m_frameStyle;
	int m_frameSize;
	bool m_ignoreHidden;
	bool m_ignoreDisabled;
};


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );

	COptions* GetOptions( void ) { return &m_options; }
private:
	COptions m_options;
public:
	virtual BOOL InitInstance( void );

	DECLARE_MESSAGE_MAP()
};

extern CApplication theApp;

namespace app
{
	inline CApplication& GetSvc( void ) { return *static_cast< CApplication* >( AfxGetApp() ); }
	inline COptions* GetOptions( void ) { return GetSvc().GetOptions(); }
}
