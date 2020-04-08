
#include "stdafx.h"
#include "ImageView.h"
#include "INavigationBar.h"
#include "Workspace.h"
#include "MainFrame.h"
#include "MainToolbar.h"
#include "ChildFrame.h"
#include "ImageState.h"
#include "ImageDoc.h"
#include "SlideData.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/PostCall.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAccelTable CImageView::s_imageAccel;

IMPLEMENT_DYNCREATE( CImageView, CScrollView )

CImageView::CImageView( void )
	: BaseClass()
	, CObjectCtrlBase( this )
	, m_pNavigBar( app::GetMainFrame()->GetToolbar() )
	, m_pMdiChildFrame( NULL )
	, m_bkColor( CLR_DEFAULT )
{
	s_imageAccel.LoadOnce( IDR_IMAGEVIEW );
	SetTrackMenuTarget( app::GetMainFrame() );

	SetZoomBar( app::GetMainFrame()->GetToolbar() );
	SetScaleZoom( CWorkspace::GetData().m_autoImageSize, 100 );
}

CImageView::~CImageView()
{
}

HICON CImageView::GetDocTypeIcon( void ) const
{
	static HICON hIconImage = AfxGetApp()->LoadIcon( IDR_IMAGETYPE );
	return hIconImage;
}

CMenu& CImageView::GetDocContextMenu( void ) const
{
	static CMenu contextMenu;
	if ( NULL == (HMENU)contextMenu )
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::ImagePopup );
	return contextMenu;
}

CImageDoc* CImageView::GetDocument( void ) const
{
	return checked_static_cast< CImageDoc* >( m_pDocument );
}

COLORREF CImageView::GetBkColor( void ) const
{
	return m_bkColor != CLR_DEFAULT ? m_bkColor : CWorkspace::GetData().m_defBkColor;
}

void CImageView::SetBkColor( COLORREF bkColor, bool doRedraw /*= true*/ )
{
	m_bkColor = bkColor;
	if ( doRedraw && m_hWnd != NULL )
		Invalidate();
}

const fs::ImagePathKey& CImageView::GetImagePathKey( void ) const
{
	return GetDocument()->m_imagePathKey;
}

CWicImage* CImageView::GetImage( void ) const
{
	return GetDocument()->GetImage();
}

CScrollView* CImageView::GetView( void )
{
	return this;
}

void CImageView::RegainFocus( RegainAction regainAction, int ctrlId /*= 0*/ )
{
	switch ( regainAction )
	{
		case Enter:
			if ( IDW_ZOOM_COMBO == ctrlId )				// ENTER key pressed with the zoom combo focused?
				if ( InputZoomPct( ui::ByEdit ) )		// successful zoom input?
					SetFocus();
			break;
		case Escape:
			SetFocus();
			break;
	}
}

void CImageView::EventChildFrameActivated( void )
{
	// called when this view or a sibling view is activated (i.e. CAlbumThumbListView)
	OutputAutoSize();
	OutputZoomPct();
	OutputNavigSlider();
}

void CImageView::EventNavigSliderPosChanged( bool thumbTracking )
{
	thumbTracking;
}

CImageState* CImageView::GetLoadingImageState( void ) const
{
	return CWorkspace::Instance().GetLoadingImageState();
}

void CImageView::MakeImageState( CImageState* pImageState ) const
{
	// read current view status, and returns true in order to save this status, otherwise false.
	ASSERT_PTR( pImageState );

	pImageState->SetDocFilePath( m_pDocument->GetPathName().GetString() );
	pImageState->RefFramePlacement().GetPlacement( m_pMdiChildFrame );

	pImageState->m_polyFlags = 0;
	pImageState->m_autoImageSize = GetAutoImageSize();
	pImageState->m_zoomPct = GetZoomPct();
	pImageState->m_bkColor = m_bkColor;			// raw color

	if ( AnyScrollBar() )
		pImageState->m_scrollPos = GetScrollPosition();
}

void CImageView::RestoreState( const CImageState& loadingImageState )
{
	if ( !HasFlag( loadingImageState.m_polyFlags, CImageState::IgnorePlacement ) )
		loadingImageState.GetFramePlacement().SetPlacement( m_pMdiChildFrame );

	SetScaleZoom( loadingImageState.m_autoImageSize, loadingImageState.m_zoomPct );
	m_bkColor = loadingImageState.m_bkColor;

	if ( loadingImageState.HasScrollPos() )
		ui::PostCall( this, &CScrollView::ScrollToPosition, loadingImageState.m_scrollPos );
}

bool CImageView::OutputNavigSlider( void )
{
	if ( !app::GetMainFrame()->IsViewActive( this ) )
		return false;

	m_pNavigBar->OutputNavigRange( 1 );
	m_pNavigBar->OutputNavigPos( 0 );
	return true;
}

void CImageView::OnImageContentChanged( void )
{
	// the following sequence of calls is critical with subtle side-effects
	SetupContentMetrics( false );

	if ( GetImage() != NULL )
		app::GetMainFrame()->ResizeViewToFit( this );				// first allow view to resize in order to fit with contents (resize to image aspect ratio)

	AssignAutoSize( CWorkspace::GetData().m_autoImageSize );		// TRICKY: switch to initial value without any recalculations
	Invalidate( TRUE );
}

