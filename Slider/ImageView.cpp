
#include "pch.h"
#include "ImageView.h"
#include "INavigationBar.h"
#include "ImageNavigator.h"
#include "Workspace.h"
#include "MainFrame.h"
#include "ChildFrame.h"
#include "ImageState.h"
#include "ImageDoc.h"
#include "SlideData.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/TextClipboard.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/PostCall.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/ToolbarButtons.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAccelTable CImageView::s_imageAccel;

IMPLEMENT_DYNCREATE( CImageView, CScrollView )

CImageView::CImageView( void )
	: TBaseClass()
	, CObjectCtrlBase( this )
	, m_imageFramePos( 0 )
	, m_bkColor( CLR_DEFAULT )
	, m_initialized( false )
	, m_pNavigBar( app::GetMainFrame()->GetNavigationBar() )
	, m_pMdiChildFrame( nullptr )
{
	s_imageAccel.LoadOnce( IDR_IMAGEVIEW );
	SetTrackMenuTarget( app::GetMainFrame() );

	m_minContentSize.cx = 145;		// avoid very small view sizes (for icons, etc)
	m_minContentSize.cy = 64;

	SetZoomBar( app::GetMainFrame()->GetZoomBar() );
	//SetZoomBar( app::GetMainFrame()->GetOldToolbar() );
	SetScaleZoom( CWorkspace::GetData().m_scalingMode, 100 );
	SetFlag( RefViewStatusFlags(), ui::FullScreen, CWorkspace::Instance().IsFullScreen() );			// copy the actual FullScreen status
}

CImageView::~CImageView()
{
}

HICON CImageView::GetDocTypeIcon( void ) const
{
	static HICON s_hIconImage = AfxGetApp()->LoadIcon( IDR_IMAGETYPE );
	return s_hIconImage;
}

CMenu& CImageView::GetDocContextMenu( void ) const
{
	static CMenu s_contextMenu;

	if ( nullptr == s_contextMenu.GetSafeHmenu() )
		ui::LoadPopupMenu( &s_contextMenu, IDR_CONTEXT_MENU, app::ImagePopup );

	return s_contextMenu;
}

CImageDoc* CImageView::GetDocument( void ) const
{
	return checked_static_cast<CImageDoc*>( m_pDocument );
}

ui::TDisplayColor CImageView::GetBkColor( void ) const implements(ui::IZoomView)
{
	return ui::GetFallbackColor( m_bkColor, CWorkspace::GetData().m_defBkColor.Evaluate() );
}

void CImageView::SetBkColor( COLORREF bkColor, bool doRedraw /*= true*/ )
{
	m_bkColor = bkColor;

	if ( doRedraw && m_hWnd != nullptr )
		Invalidate();
}

CWicImage* CImageView::GetImage( void ) const implements(ui::IImageZoomView, IImageView)
{
	return GetDocument()->GetImage( m_imageFramePos );
}

CWicImage* CImageView::QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const implements(ui::IImageZoomView)
{
	return __super::QueryImageFileDetails( rFileDetails );		// TODO: make CImageView aware of multi-frame images, with frame navigation
}

fs::TImagePathKey CImageView::GetImagePathKey( void ) const implements(IImageView)
{
	return fs::TImagePathKey( GetDocument()->GetImagePath(), m_imageFramePos );
}

CScrollView* CImageView::GetScrollView( void ) implements(IImageView)
{
	return TBaseClass::GetScrollView();
}

void CImageView::EventChildFrameActivated( void ) implements(IImageView)
{
	// called when this view or a sibling view is activated (i.e. CAlbumThumbListView)
	OutputScalingMode();
	OutputZoomPct();
	OutputNavigSlider();
}

void CImageView::HandleNavigSliderPosChanging( int newPos, bool thumbTracking )
{
	thumbTracking;
	CImageFrameNavigator navigator( GetImage() );

	if ( navigator.IsStaticMultiFrameImage() )
	{
		m_imageFramePos = newPos;
		OnImageContentChanged();
	}
}

CImageState* CImageView::GetLoadingImageState( void ) const
{
	return CWorkspace::Instance().RefLoadingImageState();
}

void CImageView::MakeImageState( CImageState* pImageState ) const
{
	// read current view status, and returns true in order to save this status, otherwise false.
	ASSERT_PTR( pImageState );

	pImageState->SetDocFilePath( m_pDocument->GetPathName().GetString() );
	pImageState->RefFramePlacement().ReadWnd( m_pMdiChildFrame );

	pImageState->m_polyFlags = 0;
	pImageState->m_scalingMode = GetScalingMode();
	pImageState->m_zoomPct = GetZoomPct();
	pImageState->m_bkColor = m_bkColor;			// raw color

	if ( AnyScrollBar() )
		pImageState->m_scrollPos = GetScrollPosition();
}

