
#include "pch.h"
#include "MainFrame.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_mainFrame[] = _T("Desktop\\MainWindow");
	static const TCHAR entry_cmdShow[] = _T("CmdShow");
	static const TCHAR entry_top[] = _T("Top");
	static const TCHAR entry_left[] = _T("Left");
	static const TCHAR entry_bottom[] = _T("Bottom");
	static const TCHAR entry_right[] = _T("Right");
}


static UINT indicators[] =
{
	ID_SEPARATOR,		   // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


CMainFrame::CMainFrame( void )
{
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::SaveWindowPlacement( void )
{
	// save main window position and status to the registry
	WINDOWPLACEMENT wp;
	GetWindowPlacement( &wp );

	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt( reg::section_mainFrame, reg::entry_cmdShow, wp.showCmd );
	pApp->WriteProfileInt( reg::section_mainFrame, reg::entry_top, wp.rcNormalPosition.top );
	pApp->WriteProfileInt( reg::section_mainFrame, reg::entry_left, wp.rcNormalPosition.left );
	pApp->WriteProfileInt( reg::section_mainFrame, reg::entry_bottom, wp.rcNormalPosition.bottom );
	pApp->WriteProfileInt( reg::section_mainFrame, reg::entry_right, wp.rcNormalPosition.right );
}

void CMainFrame::LoadWindowPlacement( CREATESTRUCT& rCreateStruct )
{
	CWinApp* pApp = AfxGetApp();

	pApp->m_nCmdShow = pApp->GetProfileInt( reg::section_mainFrame, reg::entry_cmdShow, pApp->m_nCmdShow );

	CRect normalRect( pApp->GetProfileInt( reg::section_mainFrame, reg::entry_left, -1 ),
					  pApp->GetProfileInt( reg::section_mainFrame, reg::entry_top, -1 ),
					  pApp->GetProfileInt( reg::section_mainFrame, reg::entry_right, -1 ),
					  pApp->GetProfileInt( reg::section_mainFrame, reg::entry_bottom, -1 ) );

	if ( normalRect != CRect( -1, -1, -1, -1 ) )		// previously saved position?
	{
		ui::EnsureVisibleDesktopRect( normalRect );

		rCreateStruct.x = normalRect.left;
		rCreateStruct.y = normalRect.top;
		rCreateStruct.cx = normalRect.Width();
		rCreateStruct.cy = normalRect.Height();
	}
}

BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& rCreateStruct )
{
	if ( !CMDIFrameWnd::PreCreateWindow( rCreateStruct ) )
		return FALSE;

	LoadWindowPlacement( rCreateStruct );
	return TRUE;
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, CMDIFrameWnd )
	ON_WM_CREATE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

int CMainFrame::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CMDIFrameWnd::OnCreate( pCreateStruct ) )
		return -1;

	CString suffix;
	suffix.Format( _T(" %d-bit"), sizeof( void* ) * 8 );			// append the 32/64 bit suffix
	SetTitle( GetTitle() + suffix );

	if ( !m_wndToolBar.CreateEx( this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT ) ||
		 !m_wndToolBar.LoadToolBar( IDR_MAINFRAME ) )
	{
		TRACE("Failed to create toolbar\n");
		return -1;	  // fail to create
	}

	if ( !m_wndReBar.Create( this ) ||
		 !m_wndReBar.AddBar( &m_wndToolBar ) )
	{
		TRACE("Failed to create rebar\n");
		return -1;	  // fail to create
	}

	if ( !m_wndStatusBar.Create( this ) ||
		 !m_wndStatusBar.SetIndicators( indicators, sizeof( indicators )/sizeof( UINT ) ) )
	{
		TRACE("Failed to create status bar\n");
		return -1;	  // fail to create
	}

	m_wndToolBar.SetBarStyle( m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY );		// display tool tips
	return 0;
}

void CMainFrame::OnClose( void )
{
	SaveWindowPlacement();

	__super::OnClose();
}
