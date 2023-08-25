
#include "pch.h"
#include "AlbumImageView.h"
#include "AlbumDoc.h"
#include "AlbumDialogBar.h"
#include "AlbumThumbListView.h"
#include "ImageNavigator.h"
#include "INavigationBar.h"
#include "FileAttr.h"
#include "FileOperation.h"
#include "ChildFrame.h"
#include "MainFrame.h"
#include "Workspace.h"
#include "Application.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/RuntimeException.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/PostCall.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/WicImageCache.h"
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumImageView, CImageView )

CAlbumImageView::CAlbumImageView( void )
	: CImageView()
	, m_navTimer( this, ID_NAVIGATION_TIMER, m_slideData.m_slideDelay )
	, m_isDropTargetEnabled( false )
	, m_pPeerThumbView( nullptr )
	, m_pAlbumDialogBar( nullptr )
{
	ModifyScalingMode( ui::AutoFitLargeOnly );			// for albums use auto-fit by default
}

CAlbumImageView::~CAlbumImageView()
{
}

void CAlbumImageView::StorePeerView( CAlbumThumbListView* pPeerThumbView, CAlbumDialogBar* pAlbumDialogBar )
{
	// called just after creation, before initial update
	ASSERT_NULL( m_pPeerThumbView );
	ASSERT_NULL( m_pAlbumDialogBar );

	m_pPeerThumbView = pPeerThumbView;
	m_pAlbumDialogBar = pAlbumDialogBar;
}

CAlbumDoc* CAlbumImageView::GetDocument( void ) const
{
	return checked_static_cast<CAlbumDoc*>( m_pDocument );
}

HICON CAlbumImageView::GetDocTypeIcon( void ) const overrides(CImageView)
{
	static HICON hIconImage = AfxGetApp()->LoadIcon( IDR_ALBUMTYPE );
	return hIconImage;
}

CMenu& CAlbumImageView::GetDocContextMenu( void ) const overrides(CImageView)
{
	static CMenu s_contextMenu;
	if ( nullptr == s_contextMenu.GetSafeHmenu() )
		ui::LoadPopupMenu( &s_contextMenu, IDR_CONTEXT_MENU, app::AlbumPopup );

	return s_contextMenu;
}

CImageState* CAlbumImageView::GetLoadingImageState( void ) const overrides(CImageView)
{
	return GetDocument()->GetImageState();
}

fs::TImagePathKey CAlbumImageView::GetImagePathKey( void ) const overrides(CImageView)
{
	fs::TImagePathKey imagePathKey = GetDocument()->GetImageFilePathAt( m_slideData.GetCurrentIndex() );

	imagePathKey.second = m_slideData.GetCurrentNavPos().second;
	return imagePathKey;
}

CWicImage* CAlbumImageView::GetImage( void ) const overrides(CImageView)
{
	return CAlbumDoc::AcquireImage( GetImagePathKey() );
}

CWicImage* CAlbumImageView::QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const overrides(CImageView)
{
	CWicImage* pImage = __super::QueryImageFileDetails( rFileDetails );

	if ( pImage != nullptr )
	{
		const CAlbumModel* pAlbumModel = GetDocument()->GetModel();
		int currIndex = m_slideData.GetCurrentIndex();
		const CFileAttr* pFileAttr = pAlbumModel->GetFileAttr( currIndex );

		rFileDetails.m_fileSize = pFileAttr->GetFileSize();
		rFileDetails.m_navigPos = currIndex;
		rFileDetails.m_navigCount = static_cast<UINT>( pAlbumModel->GetFileAttrCount() );
	}
	return pImage;
}

void CAlbumImageView::OnImageContentChanged( void ) overrides(CImageView)
{
	// BUG - Windows Vista+ with scroll bar window theme: CScrollView::UpdateBars() succeeds for the SB_HORZ (1st) bar, but not for SB_VERT (2nd) bar.
	//	workaround: force a WM_SIZE
	ui::RecalculateScrollBars( m_hWnd );
	//was: SetupContentMetrics( true );
}

