
#include "stdafx.h"
#include "AlbumThumbListView.h"
#include "Workspace.h"
#include "MainFrame.h"
#include "AlbumDoc.h"
#include "AlbumImageView.h"
#include "FileAttrAlgorithms.h"
#include "SplitterWindow.h"
#include "FileListDisplayPaths.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/DragListCtrl.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/Resequence.hxx"
#include <memory>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace dbg
{
	std::tstring FormatSelectedItems( const CListBox* pListBox )
	{
		std::vector< int > selIndexes;
		if ( HasFlag( pListBox->GetStyle(), LBS_EXTENDEDSEL | LBS_MULTIPLESEL ) )		// multi-selection list
		{
			if ( int selCount = pListBox->GetSelCount() )
			{
				selIndexes.resize( selCount, -1 );
				pListBox->GetSelItems( selCount, &selIndexes.front() );
			}
		}
		else
		{
			int selIndex = pListBox->GetCurSel();
			if ( selIndex != LB_ERR )
				selIndexes.push_back( selIndex );
		}
		return str::FormatSet( selIndexes );
	}
}


// CAlbumThumbListView implementation

CFont CAlbumThumbListView::s_fontCaption;
int CAlbumThumbListView::s_fontHeight = 0;

// init according to CWorkspace::Instance().m_thumbListColCount, which is 1 by default
DWORD CAlbumThumbListView::s_listCreationStyle = WS_VSCROLL | LBS_DISABLENOSCROLL;
CSize CAlbumThumbListView::scrollTimerDivider( 3, 2 );

IMPLEMENT_DYNCREATE( CAlbumThumbListView, CCtrlView )

CAlbumThumbListView::CAlbumThumbListView( void )
	: CCtrlView( _T("LISTBOX"), AFX_WS_DEFAULT_VIEW )
	, CObjectCtrlBase( this )
	, m_autoDelete( true )
	, m_pAlbumModel( NULL )
	, m_pPeerImageView( NULL )
	, m_pSplitterWnd( NULL )
	, m_beginDragTimer( this, ID_BEGIN_DRAG_TIMER, 350 )
	, m_userChangeSel( 0 )
	, m_startDragRect( 0, 0, 0, 0 )
	, m_scrollTimerCounter( 0, 0 )
	, m_selectionBackup( StoreByString )
{
	SetTrackMenuTarget( app::GetMainFrame() );

	EnsureCaptionFontCreated();
}

CAlbumThumbListView::~CAlbumThumbListView()
{
}

void CAlbumThumbListView::StorePeerView( CAlbumImageView* pPeerImageView )
{
	// called just after creation, before initial update
	ASSERT_NULL( m_pPeerImageView );
	m_pPeerImageView = pPeerImageView;
	m_pSplitterWnd = checked_static_cast< CSplitterWindow* >( GetParent() );
}

CAlbumDoc* CAlbumThumbListView::GetAlbumDoc( void ) const
{
	return m_pPeerImageView->GetDocument();
}

void CAlbumThumbListView::SetupAlbumModel( const CAlbumModel* pAlbumModel, bool doRedraw /*= true*/ )
{
	CListBox* pListBox = AsListBox();
	size_t countOld = pListBox->GetCount();
	size_t countNew = pAlbumModel != NULL ? pAlbumModel->GetFileAttrCount() : 0;
	bool doSmartUpdate = ( pAlbumModel == m_pAlbumModel && countOld > 0 && countNew > 0 );

	m_pAlbumModel = pAlbumModel;

	SetRedraw( FALSE );

	// The thumb list has no real content!
	// The content is actually referred from m_pAlbumModel member using the item index (display index)
	if ( !doSmartUpdate )
	{
		pListBox->ResetContent();
		countOld = 0;
	}

	// remove extra trailing items
	while ( countOld > countNew )
		pListBox->DeleteString( static_cast< UINT >( --countOld ) );

	// append missing items
	for ( ; countOld < countNew; ++countOld )
		pListBox->AddString( _T("") );

	ASSERT( pListBox->GetCount() == static_cast< int >( m_pAlbumModel != NULL ? m_pAlbumModel->GetFileAttrCount() : 0 ) );

	SetRedraw( TRUE );
	if ( doRedraw )
	{
		Invalidate();
#ifdef DBG_INSTANT_UPDATE
		UpdateWindow();
#endif // DBG_INSTANT_UPDATE
	}
}

int CAlbumThumbListView::GetCurSel( void ) const
{
	CListBox* pListBox = AsListBox();

	if ( IsMultiSelection() )
	{	// Multiple selection
		int caretIndex = pListBox->GetCaretIndex();

		if ( caretIndex != LB_ERR )
			return caretIndex;						// caret takes precedence
		else if ( pListBox->GetSelCount() > 0 )
		{
			int firstSelIndex = LB_ERR;

			pListBox->GetSelItems( 1, &firstSelIndex );
			return firstSelIndex;					// return the first selected item as the current selected
		}
		else
			return 0;
	}
	else
		return pListBox->GetCurSel();				// single selection case
}

bool CAlbumThumbListView::SetCurSel( int selIndex, bool notifySelChanged /*= false*/ )
{
	if ( m_userChangeSel != 0 )
		return false;			// don't change selection (this is a user selection change)

	CListViewState singleSelState( StoreByIndex );

	singleSelState.m_pIndexImpl->m_selItems.push_back( selIndex );
	singleSelState.m_pIndexImpl->m_caret = selIndex;
	SetListViewState( singleSelState, notifySelChanged, _T("SC") );
	return true;
}