void CImageView::RestoreState( const CImageState& loadingImageState )
{
	if ( !HasFlag( loadingImageState.m_polyFlags, CImageState::IgnorePlacement ) )
		const_cast<CImageState&>( loadingImageState ).RefFramePlacement().CommitWnd( m_pMdiChildFrame );

	SetScaleZoom( loadingImageState.m_scalingMode, loadingImageState.m_zoomPct );
	m_bkColor = loadingImageState.m_bkColor;

	if ( loadingImageState.HasScrollPos() )
		ui::PostCall( this, &CScrollView::ScrollToPosition, loadingImageState.m_scrollPos );
}

bool CImageView::OutputNavigSlider( void )
{
	if ( !app::GetMainFrame()->IsViewActive( this ) )
		return false;

	CImageFrameNavigator navigator( GetImage() );

	if ( navigator.IsStaticMultiFrameImage() )
	{
		m_pNavigBar->OutputNavigRange( navigator.GetFrameCount() );
		m_pNavigBar->OutputNavigPos( m_imageFramePos );
	}
	else
	{
		m_pNavigBar->OutputNavigRange( 1 );
		m_pNavigBar->OutputNavigPos( 0 );
	}

	return true;
}

std::tstring CImageView::FormatTipText_NavigSliderCtrl( void ) const
{
	fs::TImagePathKey imgPathKey = GetImagePathKey();
	std::tstring tipText = imgPathKey.first.Get();
	CImageFrameNavigator navigator( GetImage() );

	if ( navigator.IsStaticMultiFrameImage() )
		tipText += str::Format( _T("  [frame %d of %d]"), m_imageFramePos + 1, navigator.GetFrameCount() );

	return tipText;
}

void CImageView::OnImageContentChanged( void )
{
	// the following sequence of calls is critical with subtle side-effects

	// BUG - Windows Vista+ with scroll bar window theme: CScrollView::UpdateBars() succeeds for the SB_HORZ (1st) bar, but not for SB_VERT (2nd) bar.
	//	workaround: force a WM_SIZE
	ui::RecalculateScrollBars( m_hWnd );
	//was: SetupContentMetrics( false );

	if ( !m_initialized && !HasViewStatusFlag( ui::FullScreen ) )
		if ( GetImage() != nullptr )
			app::GetMainFrame()->ResizeViewToFit( this );				// first allow view to resize in order to fit with contents (resize to image aspect ratio)

	AssignScalingMode( CWorkspace::GetData().m_scalingMode );			// TRICKY: switch to initial value without any recalculations
	Invalidate( TRUE );

	OutputNavigSlider();			// update the slider range and position (for multi-frame images)
}

CWicImage* CImageView::ForceReloadImage( void )
{
	CWicImageCache::Instance().DiscardFrames( GetImagePathKey().first );
	return GetImage();
}

bool CImageView::CanEnterDragMode( void ) const
{
	return true;
}

nav::Navigate CImageView::CmdToNavigate( UINT cmdId )
{
	switch ( cmdId )
	{
		default:
			ASSERT( false );
		case ID_NAVIG_SEEK_PREV:	return nav::Previous;
		case ID_NAVIG_SEEK_NEXT:	return nav::Next;
		case ID_NAVIG_SEEK_FIRST:	return nav::First;
		case ID_NAVIG_SEEK_LAST:	return nav::Last;
	}
}

void CImageView::OnActivateView( BOOL bActivate, CView* pActivateView, CView* pDeactiveView ) overrides(CView)
{
	__super::OnActivateView( bActivate, pActivateView, pDeactiveView );

	if ( bActivate )
		if ( pDeactiveView == nullptr || pDeactiveView == pActivateView || pDeactiveView->GetParentFrame() != m_pMdiChildFrame )
			EventChildFrameActivated();		// call only if is a frame activation, that is avoid calling when is a sibling view activation, as in case of CAlbumThumbListView for the CAlbumImageView derived
}