bool CAlbumImageView::OutputNavigSlider( void ) overrides(CImageView)
{
	if ( !app::GetMainFrame()->IsViewActive( this ) )
		return false;

	CAlbumDoc* pAlbumDoc = GetDocument();

	m_pNavigBar->OutputNavigRange( static_cast<UINT>( pAlbumDoc->GetImageCount() ) );
	m_pNavigBar->OutputNavigPos( GetSlideData().GetCurrentIndex() );
	return true;
}

bool CAlbumImageView::CanEnterDragMode( void ) const overrides(CImageView)
{
	return !GetDocument()->GetModel()->IsAutoDropRecipient( false );
}


// navigation support

std::tstring CAlbumImageView::FormatTipText_NavigSliderCtrl( void ) const overrides(CImageView)
{
	fs::TImagePathKey imgPathKey = GetImagePathKey();
	int imageCount = static_cast<int>( GetDocument()->GetImageCount() );
	std::tstring tipText = imgPathKey.first.Get() + str::Format( _T("  (%d of %d)"), m_slideData.GetCurrentIndex(), imageCount );
	CImageFrameNavigator imgFrame( GetImage() );

	if ( imgFrame.IsStaticMultiFrameImage() )
		tipText += str::Format( _T(" - [frame %d/%d]"), m_slideData.GetCurrentIndex(), imgFrame.GetFramePos() + 1, imgFrame.GetFrameCount() );

	return tipText;
}

bool CAlbumImageView::IsValidIndex( size_t index ) const
{
	return GetDocument()->IsValidIndex( index );
}

bool CAlbumImageView::UpdateImage( void )
{
	bool success = false;
	int currIndex = m_slideData.GetCurrentIndex();

	if ( IsValidIndex( currIndex ) )
	{
		CMainFrame* pMainFrame = app::GetMainFrame();

		try
		{
			CWaitCursor wait;
			CScopedErrorHandling scopedThrow( &CWicImageCache::Instance(), utl::ThrowMode );

			pMainFrame->CancelStatusBarAutoClear();

			if ( GetImage() != nullptr )
			{
				success = true;

				std::vector<fs::TImagePathKey> neighbours;
				QueryNeighbouringPathKeys( neighbours );
				CWicImageCache::Instance().Enqueue( neighbours );			// pre-emptively load the neighboring images - enqueue and load on queue listener thread
			}
		}
		catch ( CException* pExc )
		{
			pMainFrame->SetStatusBarMessage( str::FormatException( pExc, _T("Loading Error: %s") ).c_str(), 5000 );		// report modelessly the exception
			app::HandleException( pExc, MB_ICONWARNING );
		}
	}

	ui::EatPendingMessages();
	OnCurrPosChanged();			// update the views and dialog bars
	return success;
}

void CAlbumImageView::QueryNeighbouringPathKeys( std::vector<fs::TImagePathKey>& rNeighbourKeys ) const
{
	CAlbumNavigator navigator( this );
	nav::TIndexFramePosPair prevNavigInfo = navigator.GetNavigateInfo( nav::Previous );
	nav::TIndexFramePosPair nextNavigInfo = navigator.GetNavigateInfo( nav::Next );
	nav::TIndexFramePosPair currNavigInfo = navigator.GetCurrentInfo();

	if ( prevNavigInfo != currNavigInfo )
		rNeighbourKeys.push_back( navigator.MakePathKey( prevNavigInfo ) );

	if ( nextNavigInfo != currNavigInfo && nextNavigInfo != prevNavigInfo )
		rNeighbourKeys.push_back( navigator.MakePathKey( nextNavigInfo ) );
}

bool CAlbumImageView::TogglePlay( bool doBeep /*= true*/ )
{
	if ( !m_navTimer.IsStarted() )
		m_navTimer.Start();
	else
		m_navTimer.Stop();

	if ( doBeep )
		ui::BeepSignal();
	return IsPlayOn();
}

void CAlbumImageView::RestartPlayTimer( void )
{
	if ( m_navTimer.IsStarted() )
		m_navTimer.Start();
}

void CAlbumImageView::SetSlideDelay( UINT slideDelay )
{
	m_navTimer.SetElapsed( m_slideData.m_slideDelay = slideDelay );
}