bool CAlbumThumbListView::QuerySelItemPaths( std::vector< fs::CFlexPath >& rSelFilePaths ) const
{
	const CListBox* pListBox = AsListBox();

	rSelFilePaths.clear();

	if ( IsMultiSelection() )
	{
		if ( int selCount = pListBox->GetSelCount() )
		{
			std::vector< int > selIndexes;
			selIndexes.resize( selCount );
			pListBox->GetSelItems( selCount, &selIndexes.front() );

			for ( std::vector< int >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
				if ( const fs::CFlexPath* pFlexPath = GetItemPath( *itSelIndex ) )
					rSelFilePaths.push_back( *pFlexPath );
		}
	}
	else
	{
		if ( const fs::CFlexPath* pFlexPath = GetItemPath( GetCurSel() ) )
			rSelFilePaths.push_back( *pFlexPath );
	}

	return !rSelFilePaths.empty();
}

// fetch the current list box lvState (selection, top, caret) into destination
void CAlbumThumbListView::GetListViewState( CListViewState& rLvState, bool filesMustExist /*= true*/, bool sortAscending /*= true*/ ) const
{
	const CListBox* pListBox = AsListBox();
	CFileListDisplayPaths displayPaths( *m_pAlbumModel, filesMustExist );

	// work on an index state for simplicity
	std::auto_ptr< CListViewState::CImpl< int > > pIndexState( new CListViewState::CImpl< int > );
	pIndexState->m_top = displayPaths.GetPos( pListBox->GetTopIndex() );

	if ( IsMultiSelection() )
	{
		pIndexState->m_caret = displayPaths.GetPos( pListBox->GetCaretIndex() );

		if ( int selCount = pListBox->GetSelCount() )
		{
			pIndexState->m_selItems.resize( selCount );
			if ( LB_ERR == pListBox->GetSelItems( selCount, &pIndexState->m_selItems.front() ) )
				ASSERT( false );
			if ( sortAscending )
				std::sort( pIndexState->m_selItems.begin(), pIndexState->m_selItems.end() );		// sort the selected indexes ascending
		}
	}
	else
		pIndexState->m_selItems.push_back( pListBox->GetCurSel() );					// single selection list

	displayPaths.SetListState( rLvState, pIndexState );								// assign to required state
}

// Sets the current list box lvState (selection, top, caret) from source selection data
// pDoRestore argument specifies what and in which order to restore from list box lvState
void CAlbumThumbListView::SetListViewState( const CListViewState& lvState, bool notifySelChanged /*= false*/, const TCHAR* pDoRestore /*= _T("TSC")*/ )
{
	ASSERT( lvState.IsConsistent() );

	CListBox* pListBox = AsListBox();

	// work on an index state for simplicity
	std::auto_ptr< CListViewState::CImpl< int > > pIndexState( CFileListDisplayPaths::MakeIndexState( lvState, *m_pAlbumModel ) );

	for ( ; *pDoRestore != _T('\0'); ++pDoRestore )
	{
		switch ( *pDoRestore )
		{
			case _T('S'):
			{
				if ( IsMultiSelection() )
				{
					pListBox->SelItemRange( false, 0, pListBox->GetCount() - 1 );		// clear the selection

					for ( size_t i = pIndexState->m_selItems.size(); i-- != 0; )
						pListBox->SetSel( pIndexState->m_selItems[ i ] );
				}
				else
					pListBox->SetCurSel( 1 == pIndexState->m_selItems.size() ? pIndexState->m_selItems.front() : LB_ERR );		// single selection list
				break;
			}
			case _T('T'):
				if ( pIndexState->m_top != LB_ERR )
					pListBox->SetTopIndex( pIndexState->m_top );
				break;
			case _T('C'):
				if ( pIndexState->m_caret != LB_ERR )
					pListBox->SetCaretIndex( pIndexState->m_caret, LB_ERR == pIndexState->m_top );
				break;
			default:
				ASSERT( false );
				break;
		}
#ifdef DBG_INSTANT_UPDATE
		UpdateWindow();
#endif // DBG_INSTANT_UPDATE
	}

	if ( notifySelChanged )
		NotifySelChange();			// update the current index in the album view
}

void CAlbumThumbListView::SelectAll( bool notifySelChanged /*= false*/ )
{
	notifySelChanged;

	CListBox* pListBox = AsListBox();
	int itemCount = pListBox->GetCount();

	if ( itemCount > 0 )
	{
		CListViewState lvState( StoreByIndex );

		GetListViewState( lvState, false, false );
		pListBox->SelItemRange( TRUE, 0, itemCount - 1 );
		SetListViewState( lvState, false, _T("TC") );		// Restore only top and caret
	}
}

bool CAlbumThumbListView::NotifySelChange( void )
{
	if ( m_userChangeSel )
		return false;			// avoid LBN_SELCHANGE simulation if selection changed by the user

	::SendMessage( ::GetParent( m_hWnd ), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(), LBN_SELCHANGE ), (LPARAM)m_hWnd );
	return true;
}

