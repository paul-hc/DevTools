
#include "pch.h"
#include "MainFrame.h"
#include "IImageView.h"
#include "LoggerSetupDialog.h"
#include "Workspace.h"
#include "ChildFrame.h"
#include "DocTemplates.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/BaseZoomView.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/StatusProgressService.h"
#include "utl/UI/ToolbarButtons.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include "utl/UI/WindowPlacement.h"
#include "utl/UI/resource.h"
#include "test/UiTestUtils.h"
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
	IDW_SB_PROGRESS_BAR,			// application shared progress bar
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


IMPLEMENT_DYNAMIC( CMainFrame, CMDIFrameWndEx )

CMainFrame::CMainFrame( void )
	: TMDIFrameWndEx()
	, m_messageClearTimer( this, MessageTimerId, 5000 )
	, m_ddeEnqueuedTimer( this, QueueTimerId, 750 )
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
	if ( !m_messageClearTimer.IsStarted() )
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


// ui::IZoomBar interface

bool CMainFrame::OutputScalingMode( ui::ImageScalingMode scalingMode )
{
	mfc::ForEach_MatchingButton<mfc::CEnumComboBoxButton>( IDW_IMAGE_SCALING_COMBO, func::MakeSetter( &mfc::CEnumComboBoxButton::SetValue, (int)scalingMode ) );
	return true;
}

ui::ImageScalingMode CMainFrame::InputScalingMode( void ) const
{
	const mfc::CEnumComboBoxButton* pScalingCombo = mfc::FindNotifyingMatchingButton<mfc::CEnumComboBoxButton>( IDW_IMAGE_SCALING_COMBO );

	ASSERT_PTR( pScalingCombo );
	return pScalingCombo->GetEnum<ui::ImageScalingMode>();
}

bool CMainFrame::OutputZoomPct( UINT zoomPct )
{
	mfc::ForEach_MatchingButton<mfc::CStockValuesComboBoxButton>( IDW_ZOOM_COMBO, func::MakeSetter( &mfc::CStockValuesComboBoxButton::template OutputValue<UINT>, zoomPct ) );
	return true;
}

UINT CMainFrame::InputZoomPct( ui::ComboField byField ) const
{
	const mfc::CStockValuesComboBoxButton* pZoomCombo = mfc::FindNotifyingMatchingButton<mfc::CStockValuesComboBoxButton>( IDW_ZOOM_COMBO );
	UINT zoomPct;

	ASSERT_PTR( pZoomCombo );
	if ( !pZoomCombo->InputValue( &zoomPct, byField, true ) )
		zoomPct = 0;

	return zoomPct;
}

// INavigationBar interface

bool CMainFrame::OutputNavigRange( UINT imageCount )
{
	enum { ThresholdCount = 30 };

	if ( mfc::CSliderButton* pSliderButton = mfc::FindFirstMatchingButton<mfc::CSliderButton>( IDW_NAVIG_SLIDER_CTRL ) )
		return pSliderButton->SetCountRange( imageCount, ThresholdCount );

	return false;
}

bool CMainFrame::OutputNavigPos( int imagePos )
{
	if ( mfc::CSliderButton* pSliderButton = mfc::FindFirstMatchingButton<mfc::CSliderButton>( IDW_NAVIG_SLIDER_CTRL ) )
	{
		if ( imagePos < 0 || imagePos > pSliderButton->GetRange().m_end )
			imagePos = 0;

		if ( pSliderButton->SetPos( imagePos, false ) )
			return true;
	}

	return false;
}

int CMainFrame::InputNavigPos( void ) const
{
	if ( mfc::CSliderButton* pSliderButton = mfc::FindFirstMatchingButton<mfc::CSliderButton>( IDW_NAVIG_SLIDER_CTRL ) )
		return pSliderButton->GetPos();

	ASSERT( false );		// shouldn't be called
	return 0;
}