CWicImage* CImageView::ForceReloadImage( void ) const
{
	CWicImageCache::Instance().DiscardFrames( GetImagePathKey().first );
	return GetImage();
}

bool CImageView::CanEnterDragMode( void ) const
{
	return true;
}

void CImageView::OnActivateView( BOOL bActivate, CView* pActivateView, CView* pDeactiveView )
{
	__super::OnActivateView( bActivate, pActivateView, pDeactiveView );

	if ( bActivate )
		if ( pDeactiveView == NULL || pDeactiveView == pActivateView || pDeactiveView->GetParentFrame() != m_pMdiChildFrame )
			EventChildFrameActivated();		// call only if is a frame activation, that is avoid calling when is a sibling view activation, as in case of CAlbumThumbListView for the CAlbumImageView derived
}

void CImageView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	UpdateViewHint hint = (UpdateViewHint)lHint;
	switch ( hint )
	{
		case Hint_FileListChanged:
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

BEGIN_MESSAGE_MAP( CImageView, BaseClass )
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEditCopy )
	ON_UPDATE_COMMAND_UI( IDW_NAV_SLIDER, OnUpdateNavSlider )
	ON_COMMAND_RANGE( CM_IMAGE_SIZE_AUTO_FIT_LARGE, CM_IMAGE_SIZE_USE_ZOOM_PCT, OnRadioAutoImageSize )
	ON_UPDATE_COMMAND_UI_RANGE( CM_IMAGE_SIZE_AUTO_FIT_LARGE, CM_IMAGE_SIZE_USE_ZOOM_PCT, OnUpdateAutoImageSize )
	ON_COMMAND( CM_ZOOM_NORMAL, CmZoomNormal )		// CM_ZOOM_NORMAL is kind of synonym with CM_IMAGE_SIZE_ACTUAL_SIZE, but with an image
	ON_COMMAND_RANGE( CM_ZOOM_IN, CM_ZOOM_OUT, CmZoom )
	ON_COMMAND( CM_RESIZE_VIEW_TO_FIT, CmResizeViewToFit )
	ON_COMMAND( CM_EDIT_BK_COLOR, CmEditBkColor )
	ON_COMMAND( CM_EXPLORE_IMAGE, CmExploreImage )
	ON_UPDATE_COMMAND_UI( CM_EXPLORE_IMAGE, OnUpdatePhysicalFileShellOperation )
	ON_UPDATE_COMMAND_UI( CM_MOVE_FILE, OnUpdateAnyFileShellOperation )
	ON_COMMAND_RANGE( CM_DELETE_FILE, CM_DELETE_FILE_NO_UNDO, CmDeleteFile )
	ON_COMMAND( CM_MOVE_FILE, CmMoveFile )
	ON_UPDATE_COMMAND_UI_RANGE( CM_DELETE_FILE, CM_DELETE_FILE_NO_UNDO, OnUpdatePhysicalFileShellOperation )
	ON_COMMAND_RANGE( CM_SCROLL_LEFT, CM_SCROLL_PAGE_DOWN, CmScroll )

	ON_CBN_SELCHANGE( IDW_AUTO_IMAGE_SIZE_COMBO, OnCBnSelChange_AutoImageSizeCombo )
	ON_CBN_SELCHANGE( IDW_ZOOM_COMBO, OnCBnSelChange_ZoomCombo )
	// standard printing
	ON_COMMAND( ID_FILE_PRINT, CScrollView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview )
END_MESSAGE_MAP()

int CImageView::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

	m_pMdiChildFrame = checked_static_cast< CChildFrame* >( GetParentFrame() );
	ASSERT_PTR( m_pMdiChildFrame );
	return 0;
}

void CImageView::OnInitialUpdate( void )
{
	__super::OnInitialUpdate();

	if ( HICON hIconType = GetDocTypeIcon() )
		m_pMdiChildFrame->SetIcon( hIconType, FALSE );

	OnImageContentChanged();

	if ( CImageState* pLoadingImageState = GetLoadingImageState() )
	{
		if ( app::GetMainFrame()->IsMdiRestored() )
			pLoadingImageState->RefFramePlacement().ChangeMaximizedShowCmd( SW_SHOWNORMAL );		// override saved maximized state if MDI child is currently resored

		RestoreState( *pLoadingImageState );		// restore persistent image view status
	}
}

void CImageView::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	pWnd;

	CMenu* pSrcPopupMenu = &GetDocContextMenu();
	if ( pSrcPopupMenu->GetSafeHmenu() != NULL )
	{
		fs::CFlexPath imagePath = GetImagePathKey().first;

		if ( imagePath.IsComplexPath() )
			DoTrackContextMenu( pSrcPopupMenu, screenPos );
		else if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, std::vector< fs::CPath >( 1, imagePath ) ) )
			DoTrackContextMenu( pContextPopup, screenPos );
	}
}