bool CAlbumThumbListView::BackupSelection( bool currentSelection /*= true*/ )
{
	bool hasSel = false;

	m_selectionBackup.Clear();
	if ( currentSelection )
	{	// backup the CURRENT selection lvState
		GetListViewState( m_selectionBackup );
		if ( !m_selectionBackup.IsEmpty() )
			hasSel = true;
		else if ( m_pAlbumModel->AnyFoundFiles() )
			m_selectionBackup.m_pStringImpl->m_selItems.push_back( m_pAlbumModel->GetFileAttr( 0 )->GetPath().Get() ), hasSel = true;
	}
	else
	{	// backup the OUTER selection lvState
		CListViewState indexesState( StoreByIndex );
		std::vector< int >& selIndexes = indexesState.m_pIndexImpl->m_selItems;
		size_t nextSelIndex = 0;

		GetListViewState( indexesState );

		if ( indexesState.IsEmpty() )
		{	// no selection: assume first item selected
			if ( m_pAlbumModel->GetFileAttrCount() > 0 && m_pAlbumModel->GetFileAttr( 0 )->IsValid() )
				m_selectionBackup.m_pStringImpl->m_selItems.push_back( m_pAlbumModel->GetFileAttr( 0 )->GetPath().Get() ), hasSel = true;
		}
		else
		{	// search for a valid file next to selection
			for ( nextSelIndex = selIndexes.back() + 1; !hasSel && nextSelIndex < m_pAlbumModel->GetFileAttrCount(); ++nextSelIndex )
				if ( m_pAlbumModel->GetFileAttr( nextSelIndex )->IsValid() )
					m_selectionBackup.m_pStringImpl->m_selItems.push_back( m_pAlbumModel->GetFileAttr( nextSelIndex )->GetPath().Get() ), hasSel = true;

			// Search for a valid file prior to selection
			for ( nextSelIndex = selIndexes.front() - 1; !hasSel && nextSelIndex >= 0; --nextSelIndex )
				if ( m_pAlbumModel->GetFileAttr( nextSelIndex )->IsValid() )
					m_selectionBackup.m_pStringImpl->m_selItems.push_back( m_pAlbumModel->GetFileAttr( nextSelIndex )->GetPath().Get() ), hasSel = true;
		}

		// set caret to the selected item (outer selection)
		if ( hasSel )
			m_selectionBackup.m_pStringImpl->m_caret = m_selectionBackup.m_pStringImpl->m_selItems.front();

		// copy top from index to string
		if ( indexesState.m_pIndexImpl->m_top != -1 )
			m_selectionBackup.m_pStringImpl->m_top = m_pAlbumModel->GetFileAttr( indexesState.m_pIndexImpl->m_top )->GetPath().Get();
	}

	TRACE( _T(" ** Backup %s Selection for: %s **\nm_selectionBackup=%s\n"),
		currentSelection ? _T("CURRENT") : _T("OUTER"), ui::GetWindowText( GetParentFrame()->m_hWnd ).c_str(), m_selectionBackup.dbgFormat().c_str() );

	return hasSel;
}

void CAlbumThumbListView::RestoreSelection( void )
{
	// Last chance: select the first file if no selection backup
	if ( m_selectionBackup.IsEmpty() )
		if ( m_pAlbumModel->AnyFoundFiles() )
			m_selectionBackup.m_pStringImpl->m_selItems.push_back( m_pAlbumModel->GetFileAttr( 0 )->GetPath().Get() );

	TRACE( _T(" ** Restore Selection for frame: %s **\nm_selectionBackup=%s\n"),
		ui::GetWindowText( GetParentFrame()->m_hWnd ).c_str(), m_selectionBackup.dbgFormat().c_str() );

	SetListViewState( m_selectionBackup, true );
	m_selectionBackup.Clear();
}

// Returns true when some of the display indexes specified overlaps with some of the current selected files
// Typically called to decide whether to use current or near selection backup on Hint_SmartBackupSelection notification
//
bool CAlbumThumbListView::SelectionOverlapsWith( const std::vector< int >& displayIndexes ) const
{
	CListViewState currSelection( StoreByString );
	GetListViewState( currSelection );

	if ( !currSelection.IsEmpty() && !displayIndexes.empty() )
		for ( size_t i = 0; i != displayIndexes.size(); ++i )
			if ( utl::Contains( currSelection.m_pStringImpl->m_selItems, m_pAlbumModel->GetFileAttr( displayIndexes[ i ] )->GetPath().Get() ) )
				return true;

	return false;
}

int CAlbumThumbListView::GetImageIndexFromPoint( CPoint& clientPos ) const
{
	BOOL isOutside = TRUE;
	int hitIndex = AsListBox()->ItemFromPoint( clientPos, isOutside );

	if ( isOutside )
		hitIndex = LB_ERR;
	return hitIndex;
}

bool CAlbumThumbListView::IsValidFileAt( size_t displayIndex ) const
{
	if ( !IsValidImageIndex( displayIndex ) )
		return false;

	return m_pAlbumModel->GetFileAttr( displayIndex )->GetPath().FileExist();
}

int CAlbumThumbListView::GetPointedImageIndex( void ) const
{
	CPoint pos;
	GetCursorPos( &pos );
	ScreenToClient( &pos );
	return GetImageIndexFromPoint( pos );
}

void CAlbumThumbListView::MeasureItem( MEASUREITEMSTRUCT* pMIS )
{
	CSize initSize = GetInitialSize();

	pMIS->itemWidth = initSize.cx;
	pMIS->itemHeight = initSize.cy;

	// handle the case of variable height
	if ( HasFlag( GetStyle(), LBS_OWNERDRAWVARIABLE ) )
	{
		if ( (int)pMIS->itemID < 0 )
			return;		// it could happen during transient draw

		if ( CWicDibSection* pThumbDib = GetItemThumb( pMIS->itemID ) )
			pMIS->itemHeight = pThumbDib->GetBmpFmt().m_size.cy + cyTop + 2 * cyTextSpace + s_fontHeight;
	}
}

