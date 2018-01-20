// ExplorerBrowser.h : main header file for the ExplorerBrowser application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "ExplorerBrowser_i.h"


// CExplorerBrowserApp:
// See ExplorerBrowser.cpp for the implementation of this class
//

class CExplorerBrowserApp : public CWinApp
{
public:
	CExplorerBrowserApp() : m_showFrames( true ), m_maximizeFirst( true ) {}
	
	bool m_showFrames;
	bool m_maximizeFirst;

// Overrides
public:
	virtual BOOL InitInstance( void );
	BOOL ExitInstance( void );
protected:
	afx_msg void OnAppAbout( void );
	afx_msg void OnToggleShowFrames( void );
	afx_msg void OnUpdateShowFrames( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};

extern CExplorerBrowserApp g_theApp;