void CAlbumImageView::HandleNavTick( void )
{
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		return;				// temporarily freeze navigation

	bool dirForward = m_slideData.m_dirForward;
	if ( ui::IsKeyPressed( VK_SHIFT ) )
		dirForward = !dirForward;

	CAlbumNavigator navigator( this );
	nav::TIndexFramePosPair newNavigInfo = navigator.GetNavigateInfo( dirForward ? nav::Next : nav::Previous );

	m_slideData.SetCurrentNavPos( newNavigInfo );
	UpdateImage();

	if ( !m_slideData.m_wrapMode && IsPlayOn() )			// must stop at limits?
		if ( CAlbumNavigator::IsAtLimit( this ) )			// has reached the limit (post-navigation)?
			TogglePlay();
}

void CAlbumImageView::NavigateTo( int pos, bool relative /*= false*/ )
{
	int imageCount = (int)GetDocument()->GetImageCount();
	int currIndex = m_slideData.GetCurrentIndex();

	if ( !relative )
		currIndex = pos;
	else
		currIndex += pos;

	currIndex = std::max( currIndex, 0 );
	currIndex = std::min( currIndex, imageCount - 1 );

	m_slideData.SetCurrentIndex( currIndex );
	UpdateImage();
}

void CAlbumImageView::LateInitialUpdate( void )
{
	const CListViewState& currListState = m_slideData.GetCurrListState();
	if ( !currListState.IsEmpty() )
		m_pPeerThumbView->SetListViewState( currListState );		// restore the persistent selection
	m_pPeerThumbView->ShowWindow( SW_SHOW );

	CView* pViewToActivate = m_slideData.HasShowFlag( af::ShowThumbView ) ? static_cast<CView*>( m_pPeerThumbView ) : this;

	// activate the target pane view (works only late)
	m_pMdiChildFrame->SetActiveView( pViewToActivate );
	pViewToActivate->SetFocus();
}

void CAlbumImageView::UpdateChildBarsState( bool onInit /*= false*/ )
{
	m_pAlbumDialogBar->ShowPane( m_slideData.HasShowFlag( af::ShowAlbumDialogBar ), false, false );
	m_pPeerThumbView->CheckListLayout( onInit ? CAlbumThumbListView::AlbumViewInit : CAlbumThumbListView::ShowCommand );
}

void CAlbumImageView::OnAlbumModelChanged( AlbumModelChange reason /*= AM_Init*/ )
{
	CAlbumDoc* pDoc = GetDocument();

	switch ( reason )
	{
		case AM_Init:
			m_slideData = pDoc->m_slideData;
			break;
		case AM_AutoDropOp:
			if ( !pDoc->m_autoDropContext.m_droppedDestFiles.empty() )
				m_slideData.SetCurrentIndex( pDoc->GetModel()->FindIndexFileAttrWithPath( fs::CFlexPath( pDoc->m_autoDropContext.m_droppedDestFiles[ 0 ] ) ) );		// that'd be the caret selected index

			// replace with the standard one if invalid
			if ( !IsValidIndex( m_slideData.GetCurrentIndex() ) )
				m_slideData.SetCurrentIndex( pDoc->m_slideData.GetCurrentIndex() );
			break;
	}

	m_pPeerThumbView->SetupAlbumModel( pDoc->GetModel() );

	switch ( reason )
	{
		case AM_Init:
		case AM_Regeneration:
		case AM_AutoDropOp:
			m_pAlbumDialogBar->OnNavRangeChanged();
			OutputNavigSlider();
			break;
	}

	// AM_Regeneration: navigation range may have changed, and UpdateImage() will be called later when the selection backup is restored
	// AM_CustomOrderChanged: navigation range is not changed for sure, and UpdateImage() will be called later when the selection backup is restored
	//
	if ( AM_Init == reason || AM_AutoDropOp == reason )
		UpdateImage();

	if ( AM_AutoDropOp == reason )
	{
		CListViewState autoDropSelState( pDoc->m_autoDropContext.m_droppedDestFiles );
		m_pPeerThumbView->SetListViewState( autoDropSelState, true );
	}
}

void CAlbumImageView::OnAutoDropRecipientChanged( void )
{
	CAlbumDoc* pDoc = GetDocument();

	if ( pDoc->GetModel()->IsAutoDropRecipient() != m_isDropTargetEnabled )
	{	// auto drop has changed
		m_isDropTargetEnabled = pDoc->GetModel()->IsAutoDropRecipient();

		pDoc->InitAutoDropRecipient();			// setup/clear the auto-drop recipient search pattern

		DragAcceptFiles( m_isDropTargetEnabled );
		m_pPeerThumbView->DragAcceptFiles( m_isDropTargetEnabled );
		if ( !m_isDropTargetEnabled )
			pDoc->ClearAutoDropContext();		// auto-drop turned off -> clear the undo buffer and all related data
	}
}

