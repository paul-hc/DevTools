
#include "pch.h"
#include "MainFrame.h"
#include "DemoSysTray.h"
#include "resource.h"
#include "utl/UI/Image_fwd.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/SystemTray.h"
#include "utl/UI/TrayIcon.h"
#include "utl/UI/resource.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseFrameWnd.hxx"


namespace reg
{
	static const TCHAR section_MainFrame[] = _T("Settings");
}


static UINT s_sbIndicators[] =
{
	ID_SEPARATOR,		   // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


IMPLEMENT_DYNAMIC( CMainFrame, CMDIFrameWnd )

CMainFrame::CMainFrame( void )
	: TBaseFrameWnd()
{
	m_regSection = reg::section_MainFrame;
	ui::LoadPopupMenu( &m_trayPopupMenu, IDR_STD_CONTEXT_MENU, ui::AppSysTray );
	m_trayPopupMenu.SetDefaultItem( ID_APP_MINIMIZE );
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::InitSystemTray( void )
{
	// add the shell tray icon
	m_pSystemTray.reset( new CSystemTrayWndHook() );
	m_pSystemTray->SetOwnerCallback( this );

	CTrayIcon* pTrayIcon = m_pSystemTray->CreateTrayIcon( IDR_MESSAGE_TRAY_ICON, false );
	res::LoadImageListDIB( pTrayIcon->RefAnimImageList(), IDB_SEARCH_FILES_ANIM_STRIP, color::ToolStripPink );

	pTrayIcon = m_pSystemTray->CreateTrayIcon( IDR_MAINFRAME );
	res::LoadImageListDIB( pTrayIcon->RefAnimImageList(), IDR_FLAG_STRIP_PNG );

	m_pDemoSysTray.reset( new CDemoSysTray( this ) );
}

BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if ( !__super::PreCreateWindow( cs ) )
		return FALSE;

	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	return TRUE;
}

BOOL CMainFrame::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override
{
	if ( m_pDemoSysTray.get() != nullptr )
		if ( m_pDemoSysTray->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, TBaseFrameWnd )
	ON_WM_CREATE()
	ON_MESSAGE( WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI )
END_MESSAGE_MAP()

int CMainFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	if ( -1 == __super::OnCreate( lpCreateStruct ) )
		return -1;

	if ( !m_wndToolBar.CreateEx( this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC ) ||
		 !m_wndToolBar.LoadToolStrip( IDR_MAINFRAME ) )
	{
		TRACE( "Failed to create toolbar\n" );
		return -1;	  // fail to create
	}

	if ( !m_wndStatusBar.Create( this ) ||
		 !m_wndStatusBar.SetIndicators( ARRAY_SPAN( s_sbIndicators ) ) )
	{
		TRACE( "Failed to create status bar\n" );
		return -1;	  // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking( CBRS_ALIGN_ANY );
	EnableDocking( CBRS_ALIGN_ANY );
	DockControlBar( &m_wndToolBar );

	InitSystemTray();
	return 0;
}

LRESULT CMainFrame::OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam )
{
	wParam, lParam;
	__super::OnIdleUpdateCmdUI( /*wParam, lParam*/ );
	return 0L;
}
