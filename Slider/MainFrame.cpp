
#include "stdafx.h"
#include "MainFrame.h"
#include "MainToolbar.h"
#include "IImageView.h"
#include "LoggerSetupDialog.h"
#include "Workspace.h"
#include "ChildFrame.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include "utl/UI/resource.h"
#include <afxpriv.h>		// for WM_SETMESSAGESTRING
#include <dde.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static UINT g_sbIndicators[] =
{
	ID_SEPARATOR,					// status line indicator
	IDW_SB_PROGRESS_CAPTION,		// caption of the shared progress bar
	IDW_SB_PROGRESS_BAR				// application shared progress bar
};


IMPLEMENT_DYNAMIC( CMainFrame, CMDIFrameWnd )

CMainFrame::CMainFrame( void )
	: CMDIFrameWnd()
	, m_pToolbar( new CMainToolbar )
	, m_messageClearTimer( this, MessageTimerId, 5000 )
	, m_queueTimer( this, QueueTimerId, 750 )
	, m_progBarResetTimer( this, ProgressResetTimerId, 250 )
{
	// TODO: add member initialization code here
	// Create the combo font as the small caption system font:
}

CMainFrame::~CMainFrame()
{
}

size_t CMainFrame::GetMdiChildCount( void ) const
{
	HWND hMDIChild = ::GetWindow( m_hWndMDIClient, GW_CHILD );

	size_t mdiCount = 0;
	for ( ; hMDIChild != NULL; ++mdiCount )
		hMDIChild = ::GetWindow( hMDIChild, GW_HWNDNEXT );
	return mdiCount;
}

IImageView* CMainFrame::GetActiveImageView( void ) const
{
	if ( CChildFrame* pActiveChild = (CChildFrame*)MDIGetActive() )
		return pActiveChild->GetImageView();
	return NULL;
}

bool CMainFrame::IsFullScreen( void ) const
{
	return CWorkspace::Instance().IsFullScreen() != false;
}

bool CMainFrame::IsMdiRestored( void ) const
{
	BOOL isMaximized;
	return MDIGetActive( &isMaximized ) != NULL && !isMaximized;
}

void CMainFrame::StartQueuedAlbumTimer( UINT timerDelay /*= 750*/ )
{
	m_queueTimer.SetElapsed( timerDelay );
}

bool CMainFrame::CancelStatusBarAutoClear( UINT idleMessageID /*= AFX_IDS_IDLEMESSAGE*/ )
{
	if ( !IsStatusBarAutoClear() )
		return false;
	SetIdleStatusBarMessage( idleMessageID );
	return true;
}

// if elapseMs is UINT_MAX, the message won't be cleared automatically
void CMainFrame::SetStatusBarMessage( const TCHAR* pMessage, UINT elapseMs /*= UINT_MAX*/ )
{
	m_messageClearTimer.Stop();

	static std::tstring message;
	message = pMessage;						// use static storage for the message pointer
	SetMessageText( message.c_str() );

	if ( elapseMs != UINT_MAX )
	{
		m_messageClearTimer.SetElapsed( elapseMs );
		m_messageClearTimer.Start();
	}
}

void CMainFrame::SetIdleStatusBarMessage( UINT idleMessageID /*= AFX_IDS_IDLEMESSAGE*/ )
{
	m_messageClearTimer.Stop();

	// switch the status-bar text to default
	m_nFlags &= ~WF_NOPOPMSG;
	m_nIDTracking = idleMessageID;
	SendMessage( WM_SETMESSAGESTRING, (WPARAM)m_nIDTracking );
	ASSERT( m_nIDTracking == m_nIDLastMessage );

	// update the status-bar right away
	if ( CWnd* pMessageBar = GetMessageBar() )
		pMessageBar->UpdateWindow();
}

void CMainFrame::CleanupWindow( void )
{
	if ( CWorkspace::GetData().m_autoSave )
		CWorkspace::Instance().SaveSettings();
	else
		CWorkspace::Instance().SaveRegSettings();		// registry settings always get saved
}