void CImageView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) overrides(CView)
{
	UpdateViewHint hint = (UpdateViewHint)lHint;
	switch ( hint )
	{
		case Hint_ToggleFullScreen:
			SetViewStatusFlag( ui::FullScreen, CWorkspace::Instance().IsFullScreen() );
			break;
		case Hint_AlbumModelChanged:
			break;
		case Hint_ReloadImage:			// this could be send by CApplication::UpdateAllViews()
			ForceReloadImage();
			break;
	}

	__super::OnUpdate( pSender, lHint, pHint );
}

BOOL CImageView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( HandleCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BOOL CImageView::PreTranslateMessage( MSG* pMsg )
{
	return
		s_imageAccel.Translate( pMsg, m_hWnd ) ||
		__super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CImageView, TBaseClass )
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEditCopy )

	ON_COMMAND_RANGE( ID_NAVIG_SEEK_PREV, ID_NAVIG_SEEK_LAST, On_NavigSeek )
	ON_UPDATE_COMMAND_UI_RANGE( ID_NAVIG_SEEK_PREV, ID_NAVIG_SEEK_LAST, OnUpdate_NavigSeek )

	ON_COMMAND_RANGE( ID_TOGGLE_SCALING_AUTO_FIT_LARGE, ID_TOGGLE_SCALING_USE_ZOOM_PCT, OnRadio_ImageScalingMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_TOGGLE_SCALING_AUTO_FIT_LARGE, ID_TOGGLE_SCALING_USE_ZOOM_PCT, OnUpdate_ImageScalingMode )

	ON_COMMAND( ID_ZOOM_NORMAL_100, On_ZoomNormal100 )		// ID_ZOOM_NORMAL_100 is kind of synonym with ID_TOGGLE_SCALE_ACTUAL_SIZE, but with an image
	ON_COMMAND_RANGE( ID_ZOOM_IN, ID_ZOOM_OUT, On_Zoom )
	ON_COMMAND( ID_RESIZE_VIEW_TO_FIT, CmResizeViewToFit )
	ON_COMMAND( ID_EDIT_BK_COLOR, On_EditBkColor )
	ON_COMMAND_RANGE( CM_SCROLL_LEFT, CM_SCROLL_PAGE_DOWN, CmScroll )

	ON_COMMAND( IDW_IMAGE_SCALING_COMBO, OnCBnSelChange_ImageScalingModeCombo )
	ON_CBN_SELENDOK( IDW_IMAGE_SCALING_COMBO, OnCBnSelChange_ImageScalingModeCombo )
	ON_BN_CLICKED( IDW_ZOOM_COMBO, OnEditInput_ZoomCombo )
	ON_CBN_SELENDOK( IDW_ZOOM_COMBO, OnCBnSelChange_ZoomCombo )
	ON_COMMAND( IDW_NAVIG_SLIDER_CTRL, OnPosChanged_NavigSliderCtrl )
	ON_NOTIFY( TRBN_THUMBPOSCHANGING, IDW_NAVIG_SLIDER_CTRL, OnThumbPosChanging_NavigSliderCtrl )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXT, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText_NavigSliderCtrl )
	ON_UPDATE_COMMAND_UI( IDW_NAVIG_SLIDER_CTRL, OnUpdate_NavigSliderCtrl )
	// standard printing
	ON_COMMAND( ID_FILE_PRINT, CScrollView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview )
END_MESSAGE_MAP()

int CImageView::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

	m_pMdiChildFrame = checked_static_cast<CChildFrame*>( GetParentFrame() );
	ASSERT_PTR( m_pMdiChildFrame );
	return 0;
}

void CImageView::OnInitialUpdate( void )
{
	__super::OnInitialUpdate();

	if ( HICON hIconType = GetDocTypeIcon() )
		m_pMdiChildFrame->SetIcon( hIconType, FALSE );

	m_initialized = false;
	OnImageContentChanged();

	if ( CImageState* pLoadingImageState = GetLoadingImageState() )
	{
		if ( app::GetMainFrame()->IsMdiRestored() )
			pLoadingImageState->RefFramePlacement().ChangeMaximizedShowCmd( SW_SHOWNORMAL );		// override saved maximized state if MDI child is currently resored

		RestoreState( *pLoadingImageState );		// restore persistent image view status
	}
	m_initialized = true;
}

void CImageView::OnSetFocus( CWnd* pOldWnd )
{
	__super::OnSetFocus( pOldWnd );

	Invalidate( TRUE );			// highlight when gaining focus
}

void CImageView::OnKillFocus( CWnd* pNewWnd )
{
	__super::OnSetFocus( pNewWnd );

	Invalidate( TRUE );			// revert background when losing of focus
}