void CAlbumImageView::OnSlideDataChanged( bool setToModified /*= true*/ )
{
	CAlbumDoc* pDoc = GetDocument();

	// copy current navigation attributes to document (i.e. persistent attributes)
	pDoc->m_slideData = m_slideData;

	if ( setToModified )
		pDoc->SetModifiedFlag();		// mark document as modified in order to prompt for saving
}

void CAlbumImageView::OnDocSlideDataChanged( void )
{
	m_slideData = GetDocument()->m_slideData;
	m_navTimer.SetElapsed( m_slideData.m_slideDelay );
	UpdateImage();
}

void CAlbumImageView::OnCurrPosChanged( bool alsoSliderCtrl /*= true*/ )
{
	m_pAlbumDialogBar->OnCurrPosChanged();
	OnImageContentChanged();

	int currIndex = m_slideData.GetCurrentIndex();

	if ( currIndex != m_pPeerThumbView->GetCurSel() )
		m_pPeerThumbView->SetCurSel( currIndex );

	if ( alsoSliderCtrl && app::GetMainFrame()->IsViewActive( this ) )
		OutputNavigSlider();
}

void CAlbumImageView::OnSelChangeThumbList( void )
{
	m_pPeerThumbView->GetListViewState( m_slideData.RefCurrListState(), false, false );
	UpdateImage();
}

void CAlbumImageView::EventChildFrameActivated( void ) overrides(CImageView)
{
	// called when the parent frame activates -> this will be the curent image view
	__super::EventChildFrameActivated();
	GetDocument()->m_slideData = m_slideData;		// copy current navigation attributes to document (i.e. persistent attributes)
}

void CAlbumImageView::HandleNavigSliderPosChanging( int newPos, bool thumbTracking ) overrides(CImageView)
{
	m_slideData.SetCurrentIndex( newPos /*m_pNavigBar->InputNavigPos()*/ );

	if ( thumbTracking )
		OnCurrPosChanged( false );
	else
		UpdateImage();
}

void CAlbumImageView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) overrides(CImageView)
{
	UpdateViewHint hint = (UpdateViewHint)lHint;

	switch ( hint )
	{
		case Hint_ToggleFullScreen:
			UpdateChildBarsState();
			break;
		case Hint_AlbumModelChanged:
			OnAlbumModelChanged( app::FromHintPtr<AlbumModelChange>( pHint ) );
			break;
		case Hint_ReloadImage:
			m_pPeerThumbView->Invalidate();		// also invalidate the peer thumb view
			break;
		case Hint_DocSlideDataChanged:
			OnDocSlideDataChanged();
			break;
		case Hint_FileChanged:
			OnSelChangeThumbList();
			break;
		case Hint_BackupCurrSelection:
		case Hint_BackupNearSelection:
			m_pPeerThumbView->BackupSelection( hint == Hint_BackupCurrSelection );
			return;			// no base update (no redraw, etc)
		case Hint_SmartBackupSelection:
			m_pPeerThumbView->BackupSelection( !m_pPeerThumbView->SelectionOverlapsWith( m_pPeerThumbView->GetDragSelIndexes() ) );
			break;
		case Hint_RestoreSelection:
			m_pPeerThumbView->RestoreSelection();
			break;
	}

	__super::OnUpdate( pSender, lHint, pHint );
}