bool CMainFrame::CreateProgressCtrl( void )
{
	ASSERT_PTR( m_statusBar.m_hWnd );

	int progBarIndex = m_statusBar.CommandToIndex( IDW_SB_PROGRESS_BAR );

	SetProgressCaptionText( _T("") );

	if ( ProgressBarWidth != -1 )
		m_statusBar.SetPaneInfo( progBarIndex, IDW_SB_PROGRESS_BAR, SBPS_NOBORDERS, ProgressBarWidth );
	m_statusBar.SetPaneText( progBarIndex, _T("") );

	if ( !m_progressCtrl.Create( WS_CHILD | PBS_SMOOTH, CRect( 0, 0, 0, 0 ), &m_statusBar, IDW_SB_PROGRESS_BAR ) )
	{
		TRACE0("Failed to create the progress bar\n");
		return false;
	}
	return true;
}

void CMainFrame::SetProgressCaptionText( const TCHAR* pCaption )
{
	int progCaptionIndex = m_statusBar.CommandToIndex( IDW_SB_PROGRESS_CAPTION );

	if ( m_statusBar.GetPaneText( progCaptionIndex ) != pCaption )
	{
		CSize textExtent;
		{
			CClientDC clientDC( &m_statusBar );
			CFont* orgFont = clientDC.SelectObject( m_statusBar.GetFont() );
			textExtent = ui::GetTextSize( &clientDC, pCaption );
			clientDC.SelectObject( orgFont );
		}
		m_statusBar.SetPaneInfo( progCaptionIndex, IDW_SB_PROGRESS_CAPTION, SBPS_NOBORDERS, textExtent.cx /*- 5*/ );
		m_statusBar.SetPaneText( progCaptionIndex, pCaption );
	}
}

bool CMainFrame::DoClearProgressCtrl( void )
{
	m_progBarResetTimer.Stop();

	// IMP: in order to clear the progress-bar, it must be "logically" turned OFF already, otherwise remains ON.
	// That's because delayed timer tick may come after re-activation
	if ( InProgress() )
		return false;

	// hide the progress-bar (if not already)
	if ( HasFlag( m_progressCtrl.GetStyle(), WS_VISIBLE ) )
		m_progressCtrl.ShowWindow( SW_HIDE );

	// clear the progress-bar internal
	m_progressCtrl.SetPos( 0 );
	m_progressCtrl.SetRange32( 0, 1 );
	m_progressCtrl.SetStep( 1 );

	SetProgressCaptionText( _T("") );
	return true;
}

void CMainFrame::BeginProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= NULL*/ )
{
	m_progressCtrl.SetRange32( valueMin, valueMin + count );	// note that valueMax is out of range (100% is valMax - 1)
	m_progressCtrl.SetPos( valueMin );
	m_progressCtrl.SetStep( stepCount );

	// show the progress-bar (if not already)
	ui::ShowWindow( m_progressCtrl );
	SetProgressCaptionText( pCaption );

	m_inProgress.AddInternalChange();
}

void CMainFrame::EndProgress( int clearDelay )
{
	bool wasInProgress = InProgress();

	m_inProgress.ReleaseInternalChange();		// turn off the progress bar (logically)
	if ( wasInProgress )
		if ( clearDelay != app::CScopedProgress::ACD_NoClear )
		{
			m_progBarResetTimer.SetElapsed( clearDelay );
			m_progBarResetTimer.Start();
		}
		else
			DoClearProgressCtrl();
}

void CMainFrame::SetPosProgress( int value )
{
	ASSERT( InProgress() );
	m_progressCtrl.SetPos( value );
}

void CMainFrame::StepItProgress( void )
{
	ASSERT( InProgress() );
	m_progressCtrl.StepIt();
}

bool CMainFrame::ResizeViewToFit( CScrollView* pScrollView )
{
	ASSERT_PTR( pScrollView->GetSafeHwnd() );

	BOOL isMaximized;
	CMDIChildWnd* pActiveMdiChild = MDIGetActive( &isMaximized );

	if ( isMaximized )
	{	// temporary hide MDI client window (in order to avoid flickering) and restore
		::ShowWindow( m_hWndMDIClient, SW_HIDE );
		MDIRestore( pActiveMdiChild );
	}
	pScrollView->ResizeParentToFit( false );

	CFrameWnd* pParentFrame = pScrollView->GetParentFrame();		// may be different than pActiveMdiChild
	CRect rectFrame, rectMdiClient;

	pParentFrame->GetWindowRect( rectFrame );
	::GetWindowRect( m_hWndMDIClient, rectMdiClient );
	if ( ui::EnsureVisibleRect( rectFrame, rectMdiClient ) )
	{
		::MapWindowPoints( HWND_DESKTOP, m_hWndMDIClient, &rectFrame.TopLeft(), 2 );
		pParentFrame->MoveWindow( rectFrame, !isMaximized );		// delay redraw if maximized
	}

	if ( isMaximized )
	{	// switch back to maximized and make visible again the MDI client window
//		MDIMaximize( pActiveMdiChild );
		::ShowWindow( m_hWndMDIClient, SW_SHOW );
	}
	return true;
}

BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& rCS )
{
	if ( !CMDIFrameWnd::PreCreateWindow( rCS ) )
		return FALSE;

	if ( !CWorkspace::Instance().IsLoaded() )			// create and load once
		CWorkspace::Instance().LoadSettings();

	return TRUE;
}

BOOL CMainFrame::OnCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	CPushRoutingFrame push( this );

	return
		CWorkspace::Instance().OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) ||
		m_pToolbar->HandleCmdMsg( cmdId, code, pExtra, pHandlerInfo ) ||
		CMDIFrameWnd::OnCmdMsg( cmdId, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, CMDIFrameWnd )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()
	ON_WM_DROPFILES()
	ON_WM_TIMER()
	ON_COMMAND( CM_ESCAPE_KEY, CmMdiClose )
	ON_COMMAND( CM_MDI_CLOSE_ALL, CmMdiCloseAll )
	ON_UPDATE_COMMAND_UI( CM_MDI_CLOSE_ALL, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_TOGGLE_MAXIMIZE, OnToggleMaximize )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_MAXIMIZE, OnUpdateMaximize )
	ON_COMMAND( CK_FULL_SCREEN, OnToggleFullScreen )
	ON_UPDATE_COMMAND_UI( CK_FULL_SCREEN, OnUpdateFullScreen )
	ON_UPDATE_COMMAND_UI( IDW_ZOOM_COMBO, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_LOGGER_OPTIONS, CmLoggerOptions )
	ON_UPDATE_COMMAND_UI( CM_LOGGER_OPTIONS, OnUpdateAlwaysEnabled )
	ON_COMMAND( CM_REFRESH_CONTENT, CmRefreshContent )
	ON_UPDATE_COMMAND_UI( CM_REFRESH_CONTENT, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_CLEAR_IMAGE_CACHE, CmClearImageCache )
	ON_UPDATE_COMMAND_UI( CM_CLEAR_IMAGE_CACHE, OnUpdateAlwaysEnabled )
	ON_UPDATE_COMMAND_UI( CM_CLEAR_TEMP_EMBEDDED_CLONES, OnUpdateAlwaysEnabled )
END_MESSAGE_MAP()

int CMainFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == CMDIFrameWnd::OnCreate( pCS ) )
		return -1;

	ASSERT_PTR( pCS->hMenu );
	ui::SetMenuImages( *CMenu::FromHandle( pCS->hMenu ) );			// m_hMenuDefault not initialized yet, but will

	DragAcceptFiles();			// enable drag/drop open

	if ( const CWindowPlacement* pLoadedPlacement = CWorkspace::Instance().GetLoadedPlacement() )
		pLoadedPlacement->SetPlacement( this );						// 1st step: restore persistent placement, but with SW_HIDE; 2nd step will use the persisted AfxGetApp()->m_nCmdShow in app InitInstance()

	if ( !m_pToolbar->CreateEx( this, TBSTYLE_FLAT,
								WS_CHILD | WS_VISIBLE |
								CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect( 0, 2, 0, 2 ) ) ||
		 !m_pToolbar->InitToolbar() )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;	  // fail to create
	}

	if ( !m_statusBar.Create( this ) || !m_statusBar.SetIndicators( g_sbIndicators, COUNT_OF( g_sbIndicators ) ) )
	{
		TRACE0("Failed to create status bar\n");
		return -1;	  // fail to create
	}
	CreateProgressCtrl();

	// TODO: delete these three lines if you don't want the toolbar to be dockable
	m_pToolbar->EnableDocking( CBRS_ALIGN_ANY );
	EnableDocking( CBRS_ALIGN_ANY );
	DockControlBar( m_pToolbar.get() );

	PostMessage( WM_COMMAND, CM_LOAD_WORKSPACE_DOCS );			// delayed load the documents saved in workspace
	return 0;
}

