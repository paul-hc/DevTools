
#include "stdafx.h"
#include "AlbumImageView.h"
#include "AlbumDoc.h"
#include "AlbumDialogBar.h"
#include "AlbumThumbListView.h"
#include "INavigationBar.h"
#include "ChildFrame.h"
#include "MainFrame.h"
#include "Workspace.h"
#include "Application.h"
#include "resource.h"
#include "utl/RuntimeException.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/PostCall.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImageCache.h"
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumImageView, CImageView )

CAccelTable CAlbumImageView::s_accelNavigate;

CAlbumImageView::CAlbumImageView( void )
	: CImageView()
	, m_navTimer( this, ID_NAVIGATION_TIMER, m_slideData.m_slideDelay )
	, m_isDropTargetEnabled( false )
	, m_pPeerThumbView( NULL )
	, m_pAlbumDialogBar( NULL )
{
	s_accelNavigate.LoadOnce( IDR_ALBUMTYPE_NAV );

	ModifyAutoImageSize( ui::AutoFitLargeOnly );			// for albums use auto-fit by default
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
	return checked_static_cast< CAlbumDoc* >( m_pDocument );
}

HICON CAlbumImageView::GetDocTypeIcon( void ) const
{
	static HICON hIconImage = AfxGetApp()->LoadIcon( IDR_ALBUMTYPE );
	return hIconImage;
}

CMenu& CAlbumImageView::GetDocContextMenu( void ) const
{
	static CMenu contextMenu;
	if ( NULL == (HMENU)contextMenu )
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AlbumPopup );
	return contextMenu;
}

const fs::ImagePathKey& CAlbumImageView::GetImagePathKey( void ) const
{
	return GetDocument()->GetImageFilePathAt( m_slideData.GetCurrentIndex() );
}

CWicImage* CAlbumImageView::GetImage( void ) const
{
	const fs::ImagePathKey& imagePathKey = GetImagePathKey();

	if ( imagePathKey.first.IsEmpty() )
		return NULL;

	return CWicImageCache::Instance().Acquire( imagePathKey ).first;
}

void CAlbumImageView::OnImageContentChanged( void )
{
	SetupContentMetrics( true );
}

bool CAlbumImageView::OutputNavigSlider( void )
{
	if ( !app::GetMainFrame()->IsViewActive( this ) )
		return false;

	CAlbumDoc* pAlbumDoc = GetDocument();

	m_pNavigBar->OutputNavigRange( static_cast< UINT >( pAlbumDoc->GetImageCount() ) );
	m_pNavigBar->OutputNavigPos( GetSlideData().GetCurrentIndex() );
	return true;
}

bool CAlbumImageView::CanEnterDragMode( void ) const
{
	return !GetDocument()->m_fileList.IsAutoDropRecipient( false );
}

CImageState* CAlbumImageView::GetLoadingImageState( void ) const
{
	return GetDocument()->GetImageState();
}


// navigation support

bool CAlbumImageView::IsValidIndex( size_t index ) const
{
	return GetDocument()->IsValidIndex( index );
}