void CMainFrame::HandleResetToolbar( UINT toolBarResId )
{	// called after loading each toolbar from resource to initialize custom buttons
	switch ( toolBarResId )
	{
		case IDR_TOOLBAR_STANDARD:
		{	// replace custom buttons:
			mfc::ToolBar_ReplaceButton( &m_standardToolBar, mfc::CEnumComboBoxButton( IDW_IMAGE_SCALING_COMBO, &ui::GetTags_ImageScalingMode(), ScalingModeComboWidth ) );
			mfc::ToolBar_ReplaceButton( &m_standardToolBar, mfc::CStockValuesComboBoxButton( IDW_ZOOM_COMBO, ui::CZoomStockTags::Instance(), ZoomComboWidth ) );
			mfc::ToolBar_SetBtnText( &m_standardToolBar, IDW_SMOOTHING_MODE_CHECK, _T("&Smooth"), true, false );
			break;
		}
		case IDR_TOOLBAR_NAVIGATE:
		{
			mfc::ToolBar_ReplaceButton( &m_navigateToolBar, mfc::CSliderButton( IDW_NAVIG_SLIDER_CTRL, NavigSliderCtrlWidth ) );
			break;
		}
	}
}

BOOL CMainFrame::OnCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	CPushRoutingFrame push( this );

	return
		__super::OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) ||				// first allow the active view/doc to override central command handlers
		CWorkspace::Instance().OnCmdMsg( cmdId, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, TMDIFrameWndEx )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()
	ON_WM_DROPFILES()
	ON_WM_TIMER()
	ON_REGISTERED_MESSAGE( AFX_WM_RESETTOOLBAR, OnResetToolbar )
	ON_COMMAND( ID_CM_ESCAPE_KEY, On_EscapeKey )
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
	ON_UPDATE_COMMAND_UI( IDW_SB_PROGRESS_CAPTION, OnUpdateAlwaysEnabled )
	ON_COMMAND( ID_FOCUS_ON_ZOOM_COMBO, On_FocusOnZoomCombo )
	ON_COMMAND( ID_FOCUS_ON_SLIDER_CTRL, On_FocusOnSliderCtrl )
END_MESSAGE_MAP()

int CMainFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

	if ( !m_menuBar.Create( this ) )
	{
		TRACE( "Failed to create menubar\n" );
		return -1;      // fail to create
	}

	m_menuBar.SetPaneStyle( m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY );

	CMFCPopupMenu::SetForceMenuFocus( FALSE );		// prevent the menu bar from taking the focus on activation

	// create first toolbar - use CreateEx()
	if ( !m_standardToolBar.CreateEx( this, TBSTYLE_FLAT, mfc::FirstToolbarStyle ) ||
		 !m_standardToolBar.LoadToolBar( IDR_TOOLBAR_STANDARD ) )
	{
		TRACE( "Failed to create toolbar\n" );
		return -1;      // fail to create
	}

	// create additional toolbar - use Create(), passing toolbar ID
	if ( !m_navigateToolBar.Create( this, mfc::AdditionalToolbarStyle, IDR_TOOLBAR_NAVIGATE ) ||
		 !m_navigateToolBar.LoadToolBar( IDR_TOOLBAR_NAVIGATE ) )
	{
		TRACE( "Failed to create album toolbar\n" );
		return FALSE;      // fail to create
	}

	m_standardToolBar.SetWindowText( str::Load( IDR_TOOLBAR_STANDARD ).c_str() );
	m_navigateToolBar.SetWindowText( str::Load( IDR_TOOLBAR_NAVIGATE ).c_str() );
	m_navigateToolBar.SetToolBarBtnText( m_navigateToolBar.CommandToIndex( ID_TOGGLE_NAVIG_PLAY ), _T("Play") );

	CString customizeLabel;
	VERIFY( customizeLabel.LoadString( ID_VIEW_CUSTOMIZE ) );

	m_standardToolBar.EnableCustomizeButton( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel );
	m_navigateToolBar.EnableCustomizeButton( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel );

	// Allow user-defined toolbars operations:
	InitUserToolbars( nullptr, FirstUserToolBarId, LastUserToolBarId );

	if ( !m_statusBar.Create( this ) ||
		 !m_statusBar.SetIndicators( ARRAY_SPAN( s_sbIndicators ) ) )
	{
		TRACE( "Failed to create status bar\n" );
		return -1;      // fail to create
	}

	m_statusBar.SetPaneStyle( Status_ProgressLabel, SBPS_NOBORDERS | SBPS_POPOUT );
	m_statusBar.SetPaneStyle( Status_Info, SBPS_STRETCH | SBPS_NOBORDERS );
	m_statusBar.SetPaneWidth( Status_Progress, ProgressBarWidth );
	m_statusBar.EnablePaneDoubleClick();

	CStatusProgressService::InitStatusBarInfo( &m_statusBar, Status_Progress, Status_ProgressLabel );		// initialize once the status progress service

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_menuBar.EnableDocking( CBRS_ALIGN_ANY );
	m_standardToolBar.EnableDocking( CBRS_ALIGN_ANY );
	m_navigateToolBar.EnableDocking( CBRS_ALIGN_ANY );
	EnableDocking( CBRS_ALIGN_ANY );

	DockPane( &m_menuBar );
	mfc::DockPanesOnRow( GetDockingManager(), 2, &m_standardToolBar, &m_navigateToolBar );


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode( DT_SMART );

	EnableAutoHidePanes( CBRS_ALIGN_ANY );											// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableWindowsDialog( ID_WINDOW_MANAGER, ID_WINDOW_MANAGER, TRUE );				// enable enhanced windows management dialog

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu( TRUE, ID_VIEW_CUSTOMIZE, customizeLabel, ID_VIEW_TOOLBAR );		// IMP: any popup containing ID_VIEW_TOOLBAR will be rebuilt on via CDockingManager::BuildPanesMenu() with app tolbars

	CMFCToolBar::EnableQuickCustomization();			// enable quick (Alt+drag) toolbar customization


	OutputScalingMode( CWorkspace::GetData().m_scalingMode );




	/*** OLD Toolbars ***/
	ASSERT_PTR( pCS->hMenu );
	ui::SetMenuImages( CMenu::FromHandle( pCS->hMenu ) );			// m_hMenuDefault not initialized yet, but will

	DragAcceptFiles();			// enable drag&drop open

	if ( CWindowPlacement* pLoadedPlacement = CWorkspace::Instance().GetLoadedPlacement() )
		pLoadedPlacement->CommitWnd( this );						// 1st step: restore persistent placement, but with SW_HIDE; 2nd step will use the persisted AfxGetApp()->m_nCmdShow in app InitInstance()

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