void CImageView::OnLButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnLButtonDown( mkFlags, point );

	SetFocus();
	if ( CWicImage* pImage = GetImage() )
		if ( ui::IsKeyPressed( VK_MENU ) )			// ALT is pressed: enter drag-drop
		{
			if ( CanEnterDragMode() )
				if ( GetContentRect().PtInRect( point ) )
					if ( pImage->IsValidFile() )
					{
						ole::CImagesDataSource dataSource;
						dataSource.CacheShellFilePath( pImage->GetImagePath() );

						CWicDibSection* pThumbBitmap = app::GetThumbnailer()->AcquireThumbnailNoThrow( pImage->GetImagePath() );
						dataSource.DragAndDropImages( pThumbBitmap, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK );
					}
		}
		else
			CZoomViewMouseTracker::Run( this, mkFlags, point );
}

void CImageView::OnLButtonDblClk( UINT mkFlags, CPoint point )
{
	__super::OnLButtonDblClk( mkFlags, point );

	if ( !GetContentRect().PtInRect( point ) )			// outside of image
		CmEditBkColor();
	else
	{	// zoom to defaults: stretch to fit / 100%
		switch ( GetAutoImageSize() )
		{
			case ui::AutoFitLargeOnly:
			case ui::AutoFitAll:
				SetScaleZoom( ui::ActualSize, 100 );
				break;
			default:
				ModifyAutoImageSize( ui::AutoFitLargeOnly );
		}
	}
}

void CImageView::OnMButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnMButtonDown( mkFlags, point );
	SetFocus();

	if ( GetImage() != NULL )
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
		return TRUE;		// message processed
	}
	return FALSE;		//__super::OnMouseWheel( mkFlags, zDelta, point );
}

void CImageView::OnEditCopy( void )
{
	std::tstring imageFileName;
	if ( CWicImage* pImage = GetImage() )
		imageFileName = pImage->GetImagePath().Get();

	CClipboard::CopyText( imageFileName, this );
}

void CImageView::OnUpdateEditCopy( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();

	pCmdUI->Enable( pImage != NULL && !pImage->GetImagePath().IsEmpty() );
}

void CImageView::OnUpdateNavSlider( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( false );
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

void CImageView::OnRadioAutoImageSize( UINT cmdId )
{
	ModifyAutoImageSize( static_cast< ui::AutoImageSize >( cmdId - CM_IMAGE_SIZE_AUTO_FIT_LARGE ) );
}

void CImageView::OnUpdateAutoImageSize( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, GetAutoImageSize() == static_cast< ui::AutoImageSize >( pCmdUI->m_nID - CM_IMAGE_SIZE_AUTO_FIT_LARGE ) );
}

void CImageView::CmZoomNormal( void )
{
	ModifyZoomPct( 100 );
}

void CImageView::CmZoom( UINT cmdId )
{
	ZoomRelative( CM_ZOOM_IN == cmdId ? ZoomIn : ZoomOut );
}

void CImageView::CmResizeViewToFit( void )
{
	if ( GetImage() != NULL )
		app::GetMainFrame()->ResizeViewToFit( this );				// first allow view to resize in order to fit with contents
}

void CImageView::CmEditBkColor( void )
{
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		SetBkColor( CLR_DEFAULT );
	else
	{
		COLORREF oldBkColor = m_bkColor;
		CColorDialog colorDialog( oldBkColor, CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR, this );
		if ( IDOK == colorDialog.DoModal() )
			if ( colorDialog.GetColor() != oldBkColor )
				SetBkColor( colorDialog.GetColor() );
	}
}

void CImageView::CmExploreImage( void )
{
	if ( CWicImage* pImage = GetImage() )
		if ( pImage->IsValidPhysicalFile() )
			shell::ExploreAndSelectFile( pImage->GetImagePath().GetPtr() );
}

void CImageView::CmDeleteFile( UINT cmdId )
{
	CWicImage* pImage = GetImage();
	if ( pImage != NULL && pImage->IsValidPhysicalFile() )
	{
		std::vector< std::tstring > filesToDelete( 1, pImage->GetImagePath().Get() );
		app::DeleteFiles( filesToDelete, CM_DELETE_FILE == cmdId );
	}
}

void CImageView::CmMoveFile( void )
{
	CWicImage* pImage = GetImage();
	if ( pImage != NULL && pImage->IsValidFile() )
	{
		std::vector< std::tstring > filesToMove( 1, pImage->GetImagePath().Get() );
		app::MoveFiles( filesToMove, this );
	}
}

void CImageView::OnUpdateAnyFileShellOperation( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();
	pCmdUI->Enable( pImage != NULL && pImage->IsValidFile( fs::Write ) );
}

void CImageView::OnUpdatePhysicalFileShellOperation( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();
	pCmdUI->Enable( pImage != NULL && pImage->IsValidPhysicalFile( fs::Write ) );
}

void CImageView::OnCBnSelChange_AutoImageSizeCombo( void )
{
	InputAutoSize();
}

void CImageView::OnCBnSelChange_ZoomCombo( void )
{
	InputZoomPct( ui::BySel );
}