bool CAlbumImageView::UpdateImage( void )
{
	bool success = false;
	CAlbumDoc* pDoc = GetDocument();
	int currIndex = m_slideData.GetCurrentIndex();

	if ( IsValidIndex( currIndex ) )
	{
		CMainFrame* pMainFrame = app::GetMainFrame();

		try
		{
			CWaitCursor wait;
			CPushThrowMode pushMode( &CWicImageCache::Instance(), true );

			pMainFrame->CancelStatusBarAutoClear();

			if ( GetImage() != NULL )
			{
				success = true;

				std::vector< fs::ImagePathKey > neighbours;
				pDoc->QueryNeighboringPathKeys( neighbours, currIndex );
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

void CAlbumImageView::SetSlideDelay( UINT slideDelay )
{
	m_navTimer.SetElapsed( m_slideData.m_slideDelay = slideDelay );
}

void CAlbumImageView::HandleNavTick( void )
{
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		return;				// temporarily freeze navigation

	bool dirForward = m_slideData.m_dirForward == !ui::IsKeyPressed( VK_SHIFT );
	size_t currPos = m_slideData.GetCurrentIndex();

	if ( !utl::AdvancePos( currPos, GetDocument()->GetImageCount(), m_slideData.m_circular, dirForward ) )		// overflow/underflow?
		if ( IsPlayOn() )
			TogglePlay();

	m_slideData.SetCurrentIndex( static_cast< int >( currPos ) );
	UpdateImage();
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

	CView* pViewToActivate = HasFlag( m_slideData.m_viewFlags, af::ShowThumbView ) ? static_cast< CView* >( m_pPeerThumbView ) : this;

	// activate the target pane view (works only late)
	m_pMdiChildFrame->SetActiveView( pViewToActivate );
	pViewToActivate->SetFocus();
}

void CAlbumImageView::UpdateChildBarsState( bool onInit /*= false*/ )
{
	m_pAlbumDialogBar->ShowBar( HasFlag( m_slideData.m_viewFlags, af::ShowAlbumDialogBar ) && !app::GetMainFrame()->IsFullScreen() );
	m_pPeerThumbView->CheckListLayout( onInit ? CAlbumThumbListView::AlbumViewInit : CAlbumThumbListView::ShowCommand );
}

void CAlbumImageView::OnFileListChanged( FileListChangeType reason /*= FL_Init*/ )
{
	CAlbumDoc* pDoc = GetDocument();

	switch ( reason )
	{
		case FL_Init:
			m_slideData = pDoc->m_slideData;
			break;
		case FL_AutoDropOp:
			if ( !pDoc->m_autoDropContext.m_droppedDestFiles.empty() )
				m_slideData.SetCurrentIndex( pDoc->m_fileList.FindFileAttr( fs::CFlexPath( pDoc->m_autoDropContext.m_droppedDestFiles[ 0 ] ) ) );		// that'd be the caret selected index

			// replace with the standard one if invalid
			if ( !IsValidIndex( m_slideData.GetCurrentIndex() ) )
				m_slideData.SetCurrentIndex( pDoc->m_slideData.GetCurrentIndex() );
			break;
	}

	m_pPeerThumbView->SetupFileList( &pDoc->m_fileList );

	switch ( reason )
	{
		case FL_Init:
		case FL_Regeneration:
		case FL_AutoDropOp:
			m_pAlbumDialogBar->OnNavRangeChanged();
			OutputNavigSlider();
			break;
	}

	// FL_Regeneration: navigation range may have changed, and UpdateImage() will be called later when the selection backup is restored
	// FL_CustomOrderChanged: navigation range is not changed for sure, and UpdateImage() will be called later when the selection backup is restored
	//
	if ( FL_Init == reason || FL_AutoDropOp == reason )
		UpdateImage();

	if ( FL_AutoDropOp == reason )
	{
		CListViewState autoDropSelState( pDoc->m_autoDropContext.m_droppedDestFiles );
		m_pPeerThumbView->SetListViewState( autoDropSelState, true );
	}
}

void CAlbumImageView::OnAutoDropRecipientChanged( void )
{
	CAlbumDoc* pDoc = GetDocument();

	if ( pDoc->m_fileList.IsAutoDropRecipient() != m_isDropTargetEnabled )
	{	// auto drop has changed
		m_isDropTargetEnabled = pDoc->m_fileList.IsAutoDropRecipient();

		pDoc->InitAutoDropRecipient();			// setup/clear the auto-drop recipient search spec

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

void CAlbumImageView::EventChildFrameActivated( void )
{
	// called when the parent frame activates -> this will be the curent image view
	CImageView::EventChildFrameActivated();
	GetDocument()->m_slideData = m_slideData;		// copy current navigation attributes to document (i.e. persistent attributes)
}

void CAlbumImageView::EventNavigSliderPosChanged( bool thumbTracking )
{
	m_slideData.SetCurrentIndex( m_pNavigBar->InputNavigPos() );

	if ( thumbTracking )
		OnCurrPosChanged( false );
	else
		UpdateImage();
}

void CAlbumImageView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	UpdateViewHint hint = (UpdateViewHint)lHint;

	switch ( hint )
	{
		case Hint_ToggleFullScreen:
			UpdateChildBarsState();
			break;
		case Hint_FileListChanged:
			OnFileListChanged( app::FromHintPtr< FileListChangeType >( pHint ) );
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
			m_pPeerThumbView->BackupSelection( !m_pPeerThumbView->SelectionOverlapsWith( CAlbumThumbListView::s_toMoveIndexes ) );
			break;
		case Hint_RestoreSelection:
			m_pPeerThumbView->RestoreSelection();
			break;
	}

	CImageView::OnUpdate( pSender, lHint, pHint );
}

BOOL CAlbumImageView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( CImageView::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	// redirect to thumb view for processing common IDs (if that's the case)
	// note: call CCtrlView::OnCmdMsg non-virtual version in order to avoid infinite recursion
	if ( m_pPeerThumbView->CCtrlView::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;
	return FALSE;
}

BOOL CAlbumImageView::PreTranslateMessage( MSG* pMsg )
{
	return
		s_accelNavigate.Translate( pMsg, m_hWnd ) ||
		CImageView::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumImageView, CImageView )
	ON_WM_DROPFILES()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_COMMAND( CM_EDIT_ALBUM, CmEditAlbum )
	ON_COMMAND( CM_TOGGLE_SIBLING_VIEW, OnToggleSiblingView )
	ON_UPDATE_COMMAND_UI( CM_TOGGLE_SIBLING_VIEW, OnUpdateSiblingView )
	ON_COMMAND( CM_NAV_PLAY, OnToggleNavPlay )
	ON_UPDATE_COMMAND_UI( CM_NAV_PLAY, OnUpdateNavPlay )
	ON_COMMAND( CM_NAV_BEGIN, CmNavBegin )
	ON_UPDATE_COMMAND_UI( CM_NAV_BEGIN, OnUpdateNavBegin )
	ON_COMMAND( CM_NAV_END, CmNavEnd )
	ON_UPDATE_COMMAND_UI( CM_NAV_END, OnUpdateNavEnd )
	ON_COMMAND( CM_NAV_PREV, CmNavPrev )
	ON_UPDATE_COMMAND_UI( CM_NAV_PREV, OnUpdateNavPrev )
	ON_COMMAND( CM_NAV_NEXT, CmNavNext )
	ON_UPDATE_COMMAND_UI( CM_NAV_NEXT, OnUpdateNavNext )
	ON_COMMAND_RANGE( CM_NAV_DIR_FWD, CM_NAV_DIR_REV, OnRadioNavigDirection )
	ON_UPDATE_COMMAND_UI_RANGE( CM_NAV_DIR_FWD, CM_NAV_DIR_REV, OnUpdateNavigDirection )
	ON_COMMAND( CM_NAV_CIRCULAR, OnToggleNavCircular )
	ON_UPDATE_COMMAND_UI( CM_NAV_CIRCULAR, OnUpdateNavCircular )
	ON_UPDATE_COMMAND_UI( IDW_NAV_SLIDER, OnUpdateNavSlider )
	ON_COMMAND( CM_OPEN_IMAGE_FILE, CmOpenImageFile )
	ON_UPDATE_COMMAND_UI( CM_OPEN_IMAGE_FILE, CImageView::OnUpdateAnyFileShellOperation )
	ON_COMMAND_RANGE( CM_DROP_MOVE_IMAGE, CM_CANCEL_DROP, CmAutoDropImage )
	ON_UPDATE_COMMAND_UI_RANGE( CM_DROP_MOVE_IMAGE, CM_CANCEL_DROP, OnUpdateAutoDropImage )
END_MESSAGE_MAP()

void CAlbumImageView::OnInitialUpdate( void )
{
	CAlbumDoc* pDoc = GetDocument();

	// at first time update, copy de-persisted navigation attributes and background color
	m_slideData = pDoc->m_slideData;
	SetBkColor( pDoc->GetBkColor(), false );

	m_pAlbumDialogBar->InitAlbumImageView( this );
	UpdateChildBarsState( true );		// info bar and thumb pane are hidden in full screen mode

	m_pAlbumDialogBar->OnNavRangeChanged();
	m_pAlbumDialogBar->OnSlideDelayChanged();

	if ( NULL == m_pPeerThumbView->GetFileList() )		// avoid double setup on initialization (it might happen cause of different ways of init, e.g. load or drop)
		m_pPeerThumbView->SetupFileList( &GetDocument()->m_fileList );

	OnAutoDropRecipientChanged();

	UpdateImage();			// only after this call image become valid (with proper metrics)

	CImageView::OnInitialUpdate();

	UpdateWindow();
	ui::PostCall( this, &CAlbumImageView::LateInitialUpdate );
}

void CAlbumImageView::OnDropFiles( HDROP hDropInfo )
{
	GetDocument()->HandleDropRecipientFiles( hDropInfo, this );
}

void CAlbumImageView::OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags )
{
	CImageView::OnKeyDown( chr, repCnt, vkFlags );
	switch ( chr )
	{
		case VK_PRIOR:
		case VK_NEXT:
		case VK_LEFT:
		case VK_RIGHT:
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
		CImageView::OnTimer( eventId );
}

BOOL CAlbumImageView::OnMouseWheel( UINT mkFlags, short zDelta, CPoint point )
{
	if ( !HasFlag( mkFlags, MK_CONTROL ) )
	{
		int delta = -( zDelta / WHEEL_DELTA );
		if ( delta != 0 )
		{
			size_t currPos = m_slideData.GetCurrentIndex();
			size_t step = HasFlag( mkFlags, MK_SHIFT ) ? m_pPeerThumbView->GetPageScrollExtent().cy : 1;		// advance by vert-page is SHIFT pressed

			utl::AdvancePos( currPos, GetDocument()->GetImageCount(), false, delta < 0, step );		// forward for wheel forward
			m_slideData.SetCurrentIndex( static_cast< int >( currPos ) );
			UpdateImage();
		}
		return TRUE;		// message processed
	}
	return CImageView::OnMouseWheel( mkFlags, zDelta, point );
}

void CAlbumImageView::CmEditAlbum( void )
{
	GetDocument()->EditAlbum( this );
}

void CAlbumImageView::OnToggleSiblingView( void )
{
	CView* pActiveView = m_pMdiChildFrame->GetActiveView();

	ASSERT( NULL == pActiveView || ( pActiveView == this || pActiveView == m_pPeerThumbView ) );
	if ( NULL == pActiveView || pActiveView == m_pPeerThumbView )
		m_pMdiChildFrame->SetActiveView( this );
	else
		m_pMdiChildFrame->SetActiveView( m_pPeerThumbView );
}

void CAlbumImageView::OnUpdateSiblingView( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( HasFlag( m_slideData.m_viewFlags, af::ShowThumbView ) );
}

void CAlbumImageView::OnToggleNavPlay( void )
{
	TogglePlay();
}

void CAlbumImageView::OnUpdateNavPlay( CCmdUI* pCmdUI )
{
	CAlbumDoc* pDoc = GetDocument();
	int imageCount = (int)pDoc->GetImageCount();
	bool toEnable = imageCount > 0;
	int currIndex = m_slideData.GetCurrentIndex();

	if ( toEnable && !m_slideData.m_circular )
		if ( m_slideData.m_dirForward )
			toEnable &= ( currIndex < imageCount - 1 );
		else
			toEnable &= ( currIndex > 0 );

	pCmdUI->Enable( toEnable );
	pCmdUI->SetCheck( IsPlayOn() );
}

void CAlbumImageView::CmNavBegin( void )
{
	if ( IsPlayOn() )
		TogglePlay( false );
	m_slideData.SetCurrentIndex( 0 );
	UpdateImage();
}

void CAlbumImageView::OnUpdateNavBegin( CCmdUI* pCmdUI )
{
	bool hasImages = GetDocument()->HasImages();

	pCmdUI->Enable( hasImages && m_slideData.GetCurrentIndex() != 0 );
}

void CAlbumImageView::CmNavEnd( void )
{
	if ( IsPlayOn() )
		TogglePlay( false );
	m_slideData.SetCurrentIndex( (int)GetDocument()->GetImageCount() - 1 );
	UpdateImage();
}

void CAlbumImageView::OnUpdateNavEnd( CCmdUI* pCmdUI )
{
	CAlbumDoc* pDoc = GetDocument();
	bool hasImages = pDoc->HasImages();
	bool atEnd = ( m_slideData.GetCurrentIndex() == static_cast< int >( pDoc->GetImageCount() - 1 ) );

	pCmdUI->Enable( hasImages && !atEnd );
}

void CAlbumImageView::CmNavPrev( void )
{
	int imageCount = (int)GetDocument()->GetImageCount();
	int currIndex = m_slideData.GetCurrentIndex();

	if ( IsPlayOn() )
		TogglePlay( false );
	--currIndex;
	if ( currIndex < 0 )
		currIndex = imageCount - 1;
	m_slideData.SetCurrentIndex( currIndex );
	UpdateImage();
}

void CAlbumImageView::OnUpdateNavPrev( CCmdUI* pCmdUI )
{
	CAlbumDoc* pDoc = GetDocument();
	bool hasImages = pDoc->HasImages();

	pCmdUI->Enable( hasImages && ( m_slideData.m_circular || m_slideData.GetCurrentIndex() > 0 ) );
}

void CAlbumImageView::CmNavNext( void )
{
	int imageCount = (int)GetDocument()->GetImageCount();
	int currIndex = m_slideData.GetCurrentIndex();

	if ( IsPlayOn() )
		TogglePlay( false );
	++currIndex;
	if ( currIndex >= imageCount )
		currIndex = 0;
	m_slideData.SetCurrentIndex( currIndex );
	UpdateImage();
}

void CAlbumImageView::OnUpdateNavNext( CCmdUI* pCmdUI )
{
	CAlbumDoc* pDoc = GetDocument();
	bool hasImages = pDoc->HasImages();
	int imageCount = (int)pDoc->GetImageCount();

	pCmdUI->Enable( hasImages && ( m_slideData.m_circular || m_slideData.GetCurrentIndex() < imageCount - 1 ) );
}

void CAlbumImageView::OnRadioNavigDirection( UINT cmdId )
{
	m_slideData.m_dirForward = CM_NAV_DIR_FWD == cmdId;
	OnSlideDataChanged();
}

void CAlbumImageView::OnUpdateNavigDirection( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, m_slideData.m_dirForward == ( CM_NAV_DIR_FWD == pCmdUI->m_nID ) );
}

void CAlbumImageView::OnToggleNavCircular( void )
{
	m_slideData.m_circular = !m_slideData.m_circular;
	OnSlideDataChanged();
}

void CAlbumImageView::OnUpdateNavCircular( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_slideData.m_circular );
}

void CAlbumImageView::OnUpdateNavSlider( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( GetDocument()->HasImages() );
}

void CAlbumImageView::CmOpenImageFile( void )
{
	CListViewState filesToOpen( StoreByString );
	m_pPeerThumbView->GetListViewState( filesToOpen );

	if ( !filesToOpen.m_pStringImpl->m_selItems.empty() )
		for ( std::vector< std::tstring >::const_iterator it = filesToOpen.m_pStringImpl->m_selItems.begin(); it != filesToOpen.m_pStringImpl->m_selItems.end(); ++it )
			AfxGetApp()->OpenDocumentFile( it->c_str() );
}

void CAlbumImageView::CmDeleteFile( UINT cmdId )
{
	CListViewState filesToDelete( StoreByString );
	m_pPeerThumbView->GetListViewState( filesToDelete );

	if ( !filesToDelete.m_pStringImpl->m_selItems.empty() )
		app::DeleteFiles( filesToDelete.m_pStringImpl->m_selItems, CM_DELETE_FILE == cmdId );
	else
		TRACE( _T("(!) No target files selected!\n") );
}

void CAlbumImageView::CmMoveFile( void )
{
	CListViewState filesToMove( StoreByString );
	m_pPeerThumbView->GetListViewState( filesToMove );

	if ( !filesToMove.m_pStringImpl->m_selItems.empty() )
		app::MoveFiles( filesToMove.m_pStringImpl->m_selItems, this );
	else
		TRACE( _T("(!) No target files selected!\n") );
}

void CAlbumImageView::CmAutoDropImage( UINT cmdId )
{
	CAlbumDoc* pDoc = GetDocument();

	switch ( cmdId )
	{
		case CM_DROP_MOVE_IMAGE:
			pDoc->m_autoDropContext.m_dropOperation = auto_drop::CContext::FileMove;
			pDoc->ExecuteAutoDrop();
			break;
		case CM_DROP_COPY_IMAGE:
			pDoc->m_autoDropContext.m_dropOperation = auto_drop::CContext::FileCopy;
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