LRESULT CMainFrame::OnResetToolbar( WPARAM toolBarResId, LPARAM )
{
	HandleResetToolbar( static_cast<UINT>( toolBarResId ) );
	return 0L;
}

void CMainFrame::OnGetMinMaxInfo( MINMAXINFO* mmi )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		__super::OnGetMinMaxInfo( mmi );
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
	else
		__super::OnTimer( eventId );
}

void CMainFrame::OnUpdateAlwaysEnabled( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( TRUE );
}

void CMainFrame::On_EscapeKey( void )
{
	CFrameWnd* pActiveFrame = MDIGetActive();

	if ( pActiveFrame != nullptr )
		if ( !ui::OwnsFocus( pActiveFrame->GetSafeHwnd() ) )
		{	// a toolbar control is focused -> focus the active view
			if ( CView* pActiveView = pActiveFrame->GetActiveView() )
				pActiveView->SetFocus();

			return;
		}

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

void CMainFrame::CmClearImageCache( void )
{
	app::GetThumbnailer()->Clear();
	CWicImageCache::Instance().Clear();
	app::GetApp()->UpdateAllViews( Hint_ReloadImage );
}

void CMainFrame::CmRefreshContent( void )
{
#ifdef USE_UT		// no UT code in release builds
	ut::CTestStatusProgress::Start( this, 10.0 );
#endif
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

void CMainFrame::On_FocusOnZoomCombo( void )
{
	if ( CMFCToolBarComboBoxButton* pComboBtn = mfc::FindFirstMatchingButton<CMFCToolBarComboBoxButton>( IDW_ZOOM_COMBO ) )
		if ( !pComboBtn->HasFocus() )
			ui::TakeFocus( pComboBtn->GetHwnd() );
}

void CMainFrame::On_FocusOnSliderCtrl( void )
{
	if ( mfc::CSliderButton* pSliderBtn = mfc::FindFirstMatchingButton<mfc::CSliderButton>( IDW_NAVIG_SLIDER_CTRL ) )
		if ( !pSliderBtn->HasFocus() )
			ui::TakeFocus( pSliderBtn->GetHwnd() );
}