void CImageView::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		if ( CMenu* pSrcPopupMenu = &GetDocContextMenu() )
		{
			//ui::CScopedTrackMfcPopupMenu scTrackMfcPopup;

			if ( CWicImage* pImage = GetImage() )
				if ( !pImage->GetImagePath().IsComplexPath() )
					if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, pImage->GetImagePath() ) )
					{
						DoTrackContextMenu( pContextPopup, screenPos );
						return;
					}

			DoTrackContextMenu( pSrcPopupMenu, screenPos );
		}
	}
	else
		__super::OnContextMenu( pWnd, screenPos );
}

void CImageView::OnLButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnLButtonDown( mkFlags, point );

	SetFocus();

	if ( CWicImage* pImage = GetImage() )
		if ( ui::IsKeyPressed( VK_MENU ) )			// ALT is pressed: enter drag-drop
		{
			if ( CanEnterDragMode() )
				if ( InContentRect( point ) )
					if ( pImage->IsValidFile() )
					{
						ole::CImagesDataSource dataSource;
						dataSource.CacheShellFilePath( pImage->GetImagePath() );

						CWicDibSection* pThumbBitmap = app::GetThumbnailer()->AcquireThumbnailNoThrow( pImage->GetImagePath() );
						dataSource.DragAndDropImages( pThumbBitmap, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK );
					}
		}
		else if ( InContentRect( point ) )
			CZoomViewMouseTracker::Run( this, mkFlags, point );
}

void CImageView::OnLButtonDblClk( UINT mkFlags, CPoint point )
{
	__super::OnLButtonDblClk( mkFlags, point );

	if ( InBackgroundRect( point ) )			// outside of image
		On_EditBkColor();
	else if ( ui::IsKeyPressed( VK_CONTROL ) )
	{
		const fs::CFlexPath& filePath = GetImagePathKey().first;
		if ( !filePath.IsComplexPath() )
			ShellInvokeDefaultVerb( std::vector<fs::CPath>( 1, filePath ) );
	}
	else
	{	// zoom to defaults: stretch to fit / 100%
		switch ( GetScalingMode() )
		{
			case ui::AutoFitLargeOnly:
			case ui::AutoFitAll:
				SetScaleZoom( ui::ActualSize, 100 );
				break;
			default:
				ModifyScalingMode( ui::AutoFitLargeOnly );
		}
	}
}

void CImageView::OnMButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnMButtonDown( mkFlags, point );
	SetFocus();

	if ( GetImage() != nullptr && InContentRect( point ) )
		CZoomViewMouseTracker::Run( this, mkFlags, point );
}

void CImageView::OnRButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnRButtonDown( mkFlags, point );
	SetFocus();
}

BOOL CImageView::OnMouseWheel( UINT mkFlags, short zDelta, CPoint point )
{
	point;
	if ( HasFlag( mkFlags, MK_CONTROL ) )
	{
		int delta = -( zDelta / WHEEL_DELTA );
		if ( delta != 0 )
			ZoomRelative( delta > 0 ? ZoomOut : ZoomIn );
		return TRUE;	// message processed
	}
	return FALSE;		//__super::OnMouseWheel( mkFlags, zDelta, point );
}

void CImageView::OnEditCopy( void )
{
	std::tstring imageFileName;
	if ( CWicImage* pImage = GetImage() )
		imageFileName = pImage->GetImagePath().Get();

	CTextClipboard::CopyText( imageFileName, m_hWnd );
}

void CImageView::OnUpdateEditCopy( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();

	pCmdUI->Enable( pImage != nullptr && !pImage->GetImagePath().IsEmpty() );
}

void CImageView::On_NavigSeek( UINT cmdId )
{
	CImageFrameNavigator navigator( GetImage() );
	nav::Navigate navigate = CmdToNavigate( cmdId );

	m_imageFramePos = navigator.GetNavigateFramePos( navigate );
	OnImageContentChanged();
}

void CImageView::OnUpdate_NavigSeek( CCmdUI* pCmdUI )
{
	CImageFrameNavigator navigator( GetImage() );
	nav::Navigate navigate = CmdToNavigate( pCmdUI->m_nID );

	pCmdUI->Enable( navigator.CanNavigate( navigate ) );
}

void CImageView::OnPosChanged_NavigSliderCtrl( void )	// OBSOLETE
{
}

void CImageView::OnUpdate_NavigSliderCtrl( CCmdUI* pCmdUI )
{
	CImageFrameNavigator navigator( GetImage() );

	pCmdUI->Enable( navigator.IsStaticMultiFrameImage() );
}

