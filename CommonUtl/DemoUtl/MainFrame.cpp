
#include "stdafx.h"
#include "MainFrame.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static UINT indicators[] =
{
	ID_SEPARATOR,		   // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


IMPLEMENT_DYNAMIC( CMainFrame, CMDIFrameWnd )

CMainFrame::CMainFrame( void )
	: CMDIFrameWnd()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if ( !CMDIFrameWnd::PreCreateWindow( cs ) )
		return FALSE;

	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	return TRUE;
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, CMDIFrameWnd )
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CMainFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	if ( CMDIFrameWnd::OnCreate( lpCreateStruct ) == -1 )
		return -1;

	if ( !m_wndToolBar.CreateEx( this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		 !m_wndToolBar.LoadToolStrip( IDR_MAINFRAME ) )
	{
		TRACE("Failed to create toolbar\n");
		return -1;	  // fail to create
	}
	m_wndToolBar.SetCustomDisabledImageList( gdi::DisabledGrayOut );

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE("Failed to create status bar\n");
		return -1;	  // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}