void CAlbumThumbListView::DrawItem( DRAWITEMSTRUCT* pDIS )
{
	if ( (int)pDIS->itemID < 0 )
		return;		// it could happen during transient draw

	CRect itemRect( pDIS->rcItem );

	switch ( pDIS->itemAction )
	{
		case ODA_DRAWENTIRE:
		case ODA_SELECT:
		{
			const fs::CFlexPath* pFilePath = GetItemPath( pDIS->itemID );
			bool validFile = pFilePath->FileExist();
			CWicDibSection* pThumbDib = GetItemThumb( pDIS->itemID );

			CDC dc;
			CRgn bkRegion;
			CSize thumbSize = pThumbDib != NULL ? pThumbDib->GetBmpFmt().m_size : CSize( 0, 0 );
			CRect thumbRect( CPoint( 0, 0 ), thumbSize ), thumbZoneRect( pDIS->rcItem );

			dc.Attach( pDIS->hDC );
			thumbZoneRect.bottom -= 2 * cyTextSpace + s_fontHeight;

			if ( pThumbDib != NULL )
			{
				ui::CenterRect( thumbRect, thumbZoneRect, true, true, false, CSize( 0, 1 ) );
				ui::CombineRects( &bkRegion, pDIS->rcItem, thumbRect, RGN_DIFF );
			}
			else
				bkRegion.CreateRectRgnIndirect( &pDIS->rcItem );		// no thumb image, fill the background anyway

			if ( HasFlag( pDIS->itemState, ODS_SELECTED ) )
			{
				FillRgn( dc, bkRegion, CWorkspace::Instance().GetImageSelColorBrush() );
				itemRect.DeflateRect( 1, 1 );
				::FrameRect( dc, &itemRect, GetSysColorBrush( COLOR_WINDOW ) );
			}
			else
			{
				CBrush bkBrush( validFile ? ::GetSysColor( COLOR_BTNFACE ) : app::ColorErrorBk );
				FillRgn( dc, bkRegion, bkBrush );
			}

			if ( ODA_DRAWENTIRE == pDIS->itemAction )
				if ( pThumbDib != NULL )
					pThumbDib->DrawAtPos( &dc, thumbRect.TopLeft() );

			const TCHAR* pFileName = pFilePath->GetNameExt();
			CRect rectText( pDIS->rcItem );

			rectText.top = rectText.bottom - ( 2 * cyTextSpace + s_fontHeight + 2 );
			rectText.DeflateRect( cxSide, cyTextSpace );
			rectText.OffsetRect( 1, -1 );		// Extra step: fine adjust text position

			HGDIOBJ hFontOld = SelectObject( dc, HGDIOBJ( s_fontCaption ) );
			COLORREF textColorOld;
			int bkModeOld = dc.SetBkMode( TRANSPARENT );

			if ( validFile )
				if ( HasFlag( pDIS->itemState, ODS_SELECTED ) )
					textColorOld = dc.SetTextColor( CWorkspace::Instance().GetImageSelTextColor() );
				else
					textColorOld = dc.SetTextColor( GetSysColor( COLOR_BTNTEXT ) );
			else
				textColorOld = dc.SetTextColor( app::ColorErrorText );

			dc.DrawText( pFileName, -1, rectText, DT_CENTER | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );

			SelectObject( dc, hFontOld );
			dc.SetTextColor( textColorOld );
			dc.SetBkMode( bkModeOld );

//#define DRAW_PERFECT_FRAME

#ifdef	_DEBUG
#	ifdef DRAW_PERFECT_FRAME
			// DEBUG: draw the "perfect" frame
			CRect perfectRect( pDIS->rcItem );
			CBrush perfectBrush( RGB( 255, 0, 0 ) );

			perfectRect.right = perfectRect.left + GetInitialSize().cx;
			::FrameRect( dc, &perfectRect, perfectBrush );
#	endif // DRAW_PERFECT_FRAME
#endif	// _DEBUG

			dc.Detach();
			break;
		}
		case ODA_FOCUS:
		{
			DrawFocusRect( pDIS->hDC, &itemRect );
			itemRect.DeflateRect( 1, 1 );
			DrawFocusRect( pDIS->hDC, &itemRect );
			break;
		}
	}
}

CMenu& CAlbumThumbListView::GetContextMenu( void )
{
	static CMenu contextMenu;
	if ( NULL == (HMENU)contextMenu )
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AlbumThumbsPopup );
	return contextMenu;
}

CSize CAlbumThumbListView::GetInitialSize( int columnCount /*= 1*/ )
{
	EnsureCaptionFontCreated();

	CSize initSize = app::GetThumbnailer()->GetBoundsSize();
	initSize.cx += 2 * cxSide;
	initSize.cy += cyTop + 2 * cyTextSpace + s_fontHeight;

	initSize.cx *= columnCount;
	return initSize;
}

CRect CAlbumThumbListView::GetListWindowRect( int columnCount /*= 1*/, CWnd* pListWnd /*= NULL */ )
{
	CRect ncExtent = GetNcExtentRect( columnCount, pListWnd );
	CRect windowRect( 0, 0, GetInitialSize( columnCount ).cx, 0 );

	windowRect.InflateRect( ncExtent );
	return windowRect;
}

// called by the splitter window to adjust list window width
int CAlbumThumbListView::QuantifyListWidth( int listWidth )
{
	CRect ncExtent = GetNcExtentRect( ComputeColumnCount( listWidth ), this );
	int listClientWidth = listWidth - ( ncExtent.left + ncExtent.right );
	int newColCount = ComputeColumnCount( listClientWidth );
	CRect newWindowRect = GetListWindowRect( newColCount, this );

	return newWindowRect.Width();
}

CRect CAlbumThumbListView::GetNcExtentRect( int columnCount /*= 1*/, CWnd* pListWnd /*= NULL*/ )
{
	static CSize frameSize( GetSystemMetrics( SM_CXBORDER ), GetSystemMetrics( SM_CYBORDER ) );
	CRect ncExtent( frameSize.cx, frameSize.cy, frameSize.cx, frameSize.cy );

	if ( pListWnd != NULL )
	{
		CRect rectWindow, rectClient;

		pListWnd->GetWindowRect( &rectWindow );
		pListWnd->ScreenToClient( &rectWindow );
		pListWnd->GetClientRect( &rectClient );

		ncExtent.TopLeft() = rectClient.TopLeft() - rectWindow.TopLeft();
		ncExtent.BottomRight() = rectWindow.BottomRight() - rectClient.BottomRight();
	}
	else
		// add scroll width to non-client size if column count is 0 or 1
		if ( 1 == columnCount )
			ncExtent.right += GetSystemMetrics( SM_CXVSCROLL );		// zero or one column -> we have verticall scroll-bar
	// U HAVE 2 BELIEVE THIS: compensate border by 1 (has to do with border style for the view)
	ncExtent.left--;
	ncExtent.top--;
	ncExtent.right--;
	ncExtent.bottom--;
	return ncExtent;
}

