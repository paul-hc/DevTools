
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


CMainFrame::CMainFrame( void )
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
