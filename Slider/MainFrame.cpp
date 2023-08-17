
#include "pch.h"
#include "MainFrame.h"
#include "MainToolbar.h"
#include "IImageView.h"
#include "LoggerSetupDialog.h"
#include "Workspace.h"
#include "ChildFrame.h"
#include "DocTemplates.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/BaseZoomView.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include "utl/UI/WindowPlacement.h"
#include "utl/UI/resource.h"
#include <afxpriv.h>		// for WM_SETMESSAGESTRING
#include <dde.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseMainFrameWndEx.hxx"


static UINT s_sbIndicators[] =
{
	ID_SEPARATOR,					// status line indicator
	IDW_SB_PROGRESS_CAPTION,		// caption of the shared progress bar
	IDW_SB_PROGRESS_BAR				// application shared progress bar
};


IMPLEMENT_DYNAMIC( CMainFrame, CMDIFrameWndEx )

CMainFrame::CMainFrame( void )
	: TMDIFrameWndEx()
	, m_pToolbar( new CMainToolbar() )
	, m_messageClearTimer( this, MessageTimerId, 5000 )
	, m_ddeEnqueuedTimer( this, QueueTimerId, 750 )
	, m_progBarResetTimer( this, ProgressResetTimerId, 250 )
{
}

CMainFrame::~CMainFrame()
{
}

size_t CMainFrame::GetMdiChildCount( void ) const
{
	HWND hMDIChild = ::GetWindow( m_hWndMDIClient, GW_CHILD );

	size_t mdiCount = 0;
	for ( ; hMDIChild != nullptr; ++mdiCount )
		hMDIChild = ::GetWindow( hMDIChild, GW_HWNDNEXT );
	return mdiCount;
}

IImageView* CMainFrame::GetActiveImageView( void ) const
{
	if ( CChildFrame* pActiveChild = (CChildFrame*)MDIGetActive() )
		return pActiveChild->GetImageView();
	return nullptr;
}

bool CMainFrame::IsMdiRestored( void ) const
{
	BOOL isMaximized;
	return MDIGetActive( &isMaximized ) != nullptr && !isMaximized;
}

void CMainFrame::StartEnqueuedAlbumTimer( UINT timerDelay /*= 750*/ )
{
	m_ddeEnqueuedTimer.SetElapsed( timerDelay );
	m_ddeEnqueuedTimer.Start();
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
	ASSERT_PTR( m_oldStatusBar.m_hWnd );

	int progBarIndex = m_oldStatusBar.CommandToIndex( IDW_SB_PROGRESS_BAR );

	SetProgressCaptionText( _T("") );

	if ( ProgressBarWidth != -1 )
		m_oldStatusBar.SetPaneInfo( progBarIndex, IDW_SB_PROGRESS_BAR, SBPS_NOBORDERS, ProgressBarWidth );

	m_oldStatusBar.SetPaneText( progBarIndex, _T("") );

	if ( !m_progressCtrl.Create( WS_CHILD | PBS_SMOOTH, CRect( 0, 0, 0, 0 ), &m_oldStatusBar, IDW_SB_PROGRESS_BAR ) )
	{
		TRACE( "Failed to create the progress bar\n" );
		return false;
	}
	return true;
}

void CMainFrame::SetProgressCaptionText( const TCHAR* pCaption )
{
	int progCaptionIndex = m_oldStatusBar.CommandToIndex( IDW_SB_PROGRESS_CAPTION );

	if ( m_oldStatusBar.GetPaneText( progCaptionIndex ) != pCaption )
	{
		CSize textExtent;
		{
			CClientDC clientDC( &m_oldStatusBar );
			CFont* orgFont = clientDC.SelectObject( m_oldStatusBar.GetFont() );
			textExtent = ui::GetTextSize( &clientDC, pCaption );
			clientDC.SelectObject( orgFont );
		}
		m_oldStatusBar.SetPaneInfo( progCaptionIndex, IDW_SB_PROGRESS_CAPTION, SBPS_NOBORDERS, textExtent.cx /*- 5*/ );
		m_oldStatusBar.SetPaneText( progCaptionIndex, pCaption );
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

void CMainFrame::BeginProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= nullptr*/ )
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

bool CMainFrame::ResizeViewToFit( CBaseZoomView* pZoomScrollView )
{
	ASSERT_PTR( pZoomScrollView->GetSafeHwnd() );

	BOOL isMaximized;
	CMDIChildWnd* pActiveMdiChild = MDIGetActive( &isMaximized );

	if ( isMaximized )
	{	// temporary hide MDI client window (in order to avoid flickering) and restore
		::ShowWindow( m_hWndMDIClient, SW_HIDE );
		MDIRestore( pActiveMdiChild );
	}
	pZoomScrollView->ResizeParentToFit( false );

	CFrameWnd* pParentFrame = pZoomScrollView->GetParentFrame();		// may be different than pActiveMdiChild
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
		//MDIMaximize( pActiveMdiChild );
		::ShowWindow( m_hWndMDIClient, SW_SHOW );
	}
	return true;
}

