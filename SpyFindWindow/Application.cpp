
#include "stdafx.h"
#include "Application.h"
#include "MainDialog.h"
#include "WndFinder.h"
#include "WndHighlighter.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


namespace reg
{
	static const TCHAR section_options[] = _T("");
	static const TCHAR entry_frameStyle[] = _T("FrameStyle");
	static const TCHAR entry_frameSize[] = _T("FrameSize");
	static const TCHAR entry_ignoreHidden[] = _T("IgnoreHidden");
	static const TCHAR entry_ignoreDisabled[] = _T("IgnoreDisabled");

	static const TCHAR entry_cacheDesktopDC[] = _T("CacheDesktopDC");
	static const TCHAR entry_redrawAtEnd[] = _T("RedrawAtEnd");
}


// COptions implementation

COptions::COptions( void )
	: m_frameStyle( opt::Frame )
	, m_frameSize( ::GetSystemMetrics( SM_CXFIXEDFRAME ) )
	, m_ignoreHidden( true )
	, m_ignoreDisabled( false )
{
}

void COptions::Load( void )
{
	CWinApp* pApp = AfxGetApp();

	m_frameStyle = (opt::FrameStyle)pApp->GetProfileInt( reg::section_options, reg::entry_frameStyle, m_frameStyle );
	m_frameSize = pApp->GetProfileInt( reg::section_options, reg::entry_frameSize, m_frameSize );
	m_ignoreHidden = pApp->GetProfileInt( reg::section_options, reg::entry_ignoreHidden, m_ignoreHidden ) != FALSE;
	m_ignoreDisabled = pApp->GetProfileInt( reg::section_options, reg::entry_ignoreDisabled, m_ignoreDisabled ) != FALSE;

	CWndHighlighter::m_cacheDesktopDC = pApp->GetProfileInt( reg::section_options, reg::entry_cacheDesktopDC, CWndHighlighter::m_cacheDesktopDC ) != FALSE;
	CWndHighlighter::m_redrawAtEnd = pApp->GetProfileInt( reg::section_options, reg::entry_redrawAtEnd, CWndHighlighter::m_redrawAtEnd ) != FALSE;
}

void COptions::Save( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section_options, reg::entry_frameStyle, m_frameStyle );
	pApp->WriteProfileInt( reg::section_options, reg::entry_frameSize, m_frameSize );
	pApp->WriteProfileInt( reg::section_options, reg::entry_ignoreHidden, m_ignoreHidden );
	pApp->WriteProfileInt( reg::section_options, reg::entry_ignoreDisabled, m_ignoreDisabled );

	pApp->WriteProfileInt( reg::section_options, reg::entry_cacheDesktopDC, CWndHighlighter::m_cacheDesktopDC );
	pApp->WriteProfileInt( reg::section_options, reg::entry_redrawAtEnd, CWndHighlighter::m_redrawAtEnd );
}



CApplication theApp;		// the one and only CApplication object


// CApplication

CApplication::CApplication( void )
{
}

BOOL CApplication::InitInstance( void )
{
	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CAboutBox::s_appIconId = IDD_MAIN_DIALOG;
	m_options.Load();

	CMainDialog dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	return FALSE;			// skip the application's message pump
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
END_MESSAGE_MAP()