int CAlbumThumbListView::GetListClientWidth( int listWidth )
{
	int ncWidth = 0;

	if ( m_hWnd != NULL )
	{
		CRect rectWindow, rectClient;

		GetWindowRect( rectWindow );
		GetClientRect( rectClient );
		ncWidth = rectWindow.Width() - rectClient.Width();
	}
	else
		// check empirically the column count and deduct the non-client size
		switch ( int( double( listWidth ) / GetInitialSize().cx + 0.5 ) )
		{
			case 1:				// zero or one column -> we have verticall scroll-bar
				ncWidth = GetSystemMetrics( SM_CXVSCROLL );
			default:
				ncWidth += 2 * GetSystemMetrics( SM_CXBORDER );
				break;
		}
	return listWidth - ncWidth;
}

bool CAlbumThumbListView::CheckListLayout( CheckLayoutMode checkMode /*= SplitterTrack*/ )
{
	bool isFullScreen = app::GetMainFrame()->IsFullScreen();
	int columnCount = 0;

	// first determine the column count
	switch ( checkMode )
	{
		case SplitterTrack:
		{
			CRect clientRect;
			GetClientRect( &clientRect );
			columnCount = clientRect.Width() / GetInitialSize().cx;
			break;
		}
		case AlbumViewInit:
		case ShowCommand:
			columnCount = m_pPeerImageView->GetSlideData().m_thumbListColCount;
			if ( isFullScreen || !HasFlag( m_pPeerImageView->GetSlideData().m_viewFlags, af::ShowThumbView ) )
				columnCount = 0;		// This thumb view is to be made invisible!
			break;
		default:
			ASSERT( false );
	}

	ASSERT( columnCount >= 0 );
	// corelate the thumb list visible flag with the actual visibility
	if ( !isFullScreen )
		SetFlag( m_pPeerImageView->RefSlideData().m_viewFlags, af::ShowThumbView, columnCount > 0 );

	if ( columnCount > 0 )
		m_pPeerImageView->RefSlideData().m_thumbListColCount = columnCount;		// store the current column count
	else
		GetParentFrame()->SetActiveView( m_pPeerImageView );					// make the album view active (focused)

	// must recreate view if single column and has LBS_MULTICOLUMN, or multiple column and hasn't LBS_MULTICOLUMN
	bool recreate = ( 1 == columnCount ) == HasFlag( GetStyle(), LBS_MULTICOLUMN );

	if ( !recreate )
	{
		if ( ShowCommand == checkMode )
			m_pSplitterWnd->MoveColumnToPos( GetListWindowRect( columnCount ).right, 0 );		// set column width
		return false;
	}
	return RecreateView( columnCount );
}

bool CAlbumThumbListView::RecreateView( int columnCount )
{
	ASSERT_PTR( m_pSplitterWnd );

	CBackupData orgViewData( this );
	CFrameWnd* pFrame = GetParentFrame();
	bool wasActive = pFrame != NULL && pFrame->GetActiveView() == this;

	m_autoDelete = false;				// prevent view object delete on window destroy
	GetDocument()->RemoveView( this );
	DestroyWindow();
	m_autoDelete = true;				// restore to view object auto-deletion

	// initialize the appropriate creation style
	s_listCreationStyle = CAlbumThumbListView::GetListCreationStyle( columnCount );

	CRect thumbRect = GetListWindowRect( columnCount );

	if ( !m_pSplitterWnd->DoRecreateWindow( *this, thumbRect.Size() ) )
		return false;

	m_pPeerImageView->LateInitialUpdate();			// display this thumb (re-created initially hidden)

	orgViewData.Restore( this );
	m_pSplitterWnd->RecalcLayout();

	// restore view activation (if was previously active)
	if ( wasActive )
		pFrame->SetActiveView( this );
	return true;
}

void CAlbumThumbListView::EnsureCaptionFontCreated( void )
{
	if ( NULL == HFONT( s_fontCaption ) )
	{
		NONCLIENTMETRICS ncMetrics;

		ZeroMemory( &ncMetrics, sizeof( NONCLIENTMETRICS ) );
		ncMetrics.cbSize = sizeof( NONCLIENTMETRICS );
		::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncMetrics, 0 );
		s_fontCaption.CreateFontIndirect( &ncMetrics.lfStatusFont );
		s_fontHeight = abs( ncMetrics.lfStatusFont.lfHeight );
	}
}

CWicDibSection* CAlbumThumbListView::GetItemThumb( int displayIndex ) const throws_()
{
	if ( !IsValidImageIndex( displayIndex ) )
		return NULL;					// index violation, could happen in transient draws

	const fs::CFlexPath& imageFilePath = m_pAlbumModel->GetFileAttr( displayIndex )->GetPath();
	if ( imageFilePath.IsEmpty() )
		return NULL;

	return app::GetThumbnailer()->AcquireThumbnailNoThrow( imageFilePath );
}

const fs::CFlexPath* CAlbumThumbListView::GetItemPath( int displayIndex ) const
{
	if ( !IsValidImageIndex( displayIndex ) )
		return NULL;					// Index violation, could happen in transient draws

	return &m_pAlbumModel->GetFileAttr( displayIndex )->GetPath();
}

CSize CAlbumThumbListView::GetPageItemCounts( void ) const
{
	CListBox* pListBox = AsListBox();
	CSize pageScrollSize( 1, 1 );
	CRect itemRect;

	if ( pListBox->GetItemRect( pListBox->GetTopIndex(), &itemRect ) != LB_ERR )
	{
		CRect clientRect;
		pListBox->GetClientRect( &clientRect );

		pageScrollSize.cx = clientRect.Width() / itemRect.Width();
		pageScrollSize.cy = clientRect.Height() / itemRect.Height();
	}

	return pageScrollSize;
}

