#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "gen/ExplorerBrowser_i.h"


class CApplication : public CWinApp
{
public:
	CApplication() : m_showFrames( true ), m_maximizeFirst( true ) {}
	
	bool m_showFrames;
	bool m_maximizeFirst;

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	BOOL ExitInstance( void );
protected:
	afx_msg void OnEscapeExit();
	afx_msg void OnAppAbout( void );
	afx_msg void OnToggleShowFrames( void );
	afx_msg void OnUpdateShowFrames( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};

extern CApplication g_theApp;