BOOL CMainFrame::OnCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	CPushRoutingFrame push( this );

	return
		__super::OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) ||				// first allow the active view/doc to override central command handlers
		CWorkspace::Instance().OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) ||
		m_pToolbar->HandleCmdMsg( cmdId, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, TMDIFrameWndEx )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()
	ON_WM_DROPFILES()
	ON_WM_TIMER()
	ON_COMMAND( ID_CM_ESCAPE_KEY, On_MdiClose )
	ON_COMMAND( ID_CM_MDI_CLOSE_ALL, On_MdiCloseAll )
	ON_UPDATE_COMMAND_UI( ID_CM_MDI_CLOSE_ALL, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_TOGGLE_MAXIMIZE, OnToggleMaximize )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_MAXIMIZE, OnUpdateMaximize )
	ON_UPDATE_COMMAND_UI( IDW_ZOOM_COMBO, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_LOGGER_OPTIONS, CmLoggerOptions )
	ON_UPDATE_COMMAND_UI( CM_LOGGER_OPTIONS, OnUpdateAlwaysEnabled )
	ON_COMMAND( CM_REFRESH_CONTENT, CmRefreshContent )
	ON_UPDATE_COMMAND_UI( CM_REFRESH_CONTENT, OnUpdateAnyMDIChild )
	ON_COMMAND( CM_CLEAR_IMAGE_CACHE, CmClearImageCache )
	ON_UPDATE_COMMAND_UI( CM_CLEAR_IMAGE_CACHE, OnUpdateAlwaysEnabled )
	ON_UPDATE_COMMAND_UI( CM_CLEAR_TEMP_EMBEDDED_CLONES, OnUpdateAlwaysEnabled )
	ON_COMMAND_RANGE( ID_REGISTER_IMAGE_ASSOC, ID_UNREGISTER_IMAGE_ASSOC, On_RegisterImageAssoc )
	ON_UPDATE_COMMAND_UI_RANGE( ID_REGISTER_IMAGE_ASSOC, ID_UNREGISTER_IMAGE_ASSOC, OnUpdate_RegisterImageAssoc )
END_MESSAGE_MAP()

int CMainFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

	if ( !m_menuBar.Create( this ) )
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_menuBar.SetPaneStyle( m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY );

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus( FALSE );

	if ( !m_standardToolBar.CreateEx( this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC ) ||
		 !m_standardToolBar.LoadToolBar( IDR_TOOLBAR_STANDARD ) )
	{
		TRACE( "Failed to create toolbar\n" );
		return -1;      // fail to create
	}

	if ( !m_albumToolBar.Create( this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC | CBRS_GRIPPER | CBRS_BORDER_3D, IDR_TOOLBAR_ALBUM ) ||
		 !m_albumToolBar.LoadToolBar( IDR_TOOLBAR_ALBUM ) )
	{
		TRACE( "Failed to create album toolbar\n" );
		return FALSE;      // fail to create
	}

	m_standardToolBar.SetWindowText( str::Load( IDR_TOOLBAR_STANDARD ).c_str() );
	m_albumToolBar.SetWindowText( str::Load( IDR_TOOLBAR_ALBUM ).c_str() );

	CString customizeLabel;
	VERIFY( customizeLabel.LoadString( ID_VIEW_CUSTOMIZE ) );

	m_standardToolBar.EnableCustomizeButton( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel );
	m_albumToolBar.EnableCustomizeButton( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel );

	// Allow user-defined toolbars operations:
	InitUserToolbars( nullptr, FirstUserToolBarId, LastUserToolBarId );

	if ( !m_statusBar.Create( this ) )
	{
		TRACE( "Failed to create status bar\n" );
		return -1;      // fail to create
	}

	m_statusBar.SetIndicators( ARRAY_SPAN( s_sbIndicators ) );

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_menuBar.EnableDocking( CBRS_ALIGN_ANY );
	m_standardToolBar.EnableDocking( CBRS_ALIGN_ANY );
	m_albumToolBar.EnableDocking( CBRS_ALIGN_ANY );
	EnableDocking( CBRS_ALIGN_ANY );

	DockPane( &m_menuBar );
	DockPane( &m_albumToolBar );
	DockPaneLeftOf( &m_standardToolBar /*left*/, &m_albumToolBar /*right*/ );


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode( DT_SMART );

	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes( CBRS_ALIGN_ANY );

	// set the visual manager and style based on persisted value
	//TODO: OnApplicationLook( theApp.m_nAppLook );

	// Enable enhanced windows management dialog
	EnableWindowsDialog( ID_WINDOW_MANAGER, ID_WINDOW_MANAGER, TRUE );

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel, ID_VIEW_TOOLBAR );

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();




	/*** OLD Toolbars ***/
	ASSERT_PTR( pCS->hMenu );
	ui::SetMenuImages( CMenu::FromHandle( pCS->hMenu ) );			// m_hMenuDefault not initialized yet, but will

	DragAcceptFiles();			// enable drag&drop open

	if ( CWindowPlacement* pLoadedPlacement = CWorkspace::Instance().GetLoadedPlacement() )
		pLoadedPlacement->CommitWnd( this );						// 1st step: restore persistent placement, but with SW_HIDE; 2nd step will use the persisted AfxGetApp()->m_nCmdShow in app InitInstance()

	if ( !m_pToolbar->CreateEx( this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT,
								WS_CHILD | WS_VISIBLE |
								CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
								CRect( 0, 2, 0, 2 ) ) ||
		 !m_pToolbar->InitToolbar() )
	{
		TRACE( "Failed to create toolbar\n" );
		return -1;	  // fail to create
	}

	if ( !m_oldStatusBar.Create( this ) || !m_oldStatusBar.SetIndicators( ARRAY_SPAN( s_sbIndicators ) ) )
	{
		TRACE( "Failed to create status bar\n" );
		return -1;	  // fail to create
	}
	CreateProgressCtrl();

	// TODO: delete these three lines if you don't want the toolbar to be dockable
	m_pToolbar->EnableDocking( CBRS_ALIGN_ANY );