BOOL CAlbumImageView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	// redirect to thumb view for processing common IDs (if that's the case)
	// note: call CCtrlView::OnCmdMsg non-virtual version in order to avoid infinite recursion
	if ( m_pPeerThumbView->CCtrlView::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;
	return FALSE;
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumImageView, CImageView )
	ON_WM_DROPFILES()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_COMMAND( ID_EDIT_ALBUM, On_EditAlbum )
	ON_COMMAND( CM_TOGGLE_SIBLING_VIEW, OnToggleSiblingView )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_SIBLING_VIEW, OnUpdateSiblingView )
	ON_COMMAND( ID_TOGGLE_NAVIG_PLAY, OnToggle_NavigPlay )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_NAVIG_PLAY, OnUpdate_NavigPlay )
	ON_COMMAND_RANGE( ID_TOGGLE_NAVIG_DIR_FWD, ID_TOGGLE_NAVIG_DIR_REV, OnRadio_NavigDirection )
	ON_UPDATE_COMMAND_UI_RANGE( ID_TOGGLE_NAVIG_DIR_FWD, ID_TOGGLE_NAVIG_DIR_REV, OnUpdate_NavigDirection )
	ON_COMMAND( ID_TOGGLE_NAVIG_WRAP_MODE, OnToggle_NavigWrapMode )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_NAVIG_WRAP_MODE, OnUpdate_NavigWrapMode )
	ON_UPDATE_COMMAND_UI( IDW_NAVIG_SLIDER_CTRL, OnUpdate_NavigSliderCtrl )
	ON_COMMAND_RANGE( CM_DROP_MOVE_IMAGE, CM_CANCEL_DROP, CmAutoDropImage )
	ON_UPDATE_COMMAND_UI_RANGE( CM_DROP_MOVE_IMAGE, CM_CANCEL_DROP, OnUpdateAutoDropImage )
END_MESSAGE_MAP()

void CAlbumImageView::OnInitialUpdate( void )
{
	CAlbumDoc* pDoc = GetDocument();

	// at first time update, copy de-persisted navigation attributes and background color
	m_slideData = pDoc->m_slideData;
	SetBkColor( pDoc->GetBkColor(), false );
	RefDrawParams()->SetSmoothingMode( pDoc->GetSmoothingMode() );

	m_pAlbumDialogBar->InitAlbumImageView( this );
	UpdateChildBarsState( true );		// info bar and thumb pane are hidden in full screen mode

	m_pAlbumDialogBar->OnNavRangeChanged();
	m_pAlbumDialogBar->OnSlideDelayChanged();

	if ( nullptr == m_pPeerThumbView->GetAlbumModel() )		// avoid double setup on initialization (it might happen cause of different ways of init, e.g. load or drop)
		m_pPeerThumbView->SetupAlbumModel( GetDocument()->GetModel() );

	OnAutoDropRecipientChanged();

	UpdateImage();			// only after this call image become valid (with proper metrics)

	__super::OnInitialUpdate();

	OnDocSlideDataChanged();		// bug fix: sync with the assigned slide data from document after loading
	UpdateWindow();
	ui::PostCall( this, &CAlbumImageView::LateInitialUpdate );
}

void CAlbumImageView::OnDropFiles( HDROP hDropInfo )
{
	GetDocument()->HandleDropRecipientFiles( hDropInfo, this );
}

void CAlbumImageView::OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags )
{
	__super::OnKeyDown( chr, repCnt, vkFlags );

	switch ( chr )
	{
		case VK_PRIOR:		// PAGE UP key
		case VK_NEXT:		// PAGE DOWN key
			// mirror these navigation keys to the thumb list view (only if non SHIFT or CONTROL - careful: multiple selection)
			if ( !ui::IsKeyPressed( VK_SHIFT ) && !ui::IsKeyPressed( VK_CONTROL ) )
				m_pPeerThumbView->SendMessage( WM_KEYDOWN, chr, MAKELPARAM( repCnt, vkFlags ) );
			break;
	}
}

void CAlbumImageView::OnTimer( UINT_PTR eventId )
{
	if ( m_navTimer.IsHit( eventId ) )
		HandleNavTick();
	else
		__super::OnTimer( eventId );
}

BOOL CAlbumImageView::OnMouseWheel( UINT mkFlags, short zDelta, CPoint point )
{
	if ( !HasFlag( mkFlags, MK_CONTROL ) )
	{
		int delta = -( zDelta / WHEEL_DELTA );
		if ( delta != 0 )
		{
			size_t currPos = m_slideData.GetCurrentIndex();
			size_t step = HasFlag( mkFlags, MK_SHIFT ) ? m_pPeerThumbView->GetPageItemCounts().cy : 1;		// advance by vert-page is SHIFT pressed

			utl::AdvancePos( currPos, GetDocument()->GetImageCount(), false, delta < 0, step );		// forward for wheel forward
			m_slideData.SetCurrentIndex( static_cast<int>( currPos ) );
			UpdateImage();
		}
		return TRUE;		// message processed
	}

	return __super::OnMouseWheel( mkFlags, zDelta, point );
}