bool CAlbumThumbListView::DoDragDrop( void )
{
	CancelDragCapture();

	TRACE( _T("(*) DragAndDrop Selection: %s\n"), dbg::FormatSelectedItems( AsListBox() ).c_str() );

	// fetch the selected file(s) sorted in ascending order
	CListViewState indexesToDropState( StoreByIndex );
	GetListViewState( indexesToDropState );
	if ( indexesToDropState.IsEmpty() )
		return false;

	CListSelectionData selData( this );
	selData.m_selIndexes = indexesToDropState.m_pIndexImpl->m_selItems;

	std::vector< CFileAttr* > selSequence;
	m_pAlbumModel->QueryFileAttrsSequence( selSequence, selData.m_selIndexes );

	std::vector< fs::CPath > filesToDrag;
	utl::Assign( filesToDrag, selSequence, func::ToFilePath() );

	ole::CImagesDataSource dataSource;
	dataSource.CacheShellFilePaths( filesToDrag );
	selData.CacheTo( &dataSource );							// cache source indexes

	return dataSource.DragAndDropImages( m_hWnd, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK ) != DROPEFFECT_NONE;
}

void CAlbumThumbListView::CancelDragCapture( void )
{
	if ( ::GetCapture() == m_hWnd )
	{	// Before releasing capture, send an artificial LBN_SELCHANGE notification if
		// the caret index is different than the album view's current index
		int caretIndex = AsListBox()->GetCaretIndex();

		if ( caretIndex != LB_ERR )
			NotifySelChange();

		SendMessage( WM_CANCELMODE );
	}
	m_beginDragTimer.Stop();
	m_startDragRect.SetRectEmpty();
}

DROPEFFECT CAlbumThumbListView::OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	m_scrollTimerCounter.x = m_scrollTimerCounter.y = 0;
	return OnDragOver( pDataObject, keyState, point );
}

DROPEFFECT CAlbumThumbListView::OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	keyState;
	CListSelectionData selData;

	if ( selData.ExtractFrom( pDataObject ) && selData.IsValid() )
		if ( selData.m_pSrcWnd != NULL /*&& selData.m_pThumbView->GetAlbumDoc() == GetAlbumDoc()*/ )		// custom order D&D is allowed only between views of the same document
		{
			int dropIndex = GetImageIndexFromPoint( point );
			if ( -1 == dropIndex )
				dropIndex = static_cast<int>( m_pAlbumModel->GetFileAttrCount() );			// drop append to end

			if ( seq::ChangesDropSequenceAt( m_pAlbumModel->GetFileAttrCount(), dropIndex, selData.m_selIndexes ) )
				return DROPEFFECT_MOVE;
		}

	return DROPEFFECT_NONE;
}

BOOL CAlbumThumbListView::OnDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
{
	dropEffect;

	OnDragLeave();

	CListSelectionData selData;
	if ( !selData.ExtractFrom( pDataObject ) || !selData.IsValid() )
		return FALSE;

	CAlbumDoc* pAlbumDoc = GetAlbumDoc();
	int dropIndex = GetImageIndexFromPoint( point );

	// custom order drag&drop was allowed only between views of the same document... TODO
	if ( CAlbumThumbListView* pThumbView = dynamic_cast< CAlbumThumbListView* >( selData.m_pSrcWnd ) )
		if ( pThumbView->GetAlbumDoc() != pAlbumDoc )
		{
			ASSERT( false );	// TODO: drag&drop between albums
		}

	m_dragSelIndexes.swap( selData.m_selIndexes );			// used for temporary storing display indexes to drop

	if ( LB_ERR == dropIndex )
		dropIndex = static_cast< int >( m_pAlbumModel->GetFileAttrCount() );		// if dropIndex is -1, then move at back selected indexes

	if ( !pAlbumDoc->GetModel()->IsCustomOrder() )			// not yet in custom order: prompt the user to switch to custom order
		if ( IDOK == AfxMessageBox( IDS_PROMPT_SWITCHTOCUSTOMORDER, MB_OKCANCEL | MB_ICONQUESTION ) )
			pAlbumDoc->RefModel()->StoreFileOrder( fattr::CustomOrder );
		else
			return FALSE;

	TRACE( _T("\tDropped to index=%d selIndexes: %s\n"), dropIndex, str::FormatSet( m_dragSelIndexes ).c_str() );

	// for the views != than the target view (if any), backup current/near selection before modifying the m_pAlbumModel member
	pAlbumDoc->UpdateAllViewsOfType( m_pPeerImageView, Hint_SmartBackupSelection );

	// move the selected indexes to their new position; after this call m_dragSelIndexes contains the new display indexes for the dropped files.
	if ( pAlbumDoc->DropCustomOrder( dropIndex, m_dragSelIndexes ) )
	{
		// after the drop, display indexes have changed - update the view
		pAlbumDoc->SetModifiedFlag();
		pAlbumDoc->OnAlbumModelChanged( FM_CustomOrderChanged );

		pAlbumDoc->UpdateAllViewsOfType( m_pPeerImageView, Hint_RestoreSelection );

		CListViewState dropState( m_dragSelIndexes );		// now it became 'droppedSelIndexes'

		SetListViewState( dropState, true );
		m_pPeerImageView->OnUpdate( NULL, 0, NULL );

		return TRUE;
	}
	else
		TRACE( _T(" (!) After display indexes move nothing is actually changed!\n") );

	return FALSE;
}

void CAlbumThumbListView::OnDragLeave( void )
{
}

BOOL CAlbumThumbListView::PreCreateWindow( CREATESTRUCT& rCS )
{
	// NB: thumb view is initially invisible and will turn visible further on CAlbumImageView::LateInitialUpdate()
	rCS.style = WS_CHILD | WS_BORDER | WS_GROUP | WS_TABSTOP |
				LBS_EXTENDEDSEL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED;

	// Very important initialization, since the static member is temporary altered
	// during view recreation (for LBS_MULTICOLUMN style change)
	rCS.style |= CAlbumThumbListView::s_listCreationStyle;

	return __super::PreCreateWindow( rCS );
}