//	EnableDocking( CBRS_ALIGN_ANY );
//	DockControlBar( m_pToolbar.get() );
	return 0;
}

void CMainFrame::OnDestroy( void )
{
	CleanupWindow();
	__super::OnDestroy();
}

void CMainFrame::OnClose( void )
{
	CWorkspace::Instance().FetchSettings();
	__super::OnClose();
}

void CMainFrame::OnGetMinMaxInfo( MINMAXINFO* mmi )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		__super::OnGetMinMaxInfo( mmi );
}

void CMainFrame::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( m_progressCtrl.m_hWnd != nullptr )
	{	// move the progress bar on top of the associated statusbar item
		CRect ctrlRect;
		int progBarIndex = m_oldStatusBar.CommandToIndex( IDW_SB_PROGRESS_BAR );

		m_oldStatusBar.GetItemRect( progBarIndex, &ctrlRect );
		m_progressCtrl.MoveWindow( &ctrlRect );
	}
}

void CMainFrame::OnWindowPosChanging( WINDOWPOS* wndPos )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		__super::OnWindowPosChanging( wndPos );
}

void CMainFrame::OnShowWindow( BOOL bShow, UINT nStatus )
{
	// Hooked just for debugging...
	__super::OnShowWindow( bShow, nStatus );
}

void CMainFrame::OnDropFiles( HDROP hDropInfo )
{
	SetForegroundWindow();
	__super::OnDropFiles( hDropInfo );
}

void CMainFrame::OnTimer( UINT_PTR eventId )
{
	if ( m_messageClearTimer.IsHit( eventId ) )
	{
		m_messageClearTimer.Stop();
		SetIdleStatusBarMessage();
	}
	else if ( m_ddeEnqueuedTimer.IsHit( eventId ) )
	{
		MSG msg;

		// postpone queue processing if there are pending DDE messages for this window
		if ( !::PeekMessage( &msg, m_hWnd, WM_DDE_FIRST, WM_DDE_LAST, PM_NOREMOVE ) )
		{
			m_ddeEnqueuedTimer.Stop();
			app::GetApp()->OpenQueuedAlbum();
		}
	}
	else if ( m_progBarResetTimer.IsHit( eventId ) )
		DoClearProgressCtrl();
	else
		__super::OnTimer( eventId );
}

void CMainFrame::On_MdiClose( void )
{
	CFrameWnd* pActiveFrame = MDIGetActive();
	if ( nullptr == pActiveFrame )
		pActiveFrame = this;
	pActiveFrame->SendMessage( WM_SYSCOMMAND, SC_CLOSE );
}

void CMainFrame::On_MdiCloseAll( void )
{
	AfxGetApp()->CloseAllDocuments( FALSE );
}

void CMainFrame::OnUpdateAnyMDIChild( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( MDIGetActive() != nullptr );
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

	pCmdUI->Enable( pActiveChild != nullptr );
	pCmdUI->SetCheck( isMaximized );
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

void CMainFrame::On_RegisterImageAssoc( UINT cmdId )
{
	bool doRegister = ID_REGISTER_IMAGE_ASSOC == cmdId;

	CAppDocManager::RegisterImageAdditionalShellExt( doRegister );
}

void CMainFrame::OnUpdate_RegisterImageAssoc( CCmdUI* pCmdUI )
{
	bool doRegister = ID_REGISTER_IMAGE_ASSOC == pCmdUI->m_nID;
	bool isRegistered = CAppDocManager::IsAppRegisteredForImageExt();

	pCmdUI->SetCheck( doRegister == isRegistered );
}