void CImageView::OnThumbPosChanging_NavigSliderCtrl( NMHDR* pNmHdr, LRESULT* pResult )
{
	const NMTRBTHUMBPOSCHANGING* pNmPosChanging = (NMTRBTHUMBPOSCHANGING*)pNmHdr;

	if ( pNmHdr->hwndFrom != nullptr )
		if ( const CSliderCtrl* pSliderCtrl = checked_static_cast<const CSliderCtrl*>( CWnd::FromHandlePermanent( pNmHdr->hwndFrom ) ) )
		{
			int newPos = static_cast<int>( pNmPosChanging->dwPos );
			if ( ui::ValidatePos( pSliderCtrl, &newPos ) )
			{
				HandleNavigSliderPosChanging( static_cast<int>( pNmPosChanging->dwPos ), TB_THUMBTRACK == pNmPosChanging->nReason );
				*pResult = 0;		// enable input
				return;
			}
		}

	*pResult = 1;		// reject input
}

BOOL CImageView::OnTtnNeedText_NavigSliderCtrl( UINT, NMHDR* pNmHdr, LRESULT* pResult )
{
	ui::CTooltipTextMessage message( pNmHdr );

	if ( !message.IsValidNotification() || message.m_cmdId != IDW_NAVIG_SLIDER_CTRL )
		return FALSE;		// not handled, countinue routing

	if ( !message.AssignTooltipText( FormatTipText_NavigSliderCtrl() ) )
		return FALSE;		// countinue routing

	*pResult = 0;
	return TRUE;			// message was handled
}

void CImageView::OnRadio_ImageScalingMode( UINT cmdId )
{
	ModifyScalingMode( static_cast<ui::ImageScalingMode>( cmdId - ID_TOGGLE_SCALING_AUTO_FIT_LARGE ) );
}

void CImageView::OnUpdate_ImageScalingMode( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, GetScalingMode() == static_cast<ui::ImageScalingMode>( pCmdUI->m_nID - ID_TOGGLE_SCALING_AUTO_FIT_LARGE ) );
}

void CImageView::On_ZoomNormal100( void )
{
	ModifyZoomPct( 100 );
}

void CImageView::On_Zoom( UINT cmdId )
{
	ZoomRelative( ID_ZOOM_IN == cmdId ? ZoomIn : ZoomOut );
}

void CImageView::CmResizeViewToFit( void )
{
	if ( GetImage() != nullptr )
		app::GetMainFrame()->ResizeViewToFit( this );				// first allow view to resize in order to fit with contents
}

void CImageView::On_EditBkColor( void )
{
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		SetBkColor( CLR_DEFAULT );
	else
	{
		COLORREF bkColor = GetBkColor(), oldBkColor = bkColor;

		if ( ui::EditColor( &bkColor, this, false ) )
			if ( bkColor != oldBkColor )
				SetBkColor( bkColor );
	}
}

void CImageView::CmScroll( UINT cmdId )
{
	switch ( cmdId )
	{
		case CM_SCROLL_LEFT:		OnScroll( MAKEWORD( SB_LINELEFT, -1 ), 0 ); break;
		case CM_SCROLL_RIGHT:		OnScroll( MAKEWORD( SB_LINERIGHT, -1 ), 0 ); break;
		case CM_SCROLL_UP:			OnScroll( MAKEWORD( -1, SB_LINEUP ), 0 ); break;
		case CM_SCROLL_DOWN:		OnScroll( MAKEWORD( -1, SB_LINEDOWN ), 0 ); break;

		case CM_SCROLL_PAGE_LEFT:	OnScroll( MAKEWORD( SB_PAGELEFT, -1 ), 0 ); break;
		case CM_SCROLL_PAGE_RIGHT:	OnScroll( MAKEWORD( SB_PAGERIGHT, -1 ), 0 ); break;
		case CM_SCROLL_PAGE_UP:		OnScroll( MAKEWORD( -1, SB_PAGEUP ), 0 ); break;
		case CM_SCROLL_PAGE_DOWN:	OnScroll( MAKEWORD( -1, SB_PAGEDOWN ), 0 ); break;
	}
}

void CImageView::OnCBnSelChange_ImageScalingModeCombo( void )
{
	InputScalingMode();
}

void CImageView::OnEditInput_ZoomCombo( void )
{
	InputZoomPct( ui::ByEdit );
}

void CImageView::OnCBnSelChange_ZoomCombo( void )
{
	InputZoomPct( ui::BySel );
}
