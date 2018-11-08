#pragma once

#include "utl/BaseApp.h"


class CApplication : public CBaseApp< CWinApp >
{
public:
	CApplication( void );

	public:
	virtual BOOL InitInstance( void );
public:
	afx_msg void OnAppAbout( void );

	DECLARE_MESSAGE_MAP()
};