void CAlbumImageView::On_EditAlbum( void )
{
	GetDocument()->EditAlbum( this );
}

void CAlbumImageView::OnToggleSiblingView( void )
{
	CView* pActiveView = m_pMdiChildFrame->GetActiveView();

	ASSERT( nullptr == pActiveView || ( pActiveView == this || pActiveView == m_pPeerThumbView ) );
	if ( nullptr == pActiveView || pActiveView == m_pPeerThumbView )
		m_pMdiChildFrame->SetActiveView( this );
	else
		m_pMdiChildFrame->SetActiveView( m_pPeerThumbView );
}

void CAlbumImageView::OnUpdateSiblingView( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_slideData.HasShowFlag( af::ShowThumbView ) );
}

void CAlbumImageView::OnToggle_NavigPlay( void )
{
	TogglePlay();
}

void CAlbumImageView::OnUpdate_NavigPlay( CCmdUI* pCmdUI )
{
	CAlbumDoc* pDoc = GetDocument();
	int imageCount = (int)pDoc->GetImageCount();
	bool toEnable = imageCount > 0;
	int currIndex = m_slideData.GetCurrentIndex();

	if ( toEnable && !m_slideData.m_wrapMode )
		if ( m_slideData.m_dirForward )
			toEnable &= ( currIndex < imageCount - 1 );
		else
			toEnable &= ( currIndex > 0 );

	pCmdUI->Enable( toEnable );
	pCmdUI->SetCheck( IsPlayOn() );
}

void CAlbumImageView::On_NavigSeek( UINT cmdId )
{
	nav::Navigate navigate = CmdToNavigate( cmdId );
	CAlbumNavigator navigator( this );
	nav::TIndexFramePosPair newNavigInfo = navigator.GetNavigateInfo( navigate );

	m_slideData.SetCurrentNavPos( newNavigInfo );

	UpdateImage();
	RestartPlayTimer();
}

void CAlbumImageView::OnUpdate_NavigSeek( CCmdUI* pCmdUI )
{
	nav::Navigate navigate = CmdToNavigate( pCmdUI->m_nID );
	CAlbumNavigator navigator( this );

	pCmdUI->Enable( navigator.CanNavigate( navigate ) );
}

void CAlbumImageView::OnRadio_NavigDirection( UINT cmdId )
{
	m_slideData.m_dirForward = ID_TOGGLE_NAVIG_DIR_FWD == cmdId;
	OnSlideDataChanged();
}

void CAlbumImageView::OnUpdate_NavigDirection( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, m_slideData.m_dirForward == ( ID_TOGGLE_NAVIG_DIR_FWD == pCmdUI->m_nID ) );
}

void CAlbumImageView::OnToggle_NavigWrapMode( void )
{
	m_slideData.m_wrapMode = !m_slideData.m_wrapMode;
	OnSlideDataChanged();
}

void CAlbumImageView::OnUpdate_NavigWrapMode( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_slideData.m_wrapMode );
}

void CAlbumImageView::OnUpdate_NavigSliderCtrl( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( GetDocument()->HasImages() );
}

void CAlbumImageView::CmAutoDropImage( UINT cmdId )
{
	CAlbumDoc* pDoc = GetDocument();

	switch ( cmdId )
	{
		case CM_DROP_MOVE_IMAGE:
			pDoc->m_autoDropContext.SetDropOperation( auto_drop::CContext::FileMove );
			pDoc->ExecuteAutoDrop();
			break;
		case CM_DROP_COPY_IMAGE:
			pDoc->m_autoDropContext.SetDropOperation( auto_drop::CContext::FileCopy );
			pDoc->ExecuteAutoDrop();
			break;
		case CM_CANCEL_DROP:
			break;
		default:
			ASSERT( false );
	}
}

void CAlbumImageView::OnUpdateAutoDropImage( CCmdUI* pCmdUI )
{
	if ( CM_CANCEL_DROP == pCmdUI->m_nID )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( GetDocument()->m_autoDropContext.GetFileCount() > 0 );
}