void CAlbumThumbListView::PostNcDestroy( void )
{
	if ( m_autoDelete )
		__super::PostNcDestroy();
}

BOOL CAlbumThumbListView::OnScroll( UINT scrollCode, UINT pos, BOOL doScroll /*= TRUE*/ )
{
	pos;
	// called in custom order drop auto-scroll
	// note: sizeScroll has actually scroll command semantics
	CSize sizeScroll( SB_ENDSCROLL, SB_ENDSCROLL );				// SB_ENDSCROLL actually means 'no scroll'

	switch ( LOBYTE( scrollCode ) )
	{
		case SB_LINELEFT:
		case SB_LINERIGHT:
		case SB_PAGELEFT:
		case SB_PAGERIGHT:
		case SB_LEFT:
		case SB_RIGHT:
			sizeScroll.cx = LOBYTE( scrollCode );
			break;
	}
	switch ( HIBYTE( scrollCode ) )
	{
		case SB_LINEUP:
		case SB_LINEDOWN:
		case SB_PAGEUP:
		case SB_PAGEDOWN:
		case SB_TOP:
		case SB_BOTTOM:
			sizeScroll.cy = HIBYTE( scrollCode );
			break;
	}

	return OnScrollBy( sizeScroll, doScroll );
}

BOOL CAlbumThumbListView::OnScrollBy( CSize sizeScroll, BOOL doScroll )
{
	// note: sizeScroll has actually scroll command semantics
	CSize scrollCommand = sizeScroll;
	DWORD style = GetStyle();
	bool result = false;

	if ( HasFlag( style, WS_HSCROLL ) && scrollCommand.cx != SB_ENDSCROLL )
	{
		if ( doScroll )											// line scroll?
			if ( !( m_scrollTimerCounter.x++ % scrollTimerDivider.cx ) )
				SendMessage( WM_HSCROLL, MAKEWPARAM( scrollCommand.cx, 0 ), NULL );
		result = true;
	}

	if ( HasFlag( style, WS_VSCROLL ) && scrollCommand.cy != SB_ENDSCROLL )
	{
		if ( doScroll && scrollCommand.cy != SB_ENDSCROLL )		// line scroll?
			if ( !( m_scrollTimerCounter.y++ % scrollTimerDivider.cy ) )
				SendMessage( WM_VSCROLL, MAKEWPARAM( scrollCommand.cy > 0, 0 ), NULL );
		result = true;
	}

	return result;
}

void CAlbumThumbListView::OnActivateView( BOOL activate, CView* pActivateView, CView* pDeactiveView )
{
	__super::OnActivateView( activate, pActivateView, pDeactiveView );

	if ( activate )
		if ( NULL == pDeactiveView || pDeactiveView == pActivateView || pDeactiveView->GetParentFrame() != GetParentFrame() )		// not a peer activation?
			m_pPeerImageView->EventChildFrameActivated();
}

void CAlbumThumbListView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	UpdateViewHint hint = (UpdateViewHint)lHint;
	switch ( hint )
	{
		case Hint_ViewUpdate:
			break;
		case Hint_ThumbBoundsResized:
			GetParentFrame()->DestroyWindow();
			return;
		default:
			return;
	}

	__super::OnUpdate( pSender, lHint, pHint );
}

BOOL CAlbumThumbListView::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( HandleCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_pPeerImageView->CImageView::OnCmdMsg( id, code, pExtra, pHandlerInfo );		// redirect to album view for common cmdIds; non-virtual call CImageView::OnCmdMsg() to avoid infinite recursion
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumThumbListView, CCtrlView )
	ON_WM_CREATE()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_DROPFILES()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
	ON_WM_TIMER()
	ON_COMMAND( CK_SHOW_THUMB_VIEW, OnToggleShowThumbView )
	ON_UPDATE_COMMAND_UI( CK_SHOW_THUMB_VIEW, OnUpdateShowThumbView )
	ON_CONTROL_REFLECT( LBN_SELCHANGE, OnLBnSelChange )
END_MESSAGE_MAP()

BOOL CAlbumThumbListView::OnChildNotify( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	UNUSED( pResult );
	switch ( message )
	{
		case WM_MEASUREITEM: MeasureItem( (MEASUREITEMSTRUCT*)lParam ); break;
		case WM_DRAWITEM: DrawItem( (DRAWITEMSTRUCT*)lParam ); break;
		default: return __super::OnChildNotify( message, wParam, lParam, pResult );
	}
	return TRUE;
}

int CAlbumThumbListView::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == __super::OnCreate( pCS ) )
		return -1;

// TODO: refine the details of auto-scrolling, custom drop-tips
//	m_dropTarget.SetScrollMode( auto_scroll::HorizBar | auto_scroll::UseDefault );
//	m_dropTarget.SetDropTipText( DROPIMAGE_MOVE, _T("Move [THUMB]"), _T("Move to %1 [THUMB]") );
//	m_dropTarget.SetDropTipText( DROPIMAGE_COPY, _T("Copy [THUMB]"), _T("Copy to %1 [THUMB]") );

	m_dropTarget.Register( this );		// register drop target

	if ( HasFlag( GetStyle(), LBS_MULTICOLUMN ) )
		AsListBox()->SetColumnWidth( GetInitialSize().cx );

	SetFont( &s_fontCaption );
	return 0;
}

HBRUSH CAlbumThumbListView::CtlColor( CDC* pDC, UINT ctlColor )
{
	pDC, ctlColor;
	return GetSysColorBrush( COLOR_BTNFACE );
}

void CAlbumThumbListView::OnDropFiles( HDROP hDropInfo )
{
	GetAlbumDoc()->HandleDropRecipientFiles( hDropInfo, m_pPeerImageView );
}

void CAlbumThumbListView::OnSetFocus( CWnd* pOldWnd )
{
	__super::OnSetFocus( pOldWnd );
}