void CMainFrame::OnDestroy( void )
{
	CleanupWindow();
	CMDIFrameWnd::OnDestroy();
}

void CMainFrame::OnClose( void )
{
	CWorkspace::Instance().FetchSettings();
	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnGetMinMaxInfo( MINMAXINFO* mmi )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		CMDIFrameWnd::OnGetMinMaxInfo( mmi );
}

void CMainFrame::OnSize( UINT sizeType, int cx, int cy )
{
	CMDIFrameWnd::OnSize( sizeType, cx, cy );

	if ( m_progressCtrl.m_hWnd != NULL )
	{	// move the progress bar on top of the associated statusbar item
		CRect ctrlRect;
		int progBarIndex = m_statusBar.CommandToIndex( IDW_SB_PROGRESS_BAR );

		m_statusBar.GetItemRect( progBarIndex, &ctrlRect );
		m_progressCtrl.MoveWindow( &ctrlRect );
	}
}

void CMainFrame::OnWindowPosChanging( WINDOWPOS* wndPos )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		CMDIFrameWnd::OnWindowPosChanging( wndPos );
}

void CMainFrame::OnShowWindow( BOOL bShow, UINT nStatus )
{
	// Hooked just for debugging...
	CMDIFrameWnd::OnShowWindow( bShow, nStatus );
}

void CMainFrame::OnDropFiles( HDROP hDropInfo )
{
	SetForegroundWindow();
	CMDIFrameWnd::OnDropFiles( hDropInfo );
}

void CMainFrame::OnTimer( UINT_PTR eventId )
{
	if ( m_messageClearTimer.IsHit( eventId ) )
	{
		m_messageClearTimer.Stop();
		SetIdleStatusBarMessage();
	}
	else if ( m_queueTimer.IsHit( eventId ) )
	{
		MSG msg;

		// postpone queue processing if there are pending DDE messages for this window
		if ( !::PeekMessage( &msg, m_hWnd, WM_DDE_FIRST, WM_DDE_LAST, PM_NOREMOVE ) )
		{
			m_queueTimer.Stop();
			app::GetApp()->OpenQueuedAlbum();
		}
	}
	else if ( m_progBarResetTimer.IsHit( eventId ) )
		DoClearProgressCtrl();
	else
		CMDIFrameWnd::OnTimer( eventId );
}

void CMainFrame::CmMdiClose( void )
{
	CFrameWnd* pActiveFrame = MDIGetActive();
	if ( NULL == pActiveFrame )
		pActiveFrame = this;
	pActiveFrame->SendMessage( WM_SYSCOMMAND, SC_CLOSE );
}

void CMainFrame::CmMdiCloseAll( void )
{
	AfxGetApp()->CloseAllDocuments( FALSE );
}

void CMainFrame::OnUpdateAnyMDIChild( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( MDIGetActive() != NULL );
}

void CMainFrame::OnToggleMaximize( void )
{
	BOOL isMaximized;
	if ( CMDIChildWnd* pActiveChild = MDIGetActive( &isMaximized ) )
		if ( isMaximized )
			pActiveChild->MDIRestore();
		else
			pActiveChild->MDIMaximize();
}

void CMainFrame::OnUpdateMaximize( CCmdUI* pCmdUI )
{
	BOOL isMaximized;
	CMDIChildWnd* pActiveChild = MDIGetActive( &isMaximized );

	pCmdUI->Enable( pActiveChild != NULL );
	pCmdUI->SetCheck( isMaximized );
}

void CMainFrame::OnToggleFullScreen( void )
{
	CWorkspace::Instance().ToggleFullScreen();
}

void CMainFrame::OnUpdateFullScreen( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( CWorkspace::Instance().IsFullScreen() );
}

void CMainFrame::CmLoggerOptions( void )
{
	CLoggerSetupDialog loggerDialog( this );
	loggerDialog.DoModal();
}

void CMainFrame::OnUpdateAlwaysEnabled( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( TRUE );
}

void CMainFrame::CmClearImageCache( void )
{
	app::GetThumbnailer()->Clear();
	CWicImageCache::Instance().Clear();
	app::GetApp()->UpdateAllViews( Hint_ReloadImage );
}

void CMainFrame::CmRefreshContent( void )
{
	app::GetApp()->UpdateAllViews( Hint_ReloadImage );
}