void CAlbumThumbListView::OnLButtonDown( UINT mkFlags, CPoint point )
{
	__super::OnLButtonDown( mkFlags, point );

	// prepare the drag rect
	int dragDropIndex = GetImageIndexFromPoint( point );
	if ( dragDropIndex != LB_ERR )
	{	// enter the drag-capture lvState
		ASSERT( m_startDragRect.IsRectNull() && !m_beginDragTimer.IsStarted() );
		// set the geometric reference
		m_startDragRect.TopLeft() = m_startDragRect.BottomRight() = point;
		m_startDragRect.InflateRect( GetSystemMetrics( SM_CXDRAG ), GetSystemMetrics( SM_CYDRAG ) );
		// set the temporal reference
		m_beginDragTimer.Start();
	}
}

void CAlbumThumbListView::OnLButtonUp( UINT mkFlags, CPoint point )
{
	__super::OnLButtonUp( mkFlags, point );
	CancelDragCapture();
}

void CAlbumThumbListView::OnRButtonDown( UINT mkFlags, CPoint point )
{
	int hitIndex = GetImageIndexFromPoint( point );
	CListBox* pListBox = AsListBox();
	if ( hitIndex != LB_ERR )
		if ( !pListBox->GetSel( hitIndex ) )
			if ( IsMultiSelection() )
			{
				if ( !HasFlag( mkFlags, MK_CONTROL ) )
					pListBox->SelItemRange( FALSE, 0, pListBox->GetCount() - 1 );		// Clear previous selection
				pListBox->SetSel( hitIndex );
			}
			else
				pListBox->SetCurSel( hitIndex );

	__super::OnRButtonDown( mkFlags, point );
}

void CAlbumThumbListView::OnMouseMove( UINT mkFlags, CPoint point )
{
	__super::OnMouseMove( mkFlags, point );

	if ( HasFlag( mkFlags, MK_LBUTTON ) )
		if ( !m_startDragRect.IsRectNull() && !m_startDragRect.PtInRect( point ) )
			DoDragDrop();
}

BOOL CAlbumThumbListView::OnMouseWheel( UINT mkFlags, short zDelta, CPoint pt )
{
	if ( HasFlag( mkFlags, MK_CONTROL | MK_SHIFT ) )
	{
		int delta = -( zDelta / WHEEL_DELTA );

		m_pPeerImageView->NavigateBy( delta );
		return TRUE;		// message processed
	}
	else if ( HasFlag( GetStyle(), WS_HSCROLL ) )
	{
		// customize behavior only for horizontal scrolling
		int minPos, maxPos, pos = GetScrollPos( SB_HORZ ), orgPos = pos;

		GetScrollRange( SB_HORZ, &minPos, &maxPos );
		pos += -( zDelta / WHEEL_DELTA );
		pos = std::max( pos, minPos );
		pos = std::min( pos, maxPos );

		if ( pos != orgPos )
		{
			MSG msg;
			SendMessage( WM_HSCROLL, MAKEWPARAM( SB_THUMBPOSITION, pos ) );
			// eat the remaining WM_MOUSEWHEEL messages
			while ( ::PeekMessage( &msg, m_hWnd, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE ) )
			{
			}
		}
		return TRUE;		// message processed
	}
	else
		return __super::OnMouseWheel( mkFlags, zDelta, pt );
}

void CAlbumThumbListView::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	pWnd;

	CMenu* pSrcPopupMenu = &GetContextMenu();
	if ( pSrcPopupMenu->GetSafeHmenu() != NULL )
	{
		std::vector< fs::CFlexPath > selFilePaths;
		if ( QuerySelItemPaths( selFilePaths ) )
		{
			std::vector< fs::CPath > physicalPaths;
			path::QueryPhysicalPaths( physicalPaths, selFilePaths );

			if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, physicalPaths ) )
				DoTrackContextMenu( pContextPopup, screenPos );
		}
		else
			DoTrackContextMenu( pSrcPopupMenu, screenPos );
	}
}

void CAlbumThumbListView::OnTimer( UINT_PTR eventId )
{
	if ( m_beginDragTimer.IsHit( eventId ) )
		DoDragDrop();
	else
		__super::OnTimer( eventId );
}

void CAlbumThumbListView::OnToggleShowThumbView( void )
{
	ToggleFlag( m_pPeerImageView->RefSlideData().m_viewFlags, af::ShowThumbView );
	CheckListLayout( ShowCommand );
}

void CAlbumThumbListView::OnUpdateShowThumbView( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !app::GetMainFrame()->IsFullScreen() );
	pCmdUI->SetCheck( HasFlag( m_pPeerImageView->GetSlideData().m_viewFlags, af::ShowThumbView ) );
}

void CAlbumThumbListView::OnLBnSelChange( void )
{
//	TRACE( _T("(*) OnLBnSelChange Selection: %s\n"), dbg::FormatSelectedItems( AsListBox() ).c_str() );

	++m_userChangeSel;
	m_pPeerImageView->OnSelChangeThumbList();
	--m_userChangeSel;
}


// CAlbumThumbListView::CBackupData implementation

CAlbumThumbListView::CBackupData::CBackupData( const CAlbumThumbListView* pSrcThumbView )
	: m_pAlbumModel( pSrcThumbView->GetAlbumModel() )
	, m_topIndex( pSrcThumbView->AsListBox()->GetTopIndex() )
	, m_currIndex( pSrcThumbView->GetCurSel() )
	, m_listCreationStyle( CAlbumThumbListView::s_listCreationStyle )
{
}

CAlbumThumbListView::CBackupData::~CBackupData()
{
	CAlbumThumbListView::s_listCreationStyle = m_listCreationStyle;
}

void CAlbumThumbListView::CBackupData::Restore( CAlbumThumbListView* pDestThumbView )
{
	pDestThumbView->SetupAlbumModel( m_pAlbumModel, false );		// don't redraw yet

	pDestThumbView->SetCurSel( m_currIndex );
	pDestThumbView->AsListBox()->SetTopIndex( m_topIndex );
}
