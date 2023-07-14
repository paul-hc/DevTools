
#include "pch.h"
#include "ReportListControl.h"
#include "ReportListCustomDraw.h"
#include "CustomDrawImager.h"
#include "CmdUpdate.h"
#include "Color.h"
#include "MenuUtilities.h"
#include "ShellDragImager.h"
#include "OleDataSource.h"
#include "StringUtilities.h"
#include "StringRange.h"
#include "WndUtilsEx.h"
#include "CheckStatePolicies.h"
#include "ComparePredicates.h"
#include "FileSystem.h"
#include "ShellUtilities.h"
#include "PostCall.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/Serialization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "BaseTrackMenuWnd.hxx"
#include "ReportListControl.hxx"
#include "utl/Resequence.hxx"


namespace reg
{
	static const TCHAR entry_viewMode[] = _T("ViewMode");
	static const TCHAR entry_viewStacking[] = _T("ViewStacking");
	static const TCHAR entry_sortByColumn[] = _T("SortByColumn");
	static const TCHAR entry_sortAscending[] = _T("SortAscending");
}


namespace lv
{
	DWORD CmdIdToListViewMode( UINT cmdId )
	{
		switch ( cmdId )
		{
			default: ASSERT( false );
			case ID_LIST_VIEW_ICON_LARGE:	return LV_VIEW_ICON;
			case ID_LIST_VIEW_ICON_SMALL:	return LV_VIEW_SMALLICON;
			case ID_LIST_VIEW_LIST:			return LV_VIEW_LIST;
			case ID_LIST_VIEW_REPORT:		return LV_VIEW_DETAILS;
			case ID_LIST_VIEW_TILE:			return LV_VIEW_TILE;
		}
	}

	DWORD CmdIdToListViewStacking( UINT cmdId )
	{
		switch ( cmdId )
		{
			default: ASSERT( false );
			case ID_LIST_VIEW_STACK_LEFTTORIGHT:	return LVS_ALIGNTOP;
			case ID_LIST_VIEW_STACK_TOPTOBOTTOM:	return LVS_ALIGNLEFT;
		}
	}

	seq::MoveTo CmdIdToMoveTo( UINT cmdId )
	{
		switch ( cmdId )
		{
			default: ASSERT( false );
			case ID_MOVE_UP_ITEM:		return seq::MovePrev;
			case ID_MOVE_DOWN_ITEM:		return seq::MoveNext;
			case ID_MOVE_TOP_ITEM:		return seq::MoveToStart;
			case ID_MOVE_BOTTOM_ITEM:	return seq::MoveToEnd;
		}
	}


	// CNmToggleCheckState implementation

	CNmToggleCheckState::CNmToggleCheckState( const CListCtrl* pListCtrl, NMLISTVIEW* pListView, const ui::ICheckStatePolicy* pCheckStatePolicy )
		: m_nmHdr( pListCtrl, lv::LVN_ToggleCheckState )
		, m_pListView( pListView )
		, m_oldCheckState( ui::CheckStateFromRaw( m_pListView->uOldState ) )
		, m_newCheckState( pCheckStatePolicy != nullptr ? pCheckStatePolicy->Toggle( m_oldCheckState ) : m_oldCheckState )		// toggle check-state by default
	{
	}


	// CNmCheckStatesChanged implementation

	bool CNmCheckStatesChanged::AddIndex( int itemIndex )
	{
		return utl::AddUnique( m_itemIndexes, itemIndex );
	}
}


static ACCEL s_keys[] =
{
	{ FVIRTKEY | FCONTROL, _T('A'), ID_EDIT_SELECT_ALL },		// Ctrl+A
	{ FVIRTKEY | FCONTROL, _T('C'), ID_EDIT_COPY },				// Ctrl+C
	{ FVIRTKEY | FCONTROL, VK_INSERT, ID_EDIT_COPY },			// Ctrl+Ins
	{ FVIRTKEY | FCONTROL, VK_ADD, ID_EXPAND },					// Ctrl+Plus
	{ FVIRTKEY | FCONTROL, VK_SUBTRACT, ID_COLLAPSE },			// Ctrl+Minus
	{ FVIRTKEY, VK_F2, ID_RENAME_ITEM }
};

const TCHAR CReportListControl::s_fmtRegColumnLayout[] = _T("Width=%d, Order=%d");


CReportListControl::CReportListControl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: CBaseTrackMenuWnd<CListCtrl>()
	, CListLikeCtrlBase( this )
	, m_columnLayoutId( 0 )
	, m_listStyleEx( listStyleEx )
	, m_optionFlags( SortInternally | PersistSorting | HighlightTextDiffsFrame )
	, m_subjectBased( false )
	, m_pTabularSep( nullptr )
	, m_sortByColumn( -1 )			// no sorting by default
	, m_sortAscending( true )
	, m_pComparePtrFunc( nullptr )		// use text compare by default
	, m_pImageList( nullptr )
	, m_pLargeImageList( nullptr )
	, m_pCheckStatePolicy( nullptr )
	, m_pFrameEditor( nullptr )
	, m_pDataSourceFactory( ole::GetStdDataSourceFactory() )
	, m_painting( false )
	, m_applyingCheckStateToSelectedItems( false )
	, m_stateIconSize( 0, 0 )
	, m_deleteSrc_DiffEffect( ui::Bold, s_deleteSrcTextColor )
	, m_mismatchDest_DiffEffect( ui::Bold, s_mismatchDestTextColor )
	, m_matchDest_DiffEffect( ui::Regular, GetSysColor( COLOR_GRAYTEXT ) )
{
	m_ctrlAccel.Create( ARRAY_SPAN( s_keys ) );

	m_pPopupMenu[ Nowhere ] = &GetStdPopupMenu( Nowhere );
	m_pPopupMenu[ OnSelection ] = &GetStdPopupMenu( OnSelection );
	m_pPopupMenu[ OnGroup ] = &GetStdPopupMenu( OnGroup );

	if ( columnLayoutId != 0 )
		SetLayoutInfo( columnLayoutId );
}

CReportListControl::~CReportListControl()
{
	ClearData();

	for ( std::vector<CColumnComparator>::const_iterator itColComparator = m_comparators.begin(); itColComparator != m_comparators.end(); ++itColComparator )
		delete itColComparator->m_pComparator;
}

bool CReportListControl::SetOptionFlag( ListOption flag, bool on )
{
	if ( HasFlag( m_optionFlags, flag ) == on )
		return false;

	SetFlag( m_optionFlags, flag, on );
	if ( m_hWnd != nullptr )
		Invalidate();
	return true;
}

bool CReportListControl::ModifyListStyleEx( DWORD dwRemove, DWORD dwAdd, UINT swpFlags /*= 0*/ )
{
	DWORD listStyleEx = m_hWnd != nullptr ? GetExtendedStyle() : m_listStyleEx;
	DWORD newListStyleEx = ( listStyleEx & ~dwRemove ) | dwAdd;

	if ( listStyleEx == newListStyleEx )
		return false;

	m_listStyleEx = newListStyleEx;

	if ( m_hWnd != nullptr )
	{
		SetExtendedStyle( m_listStyleEx );

		if ( swpFlags != 0 )
			SetWindowPos( nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | swpFlags );
	}

	return true;
}

COLORREF CReportListControl::GetActualTextColor( void ) const
{
	return ui::GetActualColorSysdef( GetTextColor(), COLOR_WINDOWTEXT );
}

bool CReportListControl::DeleteAllItems( void )
{
	ClearData();
	return __super::DeleteAllItems() != FALSE;
}

void CReportListControl::RemoveAllGroups( void )
{
	m_groupIdToItemsMap.clear();
	__super::RemoveAllGroups();
}

void CReportListControl::ClearData( void )
{
	m_initialItemsOrder.clear();
	m_groupIdToItemsMap.clear();
	m_markedCells.clear();
	m_diffColumnPairs.clear();
}

void CReportListControl::StoreImageLists( CImageList* pImageList, CImageList* pLargeImageList /*= nullptr*/ )
{
	m_pImageList = pImageList;
	m_pLargeImageList = pLargeImageList;

	if ( m_hWnd != nullptr )
	{
		SetImageList( m_pImageList, LVSIL_SMALL );
		SetImageList( m_pLargeImageList, LVSIL_NORMAL );

		UpdateCustomImagerBoundsSize();
	}
}

bool CReportListControl::UpdateCustomImagerBoundsSize( void )
{
	if ( m_pCustomImager.get() != nullptr )
		return m_pCustomImager->SetCurrGlyphGauge( GetViewModeGlyphGauge() );

	return false;			// no change
}

void CReportListControl::SetCustomFileGlyphDraw( bool showGlyphs /*= true*/ )
{
	CListLikeCtrlBase::SetCustomFileGlyphDraw( showGlyphs );

	if ( showGlyphs )
	{
		StoreImageLists( m_pCustomImager->GetImageList( ui::SmallGlyph ), m_pCustomImager->GetImageList( ui::LargeGlyph ) );
		ModifyListStyleEx( LVS_EX_DOUBLEBUFFER, 0 );		// better looking thumb rendering for tiny images (icons, small PNGs)
	}
	else
	{
		StoreImageLists( nullptr, nullptr );
		ModifyListStyleEx( 0, LVS_EX_DOUBLEBUFFER );
	}

	if ( m_hWnd != nullptr )
	{
		if ( nullptr == m_pCustomImager.get() )
		{	// hack: force the list to remove icon area for all items
			ModifyStyle( LVS_TYPEMASK, LVS_ICON );
			ChangeListViewMode( LV_VIEW_DETAILS );			// switch to report mode since other modes don't make sense without icons
		}

		ui::RecalculateScrollBars( m_hWnd );
	}
}

void CReportListControl::SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, const CSize& smallImageSize /*= CSize( 0, 0 )*/, const CSize& largeImageSize /*= CSize( 0, 0 )*/ )
{
	if ( pCustomImageDraw != nullptr )
	{
		m_pCustomImager.reset( new CSingleCustomDrawImager( pCustomImageDraw, smallImageSize, largeImageSize ) );
		StoreImageLists( m_pCustomImager->GetImageList( ui::SmallGlyph ), m_pCustomImager->GetImageList( ui::LargeGlyph ) );
		ModifyListStyleEx( LVS_EX_DOUBLEBUFFER, 0 );		// better looking thumb rendering for tiny images (icons, small PNGs)
	}
	else
	{
		m_pCustomImager.reset();
		StoreImageLists( nullptr, nullptr );
		ModifyListStyleEx( 0, LVS_EX_DOUBLEBUFFER );
	}

	if ( m_hWnd != nullptr )
	{
		if ( nullptr == m_pCustomImager.get() )
		{	// hack: force the list to remove icon area for all items
			ModifyStyle( LVS_TYPEMASK, LVS_ICON );
			ChangeListViewMode( LV_VIEW_DETAILS );			// switch to report mode since other modes don't make sense without icons
		}

		ui::RecalculateScrollBars( m_hWnd );
	}
}

void CReportListControl::SetCustomIconDraw( ui::ICustomImageDraw* pCustomIconDraw, IconStdSize smallIconStdSize /*= SmallIcon*/, IconStdSize largeIconStdSize /*= LargeIcon*/ )
{
	SetCustomImageDraw( pCustomIconDraw, CIconSize::GetSizeOf( smallIconStdSize ), CIconSize::GetSizeOf( largeIconStdSize ) );
}

CMenu& CReportListControl::GetStdPopupMenu( ListPopup popupType )
{
	static CMenu s_stdPopupMenu[ _ListPopupCount ];
	CMenu& rMenu = s_stdPopupMenu[ popupType ];

	if ( nullptr == rMenu.GetSafeHmenu() )
	{
		switch ( popupType )
		{
			default: ASSERT( false );
			case Nowhere:		ui::LoadPopupMenu( &rMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::ListView, lv::NowhereSubPopup ) ); break;
			case OnSelection:	ui::LoadPopupMenu( &rMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::ListView, lv::OnSelectionSubPopup ) ); break;
			case OnGroup:		ui::LoadPopupMenu( &rMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::ListView, lv::OnGroupSubPopup ) ); break;
		}
	}
	return rMenu;
}

CMenu* CReportListControl::GetPopupMenu( ListPopup popupType )
{
	return m_pPopupMenu[ popupType ];
}

bool CReportListControl::TrackContextMenu( ListPopup popupType, const CPoint& screenPos )
{
	if ( CMenu* pPopupMenu = GetPopupMenu( popupType ) )
	{
		ui::TrackPopupMenu( *pPopupMenu, m_pTrackMenuTarget, screenPos );
		return true;		// handled
	}

	return false;
}

void CReportListControl::SetupControl( void )
{
	if ( m_listStyleEx != 0 )
		SetExtendedStyle( m_listStyleEx );

	CListLikeCtrlBase::SetupControl();

	if ( m_pImageList != nullptr )
		SetImageList( m_pImageList, LVSIL_SMALL );
	if ( m_pLargeImageList != nullptr )
		SetImageList( m_pLargeImageList, LVSIL_NORMAL );

	if ( m_pCheckStatePolicy != nullptr )
	{
		CImageList* pStateImageList = GetStateImageList();
		ASSERT( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) && pStateImageList != nullptr );
		ASSERT( 2 == pStateImageList->GetImageCount() );				// standard check-box

		if ( const std::vector<CThemeItem>* pThemeItems = m_pCheckStatePolicy->GetThemeItems() )
			// mask transparent colour - almost white, so that themes that render with alpha blending don't show weird colours (such as radio button)
			ui::AppendToStateImageList( pStateImageList, *pThemeItems, ui::AlterColorSlightly( GetBkColor() ) );
	}

	if ( GetAcceptDropFiles() )
		DragAcceptFiles();

	if ( !m_regSection.empty() )
		LoadFromRegistry();
	else
		SetupColumnLayout( nullptr );

	UpdateCustomImagerBoundsSize();
	UpdateColumnSortHeader();		// update current sorting order
}

void CReportListControl::ChangeListViewMode( DWORD viewMode )
{
	DWORD oldViewMode = GetView();

	switch ( viewMode )
	{
		case LV_VIEW_ICON:		ModifyStyle( LVS_TYPEMASK, LVS_ICON ); break;
		case LV_VIEW_DETAILS:	ModifyStyle( LVS_TYPEMASK, LVS_REPORT ); break;
		case LV_VIEW_SMALLICON:	ModifyStyle( LVS_TYPEMASK, LVS_SMALLICON ); break;
		case LV_VIEW_LIST:		ModifyStyle( LVS_TYPEMASK, LVS_LIST ); break;
		case LV_VIEW_TILE:		break;
		default: ASSERT( false );
	}

	SetView( viewMode );

	if ( viewMode != oldViewMode )
	{
		UpdateCustomImagerBoundsSize();

		if ( LV_VIEW_DETAILS == viewMode )
			ResizeFlexColumns();			// restretch flex columns
	}
}

ui::GlyphGauge CReportListControl::GetViewModeGlyphGauge( DWORD listViewMode )
{
	switch ( listViewMode )
	{
		case LV_VIEW_ICON:
		case LV_VIEW_TILE:
			return ui::LargeGlyph;
		default:
			ASSERT( false );
		case LV_VIEW_DETAILS:
		case LV_VIEW_SMALLICON:
		case LV_VIEW_LIST:
			return ui::SmallGlyph;
	}
}

bool CReportListControl::SetCompactIconSpacing( int iconEdgeWidth /*= IconViewEdgeX*/ )
{
	CSize spacing( iconEdgeWidth * 2, 0 );
	if ( m_pLargeImageList != nullptr )
		spacing.cx += gdi::GetImageIconSize( *m_pLargeImageList ).cx;
	else if ( m_pImageList != nullptr )
		spacing.cx += gdi::GetImageIconSize( *m_pImageList ).cx;
	else
		return false;

	SetIconSpacing( spacing );
	return true;
}

int CReportListControl::GetTopIndex( void ) const
{
	switch ( GetView() )
	{
		case LV_VIEW_DETAILS:
		case LV_VIEW_LIST:
			return __super::GetTopIndex();			// only works in report/list view mode
		case LV_VIEW_ICON:
		case LV_VIEW_SMALLICON:
		case LV_VIEW_TILE:
			break;
		default: ASSERT( false );
	}

	UINT flags;
	return HitTest( CPoint( 3, 3 ), &flags );
}

void CReportListControl::SetTopIndex( int topIndex )
{
	// scroll for the last item in the list, then to topIndex
	int count = GetItemCount();
	if ( count > 1 )
	{
		SetRedraw( FALSE );
		EnsureVisible( count - 1, FALSE );
		EnsureVisible( topIndex, FALSE );
		SetRedraw( TRUE );
		Invalidate();
	}

/* alternative impl:

	int actualTopIndex = GetTopIndex();
	int horzSpacing;
	int lineHeight;
	GetItemSpacing( TRUE, &horzSpacing, &lineHeight );

	CSize scrollsize( 0, ( topIndex - actualTopIndex ) * lineHeight );
	Scroll( scrollsize );

OR:
	EnsureVisible( topIndex, FALSE );
	CRect itemRect;
	if ( GetItemRect( topIndex, &itemRect, LVIR_BOUNDS ) )
	{
		CSize size( 0, itemRect.Height() * topIndex - GetTopIndex() );
		if ( topIndex != GetTopIndex() )
			Scroll( size );
	}
*/
}

int CReportListControl::HitTest( CPoint point, UINT* pFlags /*= nullptr*/, TGroupId* pGroupId /*= nullptr*/ ) const
{
	REQUIRE( nullptr == pGroupId || pFlags != nullptr );
	UINT hitFlags;
	int itemIndex = __super::HitTest( point, &hitFlags );

	if ( HasFlag( hitFlags, LVHT_NOWHERE ) )
	{
		if ( int itemCount = GetItemCount() )
		{
			CRect lastItemRect;
			GetItemRect( itemCount - 1, &lastItemRect, LVIR_BOUNDS );

			if ( point.y >= lastItemRect.bottom )		// note: use >= since when equals to bottom it returns index of -1 (no hit)
				SetFlag( hitFlags, LVHT_MY_PASTEND );
			else
				switch ( GetView() )
				{
					case LV_VIEW_ICON:
					case LV_VIEW_SMALLICON:
					case LV_VIEW_TILE:
						if ( !EqFlag( GetStyle(), LVS_ALIGNLEFT ) )
							if ( point.x > lastItemRect.right && point.y > lastItemRect.top )		// to the right of last item?
								SetFlag( hitFlags, LVHT_MY_PASTEND );
				}
		}
		else
			SetFlag( hitFlags, LVHT_MY_PASTEND );

		if ( pGroupId != nullptr )
		{
			*pGroupId = GroupHitTest( point );

			if ( *pGroupId != -1 )
				SetFlag( hitFlags, LVHT_EX_GROUP_HEADER );
		}
	}

	if ( pFlags != nullptr )
		*pFlags = hitFlags;

	return itemIndex;
}

CListTraits::TGroupId CReportListControl::GroupHitTest( const CPoint& point, int groupType /*= LVGGR_HEADER*/ ) const
{
#ifdef UTL_VISTA_GROUPS
	if ( IsGroupViewEnabled() || -1 == __super::HitTest( point ) )
		for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
		{
			int groupId = GetGroupId( i );
			CRect rect( 0, LVGGR_HEADER, 0, 0 );
			VERIFY( GetGroupRect( groupId, &rect, groupType ) );
			if ( rect.PtInRect( point ) )
				return groupId;
		}
#else
	point;
#endif //UTL_VISTA_GROUPS

	return -1;			// don't try other ways to find the group
}

int CReportListControl::GetDropIndexAtPoint( const CPoint& point ) const
{
	if ( this == WindowFromPoint( ui::ClientToScreen( m_hWnd, point ) ) )
	{
		UINT hitFlags;
		int newDropIndex = HitTest( point, &hitFlags );
		if ( HasFlag( hitFlags, LVHT_MY_PASTEND ) )
			newDropIndex = GetItemCount();				// drop past last item
		return newDropIndex;
	}
	return -1;
}

bool CReportListControl::IsItemFullyVisible( int index ) const
{
	// more accurate than CListCtrl::IsItemVisible();
	CRect itemRect, clientRect;
	GetItemRect( index, &itemRect, LVIR_BOUNDS );
	GetClientRect( &clientRect );

	CRect clipItemRect = itemRect;
	clipItemRect &= clientRect;
	return ( clipItemRect == itemRect ) != FALSE;		// true if item is fully visible
}

bool CReportListControl::GetIconItemRect( CRect* pIconRect, int index ) const
{
	ASSERT_PTR( pIconRect );

	if ( !GetItemRect( index, pIconRect, LVIR_ICON ) )
		return false;				// item not visible

	if ( ( pIconRect->Height() - 1 ) == pIconRect->Width() )
		--pIconRect->bottom;		// reduce height since it has one extra pixel at the bottom (16x17); this allows proper icon drawing that fits vertically

	if ( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) )
		switch ( GetView() )
		{
			case LV_VIEW_ICON:
			case LV_VIEW_SMALLICON:
			case LV_VIEW_TILE:
				pIconRect->left += GetStateIconSize().cx + 5;
		}

	return true;
}

void CReportListControl::SetSortByColumn( TColumn sortByColumn, bool sortAscending /*= true*/ )
{
	m_sortByColumn = sortByColumn;
	m_sortAscending = sortAscending;

	if ( m_hWnd != nullptr && IsSortingEnabled() )
		SortList();
}

void CReportListControl::StoreSortByColumn( TColumn sortByColumn, bool sortAscending /*= true*/ )
{
	m_sortByColumn = sortByColumn;
	m_sortAscending = sortAscending;

	if ( m_hWnd != nullptr && IsSortingEnabled() )
		UpdateColumnSortHeader();
}

void CReportListControl::UpdateColumnSortHeader( void )
{
	if ( IsSortingEnabled() )
	{
		CHeaderCtrl* pColumnHeader = GetHeaderCtrl();
		ASSERT_PTR( pColumnHeader );

		for ( int i = 0, columnCount = pColumnHeader->GetItemCount(); i != columnCount; ++i )
		{
			HDITEM headerItem;

			headerItem.mask = HDI_FORMAT;
			headerItem.fmt = HDF_STRING;

			if ( i == m_sortByColumn )
				headerItem.fmt |= ( m_sortAscending ? HDF_SORTUP : HDF_SORTDOWN );

			VERIFY( pColumnHeader->SetItem( i, &headerItem ) );
		}
	}
}

void CReportListControl::SortList( void )
{
	UpdateColumnSortHeader();

	ui::CNmHdr nmHdr( this, lv::LVN_CustomSortList );

	if ( 0L == nmHdr.NotifyParent() )			// give parent a chance to custom sort the list (or sort its groups); still sort items by default?
		if ( -1 == m_sortByColumn && !m_initialItemsOrder.empty() )				// default sorting and we have an initial order?
		{
			SortItems( (PFNLVCOMPARE)&InitialOrderCompareProc, (LPARAM)this );	// restore initial item order; passes item LPARAMs (i.e. TRowKey) as left/right

			if ( IsGroupViewEnabled() )
				SortGroups( (PFNLVGROUPCOMPARE)&InitialGroupOrderCompareProc, this );
		}
		else if ( GetSortInternally() )											// otherwise was sorted externally, just update sort header
			if ( m_pComparePtrFunc != nullptr )
				SortItems( m_pComparePtrFunc, (LPARAM)this );					// passes item LPARAMs as left/right
			else
				SortItemsEx( (PFNLVCOMPARE)&TextCompareProc, (LPARAM)this );	// passes item indexes as left/right LPARAMs

	nmHdr.code = lv::LVN_ListSorted;
	nmHdr.NotifyParent();

	int caretIndex = GetCaretIndex();
	if ( caretIndex != -1 )
		EnsureVisible( caretIndex, FALSE );
}

void CReportListControl::InitialSortList( void )
{
	REQUIRE( IsSortingEnabled() );

	// store initial items order so that we can restore it when list will be switched to unsorted
	const unsigned int itemCount = GetItemCount();

	m_initialItemsOrder.clear();
	m_initialItemsOrder.reserve( itemCount );

	for ( unsigned int index = 0; index != itemCount; ++index )
		m_initialItemsOrder[ MakeRowKeyAt( index ) ] = index;

	if ( m_sortByColumn != -1 )			// has a previously saved column sort order?
		SortList();
}

void CReportListControl::AddColumnCompare( TColumn column, const pred::IComparator* pComparator, bool defaultAscending /*= true*/ )
{
	// note: pComparator could be NULL for custom sorting
	ASSERT( column >= 0 || EntireRecord == column );

	if ( nullptr == m_pComparePtrFunc )
		SetCompareFunc( (PFNLVCOMPARE)&ObjectCompareProc );

	m_comparators.push_back( CColumnComparator( column, defaultAscending, pComparator ) );
}

const pred::IComparator* CReportListControl::FindCompare( TColumn column ) const
{
	for ( std::vector<CColumnComparator>::const_iterator itColComparator = m_comparators.begin(); itColComparator != m_comparators.end(); ++itColComparator )
		if ( column == itColComparator->m_column )
			return itColComparator->m_pComparator;

	return nullptr;
}

bool CReportListControl::IsDefaultAscending( TColumn column ) const
{
	for ( std::vector<CColumnComparator>::const_iterator itColComparator = m_comparators.begin(); itColComparator != m_comparators.end(); ++itColComparator )
		if ( column == itColComparator->m_column )
			return itColComparator->m_defaultAscending;

	return true;
}

pred::CompareResult CALLBACK CReportListControl::ObjectCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl )
{
	ASSERT_PTR( pListCtrl );
	// called by SortItems() that passes item LPARAMs as left/right
	return pListCtrl->CompareSubItems( AsPtr<utl::ISubject>( leftParam ), AsPtr<utl::ISubject>( rightParam ) );
}

pred::CompareResult CReportListControl::CompareSubItems( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const
{
	if ( const pred::IComparator* pComparator = FindCompare( m_sortByColumn ) )
	{
		pred::CompareResult result = pComparator->CompareObjects( pLeft, pRight );
		if ( result != pred::Equal )
			return pred::GetResultInOrder( result, m_sortAscending );
	}
	else
	{	// default sub-item text comparison
		pred::CompareResult result = CompareSubItemsText( FindItemIndex( (LPARAM)pLeft ), FindItemIndex( (LPARAM)pRight ), m_sortByColumn );
		if ( result != pred::Equal )
			return pred::GetResultInOrder( result, m_sortAscending );
	}

	if ( const pred::IComparator* pRecordComparator = FindCompare( EntireRecord ) )		// default record comparator
	{
		pred::CompareResult result = pRecordComparator->CompareObjects( pLeft, pRight );
		if ( result != pred::Equal )
			return result;					// never use DESC order for default comparison
	}

	return CompareSubItemsByIndex( FindItemIndex( (LPARAM)pLeft ), FindItemIndex( (LPARAM)pRight ) );
}

pred::CompareResult CALLBACK CReportListControl::InitialOrderCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl )
{
	ASSERT_PTR( pListCtrl );
	// called by SortItems() that passes item LPARAMs as left/right
	return pListCtrl->CompareInitialOrder( static_cast<TRowKey>( leftParam ), static_cast<TRowKey>( rightParam ) );
}

pred::CompareResult CReportListControl::CompareInitialOrder( TRowKey leftKey, TRowKey rightKey ) const
{
	REQUIRE( -1 == m_sortByColumn );			// unsorted: it means restore initial order

	if ( const int* pLeftIndex = utl::FindValuePtr( m_initialItemsOrder, leftKey ) )
		if ( const int* pRightIndex = utl::FindValuePtr( m_initialItemsOrder, rightKey ) )
			return pred::ToCompareResult( *pLeftIndex - *pRightIndex );

	ASSERT( false );			// item not accounted for in m_initialItemsOrder
	return pred::Equal;
}

pred::CompareResult CALLBACK CReportListControl::InitialGroupOrderCompareProc( int leftGroupId, int rightGroupId, CReportListControl* /*pThis*/ )
{
	return pred::Compare_Scalar( leftGroupId, rightGroupId );		// groupID corresponds to the initial group order
}

int CALLBACK CReportListControl::TextCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl )
{
	ASSERT_PTR( pListCtrl );
	return pListCtrl->CompareSubItemsByIndex( static_cast<UINT>( leftParam ), static_cast<UINT>( rightParam ) );
}

pred::CompareResult CReportListControl::CompareSubItemsByIndex( UINT leftIndex, UINT rightIndex ) const
{
	REQUIRE( m_sortByColumn != -1 );		// must be sorted by a column

	pred::CompareResult result = pred::GetResultInOrder( CompareSubItemsText( leftIndex, rightIndex, m_sortByColumn ), m_sortAscending );

	enum { CodeColumn = 0 };				// first column, the one with the icon

	if ( pred::Equal == result && m_sortByColumn != CodeColumn )				// equal by a secondary column?
		result = CompareSubItemsText( leftIndex, rightIndex, CodeColumn );		// compare by "code" column (first column); never use DESC order for default comparison

	return result;
}

pred::CompareResult CReportListControl::CompareSubItemsText( UINT leftIndex, UINT rightIndex, TColumn byColumn ) const
{
	REQUIRE( byColumn != -1 );		// valid column?
	ASSERT( leftIndex != -1 && rightIndex != -1 );

	CString leftText = GetItemText( leftIndex, byColumn );
	CString rightText = GetItemText( rightIndex, byColumn );

	return str::IntuitiveCompare( leftText.GetString(), rightText.GetString() );
}

int CReportListControl::FindItemIndex( const std::tstring& itemText ) const
{
	LVFINDINFO findInfo;
	findInfo.flags = LVFI_STRING | LVFI_WRAP;
	findInfo.psz = itemText.c_str();
	return FindItem( &findInfo );
}

int CReportListControl::FindItemIndex( LPARAM lParam ) const
{
	ASSERT( lParam != 0 );

	LVFINDINFO findInfo;
	findInfo.flags = LVFI_PARAM | LVFI_WRAP;
	findInfo.lParam = lParam;
	return FindItem( &findInfo );
}

bool CReportListControl::EnsureVisibleObject( const utl::ISubject* pObject )
{
	int foundIndex = FindItemIndex( (LPARAM)pObject );
	return foundIndex != -1 && EnsureVisible( foundIndex, FALSE );
}

void CReportListControl::QueryItemsText( std::vector<std::tstring>& rItemsText, const std::vector<int>& indexes, int subItem /*= 0*/ ) const
{
	rItemsText.reserve( rItemsText.size() + indexes.size() );

	for ( std::vector<int>::const_iterator itIndex = indexes.begin(); itIndex != indexes.end(); ++itIndex )
		rItemsText.push_back( GetItemText( *itIndex, subItem ).GetString() );
}

void CReportListControl::QuerySelectedItemsText( std::vector<std::tstring>& rItemsText, int subItem /*= 0*/ ) const
{
	std::vector<int> selIndexes;
	int caretIndex;
	GetSelection( selIndexes, &caretIndex );
	QueryItemsText( rItemsText, selIndexes, subItem );
}

void CReportListControl::QueryAllItemsText( std::vector<std::tstring>& rItemsText, int subItem /*= 0*/ ) const
{
	size_t itemCount = GetItemCount();
	rItemsText.clear();
	rItemsText.reserve( itemCount );

	for ( UINT i = 0; i != itemCount; ++i )
		rItemsText.push_back( GetItemText( i, subItem ).GetString() );
}

bool CReportListControl::IsStateChangeNotify( const NMLISTVIEW* pNmListView, UINT selMask )
{
	ASSERT_PTR( pNmListView );

	if ( HasFlag( pNmListView->uChanged, LVIF_STATE ) )										// item state has been changed?
		if ( StateChanged( pNmListView->uNewState, pNmListView->uOldState, selMask ) )		// state changed according to the mask?
			return true;

	return false;
}

void CReportListControl::SetLayoutInfo( UINT columnLayoutId )
{
	REQUIRE( columnLayoutId != 0 );
	m_columnLayoutId = columnLayoutId;
	SetLayoutInfo( str::LoadStrings( columnLayoutId ) );
}

void CReportListControl::SetLayoutInfo( const std::vector<std::tstring>& columnSpecs )
{
	if ( m_hWnd != nullptr )
		DeleteAllColumns();

	ParseColumnLayout( m_columnInfos, columnSpecs );

	if ( m_hWnd != nullptr )
		InsertAllColumns();
}

void CReportListControl::ParseColumnLayout( std::vector<CColumnInfo>& rColumnInfos, const std::vector<std::tstring>& columnSpecs )
{
	ASSERT( !columnSpecs.empty() );

	/*	format examples:
			"Label"
			"Label=120" - 120 is the default column width;
			"Label=-1" - stretchable column width;
			"Label=-1/50" - stretchable column width, 50 minimum stretchable width;
			"Label=120>" - 120 default column width, right align;
			"Label=120<>" - 120 default column width, center align;
		if -1 is used (just for one column), that column stretches to the available column space.
	*/

	rColumnInfos.clear();
	rColumnInfos.resize( columnSpecs.size() );

	for ( UINT i = 0; i != columnSpecs.size(); ++i )
	{
		const std::tstring& columnSpec = columnSpecs[ i ];
		CColumnInfo& rColInfo = rColumnInfos[ i ];

		str::TStringRange specRange( columnSpec );
		Range<size_t> sepPos;
		if ( specRange.Find( sepPos, _T('=') ) )
		{
			rColInfo.m_label = specRange.ExtractLead( sepPos.m_start );
			specRange.RefPos().m_start = sepPos.m_end;

			size_t skipLength;
			if ( num::ParseNumber( rColInfo.m_defaultWidth, specRange.GetStartPtr(), &skipLength ) )
				specRange.RefPos().m_start += skipLength;

			if ( specRange.StripPrefix( _T('/') ) )
				if ( !num::ParseNumber( rColInfo.m_minWidth, specRange.GetStartPtr(), &skipLength ) )
					ASSERT( false );

			if ( specRange.StripPrefix( _T("<>") ) )
				rColInfo.m_alignment = LVCFMT_CENTER;
			else if ( specRange.StripPrefix( _T('>') ) )
				rColInfo.m_alignment = LVCFMT_RIGHT;
			else if ( specRange.StripPrefix( _T('<') ) )
				rColInfo.m_alignment = LVCFMT_LEFT;
		}
		else
		{
			rColInfo.m_label = columnSpec;
			rColInfo.m_defaultWidth = ColumnWidth_AutoSize;
		}
	}
}

unsigned int CReportListControl::GetColumnCount( void ) const
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	return pHeaderCtrl != nullptr ? pHeaderCtrl->GetItemCount() : 0;
}

void CReportListControl::ResetColumnLayout( void )
{
	CScopedLockRedraw freeze( this );

	std::vector<std::tstring> columnSpecs = str::LoadStrings( m_columnLayoutId );
	ParseColumnLayout( m_columnInfos, columnSpecs );

	SetupColumnLayout( nullptr );
}

void CReportListControl::DeleteAllColumns( void )
{
	if ( CHeaderCtrl* pHeader = GetHeaderCtrl() )
		for ( int i = pHeader->GetItemCount() - 1; i >= 0; --i )
			VERIFY( DeleteColumn( i ) != -1 );
}

void CReportListControl::InsertAllColumns( void )
{
	if ( !HasLayoutInfo() )
	{
		ASSERT( false );
		return;
	}

	LVCOLUMN columnInfo;
	ZeroMemory( &columnInfo, sizeof( columnInfo ) );
	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER;
	columnInfo.fmt = LVCFMT_LEFT;

	CScopedInternalChange internalChange( this );

	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
	{
		columnInfo.pszText = (TCHAR*)m_columnInfos[ i ].m_label.c_str();
		columnInfo.fmt = m_columnInfos[ i ].m_alignment;
		columnInfo.iOrder = i;

		int columnWidth = m_columnInfos[ i ].m_defaultWidth;

		if ( ColumnWidth_AutoSize == columnWidth )
			columnWidth = ColumnSpacing + GetStringWidth( m_columnInfos[ i ].m_label.c_str() );
		else if ( columnWidth < 0 )
			columnWidth = LVSCW_AUTOSIZE_USEHEADER;

		columnInfo.cx = columnWidth;
		VERIFY( InsertColumn( i, &columnInfo ) != -1 );
	}
	PostColumnLayout();
}

void CReportListControl::SetupColumnLayout( const std::vector<std::tstring>* pRegColumnLayoutItems )
{
	if ( !HasLayoutInfo() )
		return;

	bool insertMode = 0 == GetColumnCount();
	bool ignoreSavedLayout = !insertMode || nullptr == pRegColumnLayoutItems || pRegColumnLayoutItems->size() != m_columnInfos.size();

	LVCOLUMN columnInfo;
	ZeroMemory( &columnInfo, sizeof( columnInfo ) );
	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER;

	CScopedInternalChange internalChange( this );

	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
	{
		columnInfo.pszText = (TCHAR*)m_columnInfos[ i ].m_label.c_str();
		columnInfo.fmt = m_columnInfos[ i ].m_alignment;
		columnInfo.iOrder = i;

		int columnWidth = m_columnInfos[ i ].m_defaultWidth;

		if ( ColumnWidth_AutoSize == columnWidth )
			columnWidth = ColumnSpacing + GetStringWidth( m_columnInfos[ i ].m_label.c_str() );
		else if ( columnWidth < 0 )
			columnWidth = LVSCW_AUTOSIZE_USEHEADER;		// will stretch it later

		columnInfo.cx = columnWidth;

		// superimpose saved column layout from registry
		if ( !ignoreSavedLayout )
		{
			const std::tstring& columnLayout = pRegColumnLayoutItems->at( i );
			if ( !columnLayout.empty() )
				_stscanf( columnLayout.c_str(), s_fmtRegColumnLayout, &columnInfo.cx, &columnInfo.iOrder );
		}

		if ( insertMode )
			VERIFY( InsertColumn( i, &columnInfo ) != -1 );
		else
			VERIFY( SetColumn( i, &columnInfo ) );
	}
	PostColumnLayout();
}

void CReportListControl::PostColumnLayout( void )
{
	ResizeFlexColumns();

	if ( !m_tileColumns.empty() )
	{	// init the number of extra text lines (columns) to be shown in tile view mode (LV_VIEW_TILE)
		LVTILEVIEWINFO tileViewInfo = { sizeof( LVTILEVIEWINFO ) };
		tileViewInfo.dwMask = LVTVIM_COLUMNS;
		tileViewInfo.dwFlags = LVTVIF_AUTOSIZE;
		tileViewInfo.cLines = static_cast<int>( m_tileColumns.size() );		// besides the Code column

		SetTileViewInfo( &tileViewInfo );
	}
}

CReportListControl& CReportListControl::AddTileColumn( UINT tileColumn )
{
	ASSERT( tileColumn > Code );					// 'Code' sub-item is always displayed
	m_tileColumns.push_back( tileColumn );
	return *this;
}

bool CReportListControl::ResizeFlexColumns( void )
{
	if ( nullptr == m_hWnd )
		return false;
	if ( 0 == GetColumnCount() )
		return false;			// columns not initialized yet

	LVCOLUMN columnInfo;
	ZeroMemory( &columnInfo, sizeof( columnInfo ) );
	columnInfo.mask = LVCF_WIDTH;

	UINT totalFixedWidth = 0, stretchableCount = 0;

	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
		if ( m_columnInfos[ i ].m_defaultWidth < 0 )
			++stretchableCount;
		else
		{
			VERIFY( GetColumn( i, &columnInfo ) );
			totalFixedWidth += columnInfo.cx;
		}

	if ( 0 == stretchableCount )
		return false;

	// set flexible column widths
	CRect clientRect;
	GetClientRect( &clientRect );

	const int stretchableWidth = clientRect.Width() - totalFixedWidth, partWidth = stretchableWidth / stretchableCount;

	for ( UINT i = 0, s = 0, remainderWidth = stretchableWidth; i != m_columnInfos.size(); ++i )
	{
		const CColumnInfo& colInfo = m_columnInfos[ i ];
		int defaultWidth = colInfo.m_defaultWidth;
		if ( defaultWidth < 0 )
		{
			int fitWidth;
			if ( ColumnWidth_StretchToFit == defaultWidth )		// -1: take equal parts of the stretchable width
				fitWidth = partWidth;
			else												// -pct: take a percentage of the stretchable width
			{
				defaultWidth = -defaultWidth;
				ASSERT( defaultWidth <= 100 );
				fitWidth = MulDiv( stretchableWidth, defaultWidth, 100 );
			}

			if ( ++s == stretchableCount )
				fitWidth = remainderWidth;		// last flex column takes all the remainder width

			fitWidth = std::max<int>( colInfo.m_minWidth, fitWidth );
			SetColumnWidth( i, fitWidth );

			remainderWidth -= fitWidth;
		}
	}

	return true;
}

void CReportListControl::OnFinalReleaseInternalChange( void )
{
	ResizeFlexColumns();		// layout flexible columns after list content has changed
}

void CReportListControl::InputColumnLayout( std::vector<std::tstring>& rRegColumnLayoutItems )
{
	if ( !HasLayoutInfo() )
	{
		ASSERT( HasLayoutInfo() );
		return;
	}

	// input columns layout (width and order)
	LVCOLUMN columnInfo;
	ZeroMemory( &columnInfo, sizeof( columnInfo ) );
	columnInfo.mask = LVCF_WIDTH | LVCF_ORDER;

	rRegColumnLayoutItems.resize( m_columnInfos.size() );
	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
	{
		VERIFY( GetColumn( i, &columnInfo ) );
		rRegColumnLayoutItems[ i ] = str::Format( s_fmtRegColumnLayout, columnInfo.cx, columnInfo.iOrder ).c_str();
	}
}

bool CReportListControl::LoadFromRegistry( void )
{
	ASSERT( HasLayoutInfo() && !m_regSection.empty() );

	std::vector<std::tstring> regColumnLayoutItems;
	regColumnLayoutItems.reserve( m_columnInfos.size() );

	CWinApp* pApp = AfxGetApp();

	// check against the last column saved column count
	bool validSavedState = m_columnInfos.size() == pApp->GetProfileInt( m_regSection.c_str(), _T(""), 0 );

	// load layout entries if column layout hasn't changed since last save
	if ( validSavedState )
		for ( UINT i = 0; i != m_columnInfos.size(); ++i )
		{
			std::tstring columnLayout = pApp->GetProfileString( m_regSection.c_str(), m_columnInfos[ i ].m_label.c_str() ).GetString();
			regColumnLayoutItems.push_back( columnLayout );
		}

	SetupColumnLayout( &regColumnLayoutItems );

	if ( validSavedState )
	{
		DWORD oldViewMode = GetView(), viewMode = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_viewMode, oldViewMode );
		if ( viewMode != oldViewMode )
			ChangeListViewMode( viewMode );

		DWORD oldStackingStyle = GetStyle() & LVS_ALIGNMASK, stackingStyle = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_viewStacking, oldStackingStyle );
		if ( stackingStyle != oldStackingStyle )
			ModifyStyle( LVS_ALIGNMASK, stackingStyle );

		if ( IsSortingEnabled() && GetPersistSorting() )
		{
			m_sortByColumn = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_sortByColumn, m_sortByColumn );
			m_sortAscending = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_sortAscending, m_sortAscending ) != FALSE;
		}
	}

	return validSavedState;
}

void CReportListControl::SaveToRegistry( void )
{
	ASSERT( HasLayoutInfo() && !m_regSection.empty() );

	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt( m_regSection.c_str(), _T(""), (int)m_columnInfos.size() );	// saved column count

	std::vector<std::tstring> regColumnLayoutItems;
	InputColumnLayout( regColumnLayoutItems );
	ASSERT( regColumnLayoutItems.size() == m_columnInfos.size() );

	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
		pApp->WriteProfileString( m_regSection.c_str(), m_columnInfos[ i ].m_label.c_str(), regColumnLayoutItems[ i ].c_str() );

	pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_viewMode, GetView() );
	pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_viewStacking, GetStyle() & LVS_ALIGNMASK );

	if ( IsSortingEnabled() && GetPersistSorting() )
	{
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_sortByColumn, m_sortByColumn );
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_sortAscending, m_sortAscending );
	}
}

utl::ISubject* CReportListControl::GetSubjectAt( int index ) const
{
	if ( utl::ISubject* pObject = ToSubject( GetItemData( index ) ) )
		if ( !m_subjectBased )
			return nullptr;
		else
		{
			pObject = dynamic_cast<utl::ISubject*>( pObject );
			ASSERT_PTR( pObject );			// really a dynamic subject?
			return pObject;
		}

	return nullptr;
}

int CReportListControl::InsertObjectItem( int index, const utl::ISubject* pObject, int imageIndex /*= ui::No_Image*/, const TCHAR* pText /*= nullptr*/ )
{
	std::tstring displayCode;
	if ( pObject != nullptr )
	{
		if ( nullptr == pText )
		{
			displayCode = FormatCode( pObject );
			pText = displayCode.c_str();
		}

		if ( !m_subjectBased )
			m_subjectBased = true;
	}

	if ( ui::Transparent_Image == imageIndex )
		imageIndex = m_pCustomImager.get() != nullptr ? m_pCustomImager->GetTranspImageIndex() : 0;

	int insertIndex = InsertItem( LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, index, pText, 0, 0, imageIndex, reinterpret_cast<LPARAM>( pObject ) );
	ASSERT( insertIndex != -1 );

	// set by InsertItem()
	//SetItemData( insertIndex, reinterpret_castDWORD_PTR >( pObject ) );
	SetItemTileInfo( insertIndex );
	return insertIndex;
}

void CReportListControl::SetSubItemTextPtr( int index, int subItem, const TCHAR* pText /*= LPSTR_TEXTCALLBACK*/, int imageIndex /*= ui::No_Image*/ )
{
	ASSERT( subItem > 0 );

	if ( ui::Transparent_Image == imageIndex )
		imageIndex = m_pCustomImager.get() != nullptr ? m_pCustomImager->GetTranspImageIndex() : 0;
	else if ( str::IsEmpty( pText ) )
		imageIndex = ui::No_Image;			// clear the image for empty text

	VERIFY( SetItem( index, subItem, LVIF_TEXT | LVIF_IMAGE, pText, imageIndex, 0, 0, 0 ) );
}

void CReportListControl::SetSubItemImage( int index, int subItem, int imageIndex )
{
	VERIFY( SetItem( index, subItem, LVIF_IMAGE, nullptr, imageIndex, 0, 0, 0 ) );
}

bool CReportListControl::SetItemTileInfo( int index )
{
	if ( m_tileColumns.empty() )
		return false;

	LVTILEINFO tileInfo = { sizeof( LVTILEINFO ) };
	tileInfo.iItem = index;
	tileInfo.cColumns = static_cast<UINT>( m_tileColumns.size() );
	tileInfo.puColumns = &m_tileColumns.front();
	tileInfo.piColFmt = nullptr;

	return SetTileInfo( &tileInfo ) != FALSE;
}

void CReportListControl::StoreDispInfoItemText( NMLVDISPINFO* pDispInfo, const std::tstring& text )
{
	ASSERT_PTR( pDispInfo );
	_tcsncpy( pDispInfo->item.pszText, text.c_str(), pDispInfo->item.cchTextMax );
}

void CReportListControl::GetItemDataAt( CItemData& rItemData, int index ) const
{
	size_t columnCount = std::max( m_columnInfos.size(), (size_t)1 );

	rItemData.m_subItems.reserve( columnCount );
	rItemData.m_state = GetItemState( index, UINT_MAX );
	rItemData.m_lParam = GetItemData( index );

	for ( UINT subItem = 0; subItem != columnCount; ++subItem )
	{
		LVITEM item;
		memset( &item, 0, sizeof( item ) );
		item.mask = LVIF_IMAGE;
		item.iItem = index;
		item.iSubItem = subItem;
		VERIFY( GetItem( &item ) );

		rItemData.m_subItems.push_back( std::make_pair( std::tstring( GetItemText( index, subItem ).GetString() ), item.iImage ) );
	}
}

void CReportListControl::SetItemDataAt( const CItemData& itemData, int index )
{
	SetItem( index, 0, LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE,
			 itemData.m_subItems[ 0 ].first.c_str(), itemData.m_subItems[ 0 ].second,
			 itemData.m_state, UINT_MAX, itemData.m_lParam );

	size_t columnCount = std::max( m_columnInfos.size(), (size_t)1 );

	for ( UINT subItem = 1; subItem != columnCount; ++subItem )
		SetItem( index, subItem, LVIF_TEXT | LVIF_IMAGE,
				 itemData.m_subItems[ subItem ].first.c_str(), itemData.m_subItems[ subItem ].second,
				 0, 0, 0 );
}

void CReportListControl::InsertItemDataAt( const CItemData& itemData, int index )
{
	int insIndex = InsertItem( LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE, index,
							   itemData.m_subItems[ Code ].first.c_str(),
							   itemData.m_state, UINT_MAX,
							   itemData.m_subItems[ Code ].second,
							   itemData.m_lParam );

	ENSURE( index == insIndex ); insIndex;

	SetItemState( index, itemData.m_state, LVIS_STATEIMAGEMASK );		// copy the checked state, since InsertItem doesn't do that

	size_t columnCount = std::max( m_columnInfos.size(), (size_t)1 );

	for ( UINT subItem = 1; subItem != columnCount; ++subItem )
		SetItem( index, subItem, LVIF_TEXT | LVIF_IMAGE,
				 itemData.m_subItems[ subItem ].first.c_str(), itemData.m_subItems[ subItem ].second,
				 0, 0, 0 );
}

const CReportListControl::CLabelEdit* CReportListControl::EditLabelModal( int index )
{
	m_pLabelEdit.reset();

	HWND hEdit = EditLabel( index )->GetSafeHwnd();
	MSG msg;

	while ( ui::IsValidWindow( hEdit ) && m_pLabelEdit.get() != nullptr && !m_pLabelEdit->m_done )
		if ( ::PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) )
			if ( !AfxGetThread()->PumpMessage() )
			{
				::PostQuitMessage( 0 );
				return nullptr;
			}

	return m_pLabelEdit.get();
}

void CReportListControl::SwapItems( int index1, int index2 )
{
	ASSERT( index1 != index2 );

	CItemData data1, data2;

	GetItemDataAt( data1, index1 );
	GetItemDataAt( data2, index2 );

	SetItemDataAt( data1, index2 );
	SetItemDataAt( data2, index1 );
}

void CReportListControl::DropMoveItems( int destIndex, const std::vector<int>& selIndexes )
{
	// assume that selIndexes are pre-sorted
	ASSERT( destIndex >= 0 && destIndex <= GetItemCount() );

	std::vector<CItemData> datas;
	datas.resize( selIndexes.size() );

	for ( size_t i = selIndexes.size(); i-- != 0; )
	{
		int index = selIndexes[ i ];

		GetItemDataAt( datas[ i ], index );
		DeleteItem( index );
		if ( index < destIndex )
			--destIndex;
	}

	for ( std::vector<CItemData>::const_iterator itItem = datas.begin(); itItem != datas.end(); ++itItem, ++destIndex )
		InsertItemDataAt( *itItem, destIndex );

	ForceRearrangeItems();
}

bool CReportListControl::ForceRearrangeItems( void )
{
	// Nasty problem: in all icon views, the list does not reflect the current order after DeleteItem() and InsertItem() reordering.
	// This only works well in report view mode.
	// Workaround: switch to report view and than back to current view mode so that the items are rearranges, i.e. displayed in the current order.
	//
	DWORD viewMode = GetView();
	if ( LV_VIEW_DETAILS == viewMode )
		return false;						// not necessary to re-arrange in report view

	if ( HasFlag( GetStyle(), WS_VSCROLL | WS_HSCROLL ) )
	{
		// HACK1: the most precise and effective: toggle the LVS_EX_CHECKBOXES ex style - this rearranges the items internally *retaining the current top index*
		DWORD oldCheckBoxExStyle = ListView_GetExtendedListViewStyle( m_hWnd ) & LVS_EX_CHECKBOXES;

		ListView_SetExtendedListViewStyleEx( m_hWnd, LVS_EX_CHECKBOXES, oldCheckBoxExStyle ^ LVS_EX_CHECKBOXES );		// toggle flip
		ListView_SetExtendedListViewStyleEx( m_hWnd, LVS_EX_CHECKBOXES, oldCheckBoxExStyle );
	}
	else
	{
		// HACK2: this rearranges the items but the current top index is not preserved. Note that GetTopIndex() does not work in icon view (SDK doc).
		// Works just fine when there is no scrolling in current icon view.
		SetView( LV_VIEW_DETAILS );
		SetView( viewMode );
		Arrange( LVA_DEFAULT );
	}
	return true;
}

CSize CReportListControl::GetStateIconSize( void ) const
{
	if ( 0 == m_stateIconSize.cx && 0 == m_stateIconSize.cy )
		if ( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) )
			if ( CImageList* pStateImageList = GetStateImageList() )
				m_stateIconSize = gdi::GetImageIconSize( *pStateImageList );		// cache the size of an state image list icon, used to offset glyph rect in certain view modes

	return m_stateIconSize;
}

bool CReportListControl::SetCheckState( int index, int checkState )
{
	if ( !SetRawCheckState( index, ui::CheckStateToRaw( checkState ) ) )
		return false;

	if ( m_pNmToggling.get() != nullptr )
		m_pNmToggling->AddIndex( index );		// accumulate items that have changed state during a toggle

	return true;
}

bool CReportListControl::ModifyCheckState( int index, int checkState )
{
	if ( GetCheckState( index ) == checkState )
		return false;			// checked state not modified

	SetCheckState( index, checkState );
	return true;
}

int CReportListControl::GetObjectCheckState( const utl::ISubject* pObject ) const
{
	int index = FindItemIndex( pObject );
	return index != -1 && GetCheckState( index );
}

bool CReportListControl::ModifyObjectCheckState( const utl::ISubject* pObject, int checkState )
{
	int index = FindItemIndex( pObject );
	return index != -1 && ModifyCheckState( index, checkState );
}

bool CReportListControl::IsChecked( int index ) const
{
	if ( m_pCheckStatePolicy != nullptr )
		return GetCheckStatePolicy()->IsCheckedState( GetCheckState( index ) );

	return GetCheck( index ) != FALSE;
}

void CReportListControl::SetCheckStatePolicy( const ui::ICheckStatePolicy* pCheckStatePolicy )
{
	ASSERT_NULL( m_hWnd );
	m_pCheckStatePolicy = pCheckStatePolicy;
	ModifyListStyleEx( 0, LVS_EX_CHECKBOXES );
}

void CReportListControl::ToggleCheckState( int index, int newCheckState )
{
	ASSERT_NULL( m_pNmToggling.get() );						// toggle custom check-state is non recursive (is client using a CScopedInternalChange?)
	m_pNmToggling.reset( new lv::CNmCheckStatesChanged( this ) );
	m_pNmToggling->m_itemIndexes.push_back( index );		// first index is the toggled reference

	CScopedInternalChange change( this );					// to avoid infinite recursion

	SetItemCheckState( index, newCheckState );

	// delayed notification for the item, after LVN_ITEMCHANGED has been send by the list for all items involved (multiple times for a group of radio buttons)
	ui::PostCall( this, &CReportListControl::NotifyCheckStatesChanged );
}

void CReportListControl::SetItemCheckState( int index, int checkState )
{
	ASSERT_PTR( m_pCheckStatePolicy );
	ASSERT( m_pCheckStatePolicy->IsEnabledState( checkState ) );

	if ( CheckRadio::Instance() == m_pCheckStatePolicy && CheckRadio::RadioChecked == checkState )
		if ( IsGroupViewEnabled() )
		{
			int radioGroupId = GetItemGroupId( index );
			if ( radioGroupId != -1 )						// radio button item belongs to a group?
			{
				std::vector<utl::ISubject*> radioItems;
				QueryGroupItems( radioItems, radioGroupId );

				// uncheck all other radio button items
				for ( std::vector<utl::ISubject*>::const_iterator itRadioItem = radioItems.begin(); itRadioItem != radioItems.end(); ++itRadioItem )
				{
					int radioIndex = FindItemIndex( *itRadioItem );
					if ( radioIndex != -1 && radioIndex != index )
						ModifyCheckState( radioIndex, CheckRadio::RadioUnchecked );
				}
			}
		}

	SetCheckState( index, checkState );
}

void CReportListControl::NotifyCheckStatesChanged( void )
{
	ASSERT_PTR( m_pNmToggling.get() );

	m_pNmToggling->m_nmHdr.NotifyParent();
	m_pNmToggling.reset();
}

size_t CReportListControl::ApplyCheckStateToSelectedItems( int toggledIndex, int checkState )
{
	size_t changedCount = 0;

	if ( !m_applyingCheckStateToSelectedItems )
		if ( IsSelected( toggledIndex ) && IsMultiSelectionList() )		// toggledIndex is part of multiple selection?
			if ( ui::IsCheckBoxState( checkState, m_pCheckStatePolicy ) )
			{
				CScopedInternalChange change( this );
				CScopedValue<bool> scopedApplyingToSel( &m_applyingCheckStateToSelectedItems, true );

				for ( POSITION pos = GetFirstSelectedItemPosition(); pos != nullptr; )
				{
					int index = GetNextSelectedItem( pos );

					if ( index != toggledIndex )						// exclude the toggled item
						if ( ui::IsCheckBoxState( GetCheckState( index ), m_pCheckStatePolicy ) )
							if ( ModifyCheckState( index, checkState ) )
								++changedCount;
				}
			}

	return changedCount;
}

bool CReportListControl::SetCaretIndex( int index, bool doSet /*= true*/ )
{
	if ( !SetItemState( index, doSet ? LVIS_FOCUSED : 0, LVIS_FOCUSED ) )
		return false;

	if ( doSet )
		EnsureVisible( index, FALSE );
	return true;
}

bool CReportListControl::SingleSelected( void ) const
{
	int selCount = 0;
	for ( POSITION pos = GetFirstSelectedItemPosition(); pos != nullptr && selCount < 2; ++selCount )
		GetNextSelectedItem( pos );

	return 1 == selCount;
}

int CReportListControl::GetCurSel( void ) const
{
	int selIndex = -1;
	if ( IsMultiSelectionList() )
	{
		selIndex = FindItemWithState( LVNI_SELECTED | LVNI_FOCUSED );
		if ( -1 == selIndex )
			selIndex = FindItemWithState( LVNI_FOCUSED );
	}
	else
	{
		selIndex = FindItemWithState( LVNI_SELECTED );
		if ( -1 == selIndex )
			selIndex = FindItemWithState( LVNI_FOCUSED );
	}

	return selIndex;
}

bool CReportListControl::SetCurSel( int index, bool doSelect /*= true*/ )
{
	if ( -1 == index )
	{
		ClearSelection();
		return false;
	}

	if ( !SetItemState( index, doSelect ? ( LVIS_SELECTED | LVIS_FOCUSED ) : 0, LVIS_SELECTED | LVIS_FOCUSED ) )
		return false;

	if ( doSelect )
		EnsureVisible( index, FALSE );
	return true;
}

int CReportListControl::GetSelCaretIndex( void ) const
{
	if ( !IsMultiSelectionList() )
		return GetCurSel();

	std::vector<int> selIndexes;
	int caretIndex;

	if ( GetSelection( selIndexes, &caretIndex ) )
		if ( -1 == caretIndex )										// no caret?
			return FindItemWithState( LVNI_SELECTED );
		else if ( utl::Contains( selIndexes, caretIndex ) )			// caret is selected?
			return caretIndex;

	return -1;
}

bool CReportListControl::GetSelection( std::vector<int>& rSelIndexes, int* pCaretIndex /*= nullptr*/, int* pTopIndex /*= nullptr*/ ) const
{
	rSelIndexes.reserve( rSelIndexes.size() + GetSelectedCount() );

	for ( POSITION pos = GetFirstSelectedItemPosition(); pos != nullptr; )
		rSelIndexes.push_back( GetNextSelectedItem( pos ) );

	if ( pCaretIndex != nullptr )
		*pCaretIndex = FindItemWithState( LVNI_FOCUSED );

	if ( pTopIndex != nullptr )
		*pTopIndex = GetTopIndex();

	std::sort( rSelIndexes.begin(), rSelIndexes.end() );		// sort indexes ascending
	return !rSelIndexes.empty();
}

bool CReportListControl::GetSelIndexBounds( int* pMinSelIndex, int* pMaxSelIndex ) const
{
	std::vector<int> selIndexes;
	if ( !GetSelection( selIndexes ) )
	{
		utl::AssignPtr( pMinSelIndex, -1 );
		utl::AssignPtr( pMaxSelIndex, -1 );
		return false;
	}

	utl::AssignPtr( pMinSelIndex, selIndexes.front() );
	utl::AssignPtr( pMaxSelIndex, selIndexes.back() );
	return true;
}

Range<int> CReportListControl::GetSelIndexRange( void ) const
{
	Range<int> selRange;
	GetSelIndexBounds( &selRange.m_start, &selRange.m_end );
	return selRange;
}

bool CReportListControl::Select( const void* pObject )
{
	int indexFound = FindItemIndex( pObject );
	if ( IsMultiSelectionList() )
	{
		if ( indexFound != -1 )
		{
			std::vector<int> selIndexes;
			selIndexes.push_back( indexFound );

			SetSelection( selIndexes, indexFound );
		}
		else
			ClearSelection();

		return ( -1 == indexFound ) == ( nullptr == pObject );
	}
	else
		return SetCurSel( indexFound );
}

void CReportListControl::SetSelection( const std::vector<int>& selIndexes, int caretIndex /*= -1*/ )
{
	// clear the selection
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		SetItemState( i, 0, LVNI_SELECTED | LVNI_FOCUSED );

	// select new items
	for ( size_t i = 0; i != selIndexes.size(); ++i )
		SetItemState( selIndexes[ i ], LVNI_SELECTED, LVNI_SELECTED );

	if ( -1 == caretIndex && !selIndexes.empty() )
		caretIndex = selIndexes.back();				// caret on the last selected

	if ( caretIndex != -1 )
	{
		SetItemState( caretIndex, LVNI_FOCUSED, LVNI_FOCUSED );
		SetSelectionMark( caretIndex );
		EnsureVisible( caretIndex, false );
	}
}

void CReportListControl::ClearSelection( void )
{
	std::vector<int> selIndexes;
	if ( GetSelection( selIndexes ) )
	{
		// clear the selection
		for ( UINT i = 0; i != selIndexes.size(); ++i )
			SetItemState( selIndexes[ i ], 0, LVNI_SELECTED );
	}
}

void CReportListControl::SelectAll( void )
{
	for ( int i = 0, count = GetItemCount(); i != count; ++i )
		SetItemState( i, LVNI_SELECTED, LVNI_SELECTED );
}


int CReportListControl::DeleteSelection( void )
{
	std::vector<int> selIndexes;
	if ( !GetSelection( selIndexes ) )
		return GetCaretIndex();

	lv::CNmItemsRemoved removeNotify( this, selIndexes.front() );
	QueryObjectsByIndex( removeNotify.m_removedObjects, selIndexes );

	{
		CScopedInternalChange internalChange( this );

		for ( std::vector<int>::const_reverse_iterator itSelIndex = selIndexes.rbegin(); itSelIndex != selIndexes.rend(); ++itSelIndex )
			DeleteItem( *itSelIndex );
	}

	int newSelIndex = std::min( selIndexes.front(), GetItemCount() - 1 );

	if ( newSelIndex != -1 )
		SetCurSel( newSelIndex );

	removeNotify.m_nmHdr.NotifyParent();			// lv::LVN_ItemsRemoved -> notify parent to delete owned objects

	return newSelIndex;
}

void CReportListControl::MoveSelectionTo( seq::MoveTo moveTo )
{
	{
		CScopedInternalChange internalChange( this );

		std::vector<int> selIndexes;
		GetSelection( selIndexes );

		CListCtrlSequence sequence( this );
		seq::Resequence( sequence, selIndexes, moveTo );

		Range<int> maxSelIndex = GetSelIndexRange();
		EnsureVisible( seq::MovePrev == moveTo || seq::MoveToStart == moveTo ? maxSelIndex.m_start : maxSelIndex.m_end, FALSE );
	}

	ui::SendCommandToParent( m_hWnd, lv::LVN_ItemsReorder );
}

bool CReportListControl::CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags, const CListSelectionData& selData ) const
{
	ASSERT_PTR( pDataSource );

	if ( !selData.IsValid() || !HasFlag( sourceFlags, ListSourcesMask ) )
		return false;

	if ( HasFlag( sourceFlags, ds::Indexes ) )
		selData.CacheTo( pDataSource );

	std::vector<std::tstring> textLines;
	if ( HasFlag( sourceFlags, ds::ItemsText | ds::ShellFiles ) )
		for ( std::vector<int>::const_iterator itSelIndex = selData.m_selIndexes.begin(); itSelIndex != selData.m_selIndexes.end(); ++itSelIndex )
			textLines.push_back( FormatItemLine( *itSelIndex ) );

	if ( HasFlag( sourceFlags, ds::ItemsText ) )
		ole_utl::CacheTextData( pDataSource, str::Join( textLines, _T("\r\n") ) );

	if ( HasFlag( sourceFlags, ds::ShellFiles ) )
	{
		std::vector<fs::CPath> filePaths;
		utl::Assign( filePaths, textLines, func::tor::StringOf() );

		pDataSource->CacheShellFilePaths( filePaths );
	}

	return true;
}

std::tstring CReportListControl::FormatItemLine( int index ) const
{
	std::tstring lineText;
	utl::ISubject* pObject = GetSubjectAt( index );

	if ( !str::IsEmpty( m_pTabularSep ) )
	{
		for ( unsigned int col = 0, count = GetColumnCount(); col != count; ++col )
		{
			if ( col != 0 )
				lineText += m_pTabularSep;

			if ( Code == col && pObject != nullptr )
				lineText += pObject->GetCode();
			else
				lineText += GetItemText( index, col ).GetString();
		}
	}
	else
		if ( pObject != nullptr )
			lineText = pObject->GetCode();		// assume GetCode() represents a full path, and GetDisplayCode() represents a fname.ext
		else
			lineText = GetItemText( index, Code ).GetString();

	return lineText;
}

bool CReportListControl::Copy( int sourceFlags /*= ListSourcesMask*/ )
{
	// inspired from COleServerItem::CopyToClipboard() in MFC
	std::auto_ptr<ole::CDataSource> pDataSource( m_pDataSourceFactory->NewDataSource() );
	if ( CacheSelectionData( pDataSource.get(), sourceFlags ) )
	{	// put it on the clipboard and let the clipboard manage (own) the data source
		pDataSource->SetClipboard();
		pDataSource.release();
		return true;
	}
	return false;
}

std::auto_ptr<CImageList> CReportListControl::CreateDragImageMulti( const std::vector<int>& indexes, CPoint* pFrameOrigin /*= nullptr*/ )
{
	std::auto_ptr<CImageList> pDragImage;			// imagelist with the merged drag images
	if ( !indexes.empty() )
	{
		SHDRAGIMAGE shDragImage;
		if ( shell::GetDragImage( shDragImage, m_hWnd ) != nullptr )
		{
			pDragImage.reset( new CImageList() );
			pDragImage->Create( shDragImage.sizeDragImage.cx, shDragImage.sizeDragImage.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );		// 1 image with all items

			// API level call to skip attaching a temp CBitmap; note that shell drag images owns the bitmap.
		#if _MSC_VER > 1500		// >MSVC++ 9.0 (Visual Studio 2008)
			ImageList_AddMasked( pDragImage->GetSafeHandle(), shDragImage.hbmpDragImage, shDragImage.crColorKey );
		#else
			AfxImageList_AddMasked( pDragImage->GetSafeHandle(), shDragImage.hbmpDragImage, shDragImage.crColorKey );
		#endif

			if ( pFrameOrigin != nullptr )
				*pFrameOrigin = GetFrameBounds( indexes ).TopLeft();	// offset of the current mouse cursor to the imagelist we can use in BeginDrag()
		}
	}
	return pDragImage;
}

std::auto_ptr<CImageList> CReportListControl::CreateDragImageSelection( CPoint* pFrameOrigin /*= nullptr*/ )
{
	std::vector<int> selIndexes;
	GetSelection( selIndexes );
	return CreateDragImageMulti( selIndexes, pFrameOrigin );
}

CRect CReportListControl::GetFrameBounds( const std::vector<int>& indexes ) const
{
	CRect frameRect( 0, 0, 0, 0 );
	for ( UINT i = 0; i != indexes.size(); ++i )
	{
		CRect itemRect;
		GetItemRect( indexes[ i ], &itemRect, LVIR_BOUNDS );
		if ( 0 == i )
			frameRect = itemRect;
		else
			frameRect |= itemRect;
	}
	return frameRect;			// frame bounds of the selected items in client coords
}


// cell text effects (custom draw)

void CReportListControl::MarkCellAt( int index, TColumn subItem, const ui::CTextEffect& textEfect )
{
	m_markedCells[ TCellPair( MakeRowKeyAt( index ), subItem ) ] = textEfect;			// also works with EntireRecord
}

void CReportListControl::UnmarkCellAt( int index, TColumn subItem )
{
	TCellTextEffectMap::iterator itFound = m_markedCells.find( TCellPair( MakeRowKeyAt( index ), subItem ) );
	if ( itFound != m_markedCells.end() )
		m_markedCells.erase( itFound );
}

void CReportListControl::ClearMarkedCells( void )
{
	m_markedCells.clear();
	Invalidate();
}

const ui::CTextEffect* CReportListControl::FindTextEffectAt( TRowKey rowKey, TColumn subItem ) const
{
	return utl::FindValuePtr( m_markedCells, TCellPair( rowKey, subItem ) );
}

const CReportListControl::CDiffColumnPair* CReportListControl::FindDiffColumnPair( TColumn column ) const
{
	for ( std::list<CDiffColumnPair>::const_iterator itDiffPair = m_diffColumnPairs.begin(); itDiffPair != m_diffColumnPairs.end(); ++itDiffPair )
		if ( itDiffPair->HasColumn( column ) )
			return &*itDiffPair;

	return nullptr;
}


// groups

#ifdef UTL_VISTA_GROUPS

int CReportListControl::GetGroupId( int groupIndex ) const
{
	LVGROUP group;
	utl::ZeroWinStruct( &group );

	group.mask = LVGF_GROUPID;
	VERIFY( GetGroupInfoByIndex( groupIndex, &group ) );
	return group.iGroupId;
}

int CReportListControl::GetItemGroupId( int itemIndex ) const
{
	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = itemIndex;
	if ( !GetItem( &item ) )
		return -1;					// item does not belong to a group
	return item.iGroupId;
}

bool CReportListControl::SetItemGroupId( int itemIndex, int groupId )
{
	// rows not assigned to a group will not show in group-view
	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = itemIndex;
	item.iSubItem = 0;
	item.iGroupId = groupId;
	if ( !SetItem( &item ) )
		return false;

	m_groupIdToItemsMap.insert( std::make_pair( groupId, MakeRowKeyAt( itemIndex ) ) );
	return true;
}

std::pair<int, UINT> CReportListControl::_GetGroupItemsRange( int groupId ) const
{
	LVGROUP group;
	utl::ZeroWinStruct( &group );

	group.mask = LVGF_ITEMS;
	group.iGroupId = groupId;
	VERIFY( GetGroupInfo( groupId, &group ) != -1 );		// returns the groupID

	return std::make_pair( group.iFirstItem, group.cItems );
}

std::tstring CReportListControl::GetGroupHeaderText( int groupId ) const
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_HEADER | LVGF_GROUPID;
	group.iGroupId = groupId;
	VERIFY( GetGroupInfo( groupId, &group ) != -1 );
	return group.pszHeader;
}

int CReportListControl::InsertGroupHeader( int groupIndex, int groupId, const std::tstring& header, DWORD state /*= LVGS_NORMAL*/, DWORD align /*= LVGA_HEADER_LEFT*/ )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_GROUPID | LVGF_STATE | LVGF_ALIGN | LVGF_HEADER;
	group.iGroupId = groupId;
	group.state = state;
	group.uAlign = align;
	group.pszHeader = const_cast<TCHAR*>( header.c_str() );

	return InsertGroup( groupIndex, &group );
}

bool CReportListControl::SetGroupFooter( int groupId, const std::tstring& footer, DWORD align /*= LVGA_FOOTER_CENTER*/ )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_FOOTER | LVGF_ALIGN;
	group.uAlign = align;
	group.pszFooter = const_cast<TCHAR*>( footer.c_str() );

	return SetGroupInfo( groupId, &group ) != -1;
}

bool CReportListControl::SetGroupTask( int groupId, const std::tstring& task )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_TASK;
	group.pszTask = const_cast<TCHAR*>( task.c_str() );

	return SetGroupInfo( groupId, &group ) != -1;
}

bool CReportListControl::SetGroupSubtitle( int groupId, const std::tstring& subtitle )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_SUBTITLE;
	group.pszSubtitle = const_cast<TCHAR*>( subtitle.c_str() );

	return SetGroupInfo( groupId, &group ) != -1;
}

bool CReportListControl::SetGroupTitleImage( int groupId, int image, const std::tstring& topDescr, const std::tstring& bottomDescr )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_TITLEIMAGE;
	group.iTitleImage = image;				// index of the title image in the control imagelist

	if ( !topDescr.empty() )
	{	// top description is drawn opposite the title image when there is a title image, no extended image, and uAlign==LVGA_HEADER_CENTER
		group.mask |= LVGF_DESCRIPTIONTOP;
		group.pszDescriptionTop = const_cast<TCHAR*>( topDescr.c_str() );
	}
	if ( !bottomDescr.empty() )
	{	// bottom description is drawn under the top description text when there is a title image, no extended image, and uAlign==LVGA_HEADER_CENTER
		group.mask |= LVGF_DESCRIPTIONBOTTOM;
		group.pszDescriptionBottom = const_cast<TCHAR*>( bottomDescr.c_str() );
	}

	return SetGroupInfo( groupId, &group ) != -1;
}

void CReportListControl::DeleteEntireGroup( int groupId )
{
	for ( int rowIndex = 0, count = GetItemCount(); rowIndex != count; ++rowIndex )
		if ( GetItemGroupId( rowIndex ) == groupId )
			DeleteItem( rowIndex-- );

	RemoveGroup( groupId );
}

void CReportListControl::CheckEntireGroup( int groupId, bool check /*= true*/ )
{
	if ( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) )
		for ( int rowIndex = 0, count = GetItemCount(); rowIndex != count; ++rowIndex )
			if ( GetItemGroupId( rowIndex ) == groupId )
				SetChecked( rowIndex, check );
}

void CReportListControl::CollapseAllGroups( void )
{
	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
	{
		int groupId = GetGroupId( i );
		if ( !HasGroupState( groupId, LVGS_COLLAPSED ) )
			SetGroupState( groupId, LVGS_COLLAPSED, LVGS_COLLAPSED );
	}
}

void CReportListControl::ExpandAllGroups( void )
{
	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
	{
		int groupId = GetGroupId( i );
		if ( HasGroupState( groupId, LVGS_COLLAPSED ) )
			SetGroupState( groupId, LVGS_COLLAPSED, LVGS_NORMAL );
	}
}

#endif //UTL_VISTA_GROUPS


void CReportListControl::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	SetupControl();
}

BOOL CReportListControl::PreTranslateMessage( MSG* pMsg )
{
	return
		TranslateMessage( pMsg ) ||
		__super::PreTranslateMessage( pMsg );
}

BOOL CReportListControl::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( IsCommandFrame() )
	{
		/*	Special routing when this list is a CommandFrame:
			- Toolbar and context menu commands are handled internally by the list - list is the Owner of its toolbar;
			- Parent dialog may have its own handlerd that override list handlers: route the commands to dialog handlers in that case.

			In this operationg mode the parent dialog works like "Don't call us, we'll call you":
			- If it overrrides OnCmdMsg(), it should NOT route it to CReportListControl::OnCmdMsg();
			- It works the other way around, list routes to parent dialog.
		 */

		if ( m_pFrameEditor != nullptr )
		{
			if ( m_pFrameEditor->HandleCtrlCmdMsg( id, code, pExtra, pHandlerInfo ) )
				return TRUE;			// handled by frame editor, which take precedence over internal handler
		}
		else
			if ( GetParent()->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
				return TRUE;			// handled by dialog custom handler, which take precedence over internal handler
	}

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CReportListControl, CBaseTrackMenuWnd<CListCtrl> )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_KEYDOWN()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT_EX( LVN_ITEMCHANGING, OnLvnItemChanging_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_ITEMCHANGED, OnLvnItemChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( HDN_ITEMCHANGING, OnHdnItemChanging_Reflect )
	ON_NOTIFY_REFLECT_EX( HDN_ITEMCHANGED, OnHdnItemChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_COLUMNCLICK, OnLvnColumnClick_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_BEGINLABELEDIT, OnLvnBeginLabelEdit_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_ENDLABELEDIT, OnLvnEndLabelEdit_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_GETDISPINFO, OnLvnGetDispInfo_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_CUSTOMDRAW, OnNmCustomDraw_Reflect )
	ON_COMMAND_RANGE( ID_LIST_VIEW_ICON_LARGE, ID_LIST_VIEW_TILE, OnListViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_LIST_VIEW_ICON_LARGE, ID_LIST_VIEW_TILE, OnUpdateListViewMode )
	ON_COMMAND_RANGE( ID_LIST_VIEW_STACK_LEFTTORIGHT, ID_LIST_VIEW_STACK_TOPTOBOTTOM, OnListViewStacking )
	ON_UPDATE_COMMAND_UI_RANGE( ID_LIST_VIEW_STACK_LEFTTORIGHT, ID_LIST_VIEW_STACK_TOPTOBOTTOM, OnUpdateListViewStacking )
	ON_COMMAND( ID_RESET_DEFAULT, OnResetColumnLayout )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdateResetColumnLayout )
	ON_COMMAND( ID_EDIT_COPY, OnCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateAnySelected )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnSelectAll )
	ON_UPDATE_COMMAND_UI( ID_EDIT_SELECT_ALL, OnUpdateSelectAll )
	ON_COMMAND_RANGE( ID_MOVE_UP_ITEM, ID_MOVE_BOTTOM_ITEM, OnMoveTo )
	ON_UPDATE_COMMAND_UI_RANGE( ID_MOVE_UP_ITEM, ID_MOVE_BOTTOM_ITEM, OnUpdateMoveTo )
	ON_COMMAND( ID_RENAME_ITEM, OnRename )
	ON_UPDATE_COMMAND_UI( ID_RENAME_ITEM, OnUpdateRename )
	ON_COMMAND_RANGE( ID_EXPAND, ID_COLLAPSE, OnExpandCollapseGroups )
	ON_UPDATE_COMMAND_UI_RANGE( ID_EXPAND, ID_COLLAPSE, OnUpdateExpandCollapseGroups )
END_MESSAGE_MAP()

int CReportListControl::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == __super::OnCreate( pCreateStruct ) )
		return -1;

	SetupControl();
	return 0;
}

void CReportListControl::OnDestroy( void )
{
	if ( !m_regSection.empty() )
		SaveToRegistry();

	__super::OnDestroy();
}

void CReportListControl::OnDropFiles( HDROP hDropInfo )
{
	GetAncestor( GA_PARENT )->SetActiveWindow();		// activate us first

	CPoint dropPoint = ui::GetCursorPos( m_hWnd );
	lv::CNmDropFiles dropFiles( this, dropPoint, GetDropIndexAtPoint( dropPoint ) );

	shell::QueryDroppedFiles( dropFiles.m_filePaths, hDropInfo, SortAscending );

	dropFiles.m_nmHdr.NotifyParent();					// lv::LVN_DropFiles -> notify parent of dropped file paths
}

void CReportListControl::OnWindowPosChanged( WINDOWPOS* pWndPos )
{
	__super::OnWindowPosChanged( pWndPos );

	if ( !HasFlag( pWndPos->flags, SWP_NOMOVE ) || !HasFlag( pWndPos->flags, SWP_NOSIZE ) )
		ResizeFlexColumns();
}

void CReportListControl::OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags )
{
	std::auto_ptr<CSelFlowSequence> pSelFlow = CSelFlowSequence::MakeFlow( this );

	if ( nullptr == pSelFlow.get() || pSelFlow->HandleKeyDown( chr ) )
	{
		__super::OnKeyDown( chr, repCnt, vkFlags );

		if ( pSelFlow.get() != nullptr )
			pSelFlow->PostKeyDown( chr );
	}
}

void CReportListControl::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	__super::OnNcLButtonDown( hitTest, point );
}

void CReportListControl::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	UINT flags;
	TGroupId groupId;
	int hitIndex = HitTest( ui::ScreenToClient( m_hWnd, screenPos ), &flags, &groupId );
	ListPopup popupType = _ListPopupCount;

	if ( HasFlag( flags, LVHT_EX_GROUP ) )
		popupType = OnGroup;
	else if ( pWnd == GetHeaderCtrl() || -1 == hitIndex )		// clicked on header or nowhere?
		popupType = Nowhere;
	else if ( HasFlag( flags, LVHT_ONITEM ) )
		popupType = OnSelection;

	if ( popupType != _ListPopupCount )
		if ( TrackContextMenu( popupType, screenPos ) )
			return;					// supress rising WM_CONTEXTMENU to the parent

	Default();
}

void CReportListControl::OnPaint( void )
{
	REQUIRE( !m_painting );

	m_painting = true;			// for diff text items: supress sub-item draw by the list (default list painting smudges the text due to font effects)
	__super::OnPaint();
	m_painting = false;
}

BOOL CReportListControl::OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pListView = (NMLISTVIEW*)pNmHdr;
	*pResult = 0L;

	if ( !IsInternalChange() )									// a user change?
		if ( IsCheckStateChangeNotify( pListView ) )			// item check-state is about to change?
		{
			if ( m_pCheckStatePolicy != nullptr )
			{
				lv::CNmToggleCheckState toggleInfo( this, pListView, m_pCheckStatePolicy );		// toggle check-state by default

				if ( 0L == toggleInfo.m_nmHdr.NotifyParent() )	// send lv::LVN_ToggleCheckState, give parent a chance to override toggling
					if ( toggleInfo.m_newCheckState != toggleInfo.m_oldCheckState )
						ToggleCheckState( pListView->iItem, toggleInfo.m_newCheckState );		// will send a delayed lv::LVN_CheckStatesChanged notification

				*pResult = TRUE;								// prevent automatic toggle initiated internally by the list-ctrl (default behaviour)
				return TRUE;
			}
		}

	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CReportListControl::OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pListView = (NMLISTVIEW*)pNmHdr;
	*pResult = 0L;

	if ( GetToggleCheckSelItems() )											// apply toggle to multi-selection?
		if ( !IsInternalChange() || m_pNmToggling.get() != nullptr )			// user has toggled the check-state (directly or indirectly)?
			if ( IsCheckStateChangeNotify( pListView ) )					// user has toggled the check-state?
				ApplyCheckStateToSelectedItems( pListView->iItem, ui::CheckStateFromRaw( pListView->uNewState ) );		// apply check-state to selected items if toggled an item that is part of the multi-selection

	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CReportListControl::OnHdnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMHEADER* pHeaderInfo = (NMHEADER*)pNmHdr;
	UNUSED_ALWAYS( pHeaderInfo );
	*pResult = 0L;
	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CReportListControl::OnHdnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMHEADER* pHeaderInfo = (NMHEADER*)pNmHdr;
	UNUSED_ALWAYS( pHeaderInfo );
	*pResult = 0L;
	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CReportListControl::OnLvnColumnClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr;

	*pResult = 0L;

	CNmCanSortByColumn nmCanSortBy( this, pNmListView->iSubItem );
	if ( nmCanSortBy.m_nmHdr.NotifyParent() != 0L )
	{
		if ( CToolTipCtrl* pTooltip = GetToolTips() )
		{
			if ( CHeaderCtrl* pColumnHeader = GetHeaderCtrl() )
			{
				CRect columnRect;
				if ( pColumnHeader->GetItemRect( pNmListView->iSubItem, &columnRect ) )
				{
				}
			}

			pTooltip->UpdateTipText( _T("Warning: cannot sort by this column!"), this );
			pTooltip->Activate( TRUE );
			pTooltip->Popup();
		}

		*pResult = 1L;		// parent rejected sorting by this column (sorting disabled)
		return TRUE;		// handled, skip routing to parent
	}

	if ( ui::IsKeyPressed( VK_CONTROL ) || ui::IsKeyPressed( VK_SHIFT ) )
		SetSortByColumn( -1 );											// switch to original order (no sorting)
	else
	{
		TColumn sortByColumn = pNmListView->iSubItem;

		// switch sorting
		if ( sortByColumn != m_sortByColumn )
			SetSortByColumn( sortByColumn, IsDefaultAscending( sortByColumn ) );		// sort by a new column (ascending/descending depending on column data)
		else
			if ( sortByColumn != -1 )
				SetSortByColumn( sortByColumn, !m_sortAscending );		// toggle ascending/descending on the same column
	}

	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnLvnBeginLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNmHdr;

	m_pLabelEdit.reset( new CLabelEdit( pDispInfo->item.iItem, GetItemText( pDispInfo->item.iItem, 0 ).GetString() ) );
	*pResult = 0;
	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnLvnEndLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNmHdr;
	*pResult = 0;

	if ( m_pLabelEdit.get() != nullptr )
		if ( pDispInfo->item.pszText != nullptr )
		{
			m_pLabelEdit->m_done = true;
			m_pLabelEdit->m_newLabel = pDispInfo->item.pszText;
			*pResult = TRUE;		// assume valid input

			if ( !ParentHandlesWmNotify( LVN_ENDLABELEDIT ) )
				return TRUE;		// mark as handled so changes are applied
		}
		else
			m_pLabelEdit.reset();

	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnLvnGetDispInfo_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNmHdr;

	if ( HasFlag( pDispInfo->item.mask, LVIF_TEXT ) )
		if ( !m_painting )					// supress sub-item draw by the list (default list painting)
			if ( const CDiffColumnPair* pDiffPair = FindDiffColumnPair( pDispInfo->item.iSubItem ) )
				if ( const str::TMatchSequence* pMatchSeq = utl::FindValuePtr( pDiffPair->m_rowSequences, MakeRowKeyAt( pDispInfo->item.iItem ) ) )
					pDispInfo->item.pszText = const_cast<TCHAR*>( SrcDiff == pDiffPair->GetDiffSide( pDispInfo->item.iSubItem )
						? pMatchSeq->m_textPair.first.c_str()
						: pMatchSeq->m_textPair.second.c_str() );

	*pResult = 0;

	if ( !ParentHandlesWmNotify( LVN_GETDISPINFO ) )
		return TRUE;			// mark as handled so changes are applied

	return FALSE;				// continue handling by parent, even if changed (additive logic)
}

BOOL CReportListControl::OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pDraw = (NMLVCUSTOMDRAW*)pNmHdr;
	if ( CListLikeCustomDrawBase::IsTooltipDraw( &pDraw->nmcd ) )
		return TRUE;		// IMP: avoid custom drawing for tooltips

	//TRACE( _T(" CReportListControl::DrawStage: %s\n"), dbg::FormatDrawStage( pDraw->nmcd.dwDrawStage ) );

	CReportListCustomDraw draw( pDraw, this );

	*pResult = CDRF_DODEFAULT;
	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;

			if ( m_pCustomImager.get() != nullptr )
				*pResult |= CDRF_NOTIFYPOSTPAINT;				// will superimpose the thumbnail on top of transparent image (on CDDS_ITEMPOSTPAINT drawing stage)

			break;
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
			if ( draw.ApplyCellTextEffect() )
				*pResult |= CDRF_NEWFONT;

			if ( !m_diffColumnPairs.empty() )
				*pResult |= CDRF_NOTIFYPOSTPAINT;				// will draw diff sub-item text (on CDDS_ITEMPOSTPAINT | CDDS_SUBITEM drawing stage)
			break;

		case CDDS_ITEMPOSTPAINT:
			if ( !draw.m_isReportMode )
				draw.DrawCellTextDiffs();						// draw text diffs since there is no CDDS_SUBITEM notification

			if ( m_pCustomImager.get() != nullptr )
				if ( IsItemVisible( draw.m_index ) )
				{
					CRect itemImageRect;
					if ( GetIconItemRect( &itemImageRect, draw.m_index ) )		// visible item?
						if ( m_pCustomImager->DrawItemGlyph( &pDraw->nmcd, itemImageRect ) )
							return TRUE;				// handled
				}

			break;
		case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
			draw.DrawCellTextDiffs();
			break;
	}

	if ( !ParentHandlesWmNotify( NM_CUSTOMDRAW ) )
		return TRUE;			// mark as handled so changes are applied

	return FALSE;				// continue handling by parent, even if changed (additive logic)
}

void CReportListControl::OnListViewMode( UINT cmdId )
{
	ChangeListViewMode( lv::CmdIdToListViewMode( cmdId ) );
	Arrange( LVA_DEFAULT );
}

void CReportListControl::OnUpdateListViewMode( CCmdUI* pCmdUI )
{
	DWORD viewMode = lv::CmdIdToListViewMode( pCmdUI->m_nID );
	bool modeHasImages = ( LV_VIEW_ICON == viewMode ? m_pLargeImageList : m_pImageList ) != nullptr;

	pCmdUI->Enable( modeHasImages );
	ui::SetRadio( pCmdUI, viewMode == GetView() );
}

void CReportListControl::OnListViewStacking( UINT cmdId )
{
	DWORD stackingStyle = lv::CmdIdToListViewStacking( cmdId );
	ModifyStyle( LVS_ALIGNMASK, stackingStyle );

	switch ( stackingStyle )
	{
		case LVS_ALIGNTOP:	Arrange( LVA_ALIGNTOP ); break;
		case LVS_ALIGNLEFT:	Arrange( LVA_ALIGNLEFT ); break;
		default:			Arrange( LVA_DEFAULT ); break;
	}
}

void CReportListControl::OnUpdateListViewStacking( CCmdUI* pCmdUI )
{
	DWORD stackingStyle = lv::CmdIdToListViewStacking( pCmdUI->m_nID );
	DWORD viewMode = GetView();

	pCmdUI->Enable( viewMode != LV_VIEW_DETAILS );
	ui::SetRadio( pCmdUI, stackingStyle == ( GetStyle() & LVS_ALIGNMASK ) );
}

void CReportListControl::OnResetColumnLayout( void )
{
	ResetColumnLayout();
}

void CReportListControl::OnUpdateResetColumnLayout( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_columnLayoutId != 0 && LV_VIEW_DETAILS == GetView() );
}

void CReportListControl::OnCopy( void )
{
	Copy();
}

void CReportListControl::OnSelectAll( void )
{
	SelectAll();
}

void CReportListControl::OnUpdateSelectAll( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsMultiSelectionList() );
}

void CReportListControl::OnMoveTo( UINT cmdId )
{
	MoveSelectionTo( lv::CmdIdToMoveTo( cmdId ) );
}

void CReportListControl::OnUpdateMoveTo( CCmdUI* pCmdUI )
{
	std::vector<int> selIndexes;
	GetSelection( selIndexes );

	pCmdUI->Enable( seq::CanMoveSelection( GetItemCount(), selIndexes, lv::CmdIdToMoveTo( pCmdUI->m_nID ) ) );
}

void CReportListControl::OnRename( void )
{
	EditLabel( GetCurSel() );
}

void CReportListControl::OnUpdateRename( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( HasFlag( GetStyle(), LVS_EDITLABELS ) && GetCurSel() != -1 );
}

void CReportListControl::OnExpandCollapseGroups( UINT cmdId )
{
	if ( ID_EXPAND == cmdId )
		ExpandAllGroups();
	else
		CollapseAllGroups();
}

void CReportListControl::OnUpdateExpandCollapseGroups( CCmdUI* pCmdUI )
{
	bool anyToToggle = false;

	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
	{
		bool isCollapsed = HasGroupState( GetGroupId( i ), LVGS_COLLAPSED );
		if ( ID_EXPAND == pCmdUI->m_nID ? isCollapsed : ( !isCollapsed ) )
		{
			anyToToggle = true;
			break;
		}
	}

	pCmdUI->Enable( anyToToggle );
}

void CReportListControl::OnUpdateAnySelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( AnySelected() );
}

void CReportListControl::OnUpdateSingleSelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( SingleSelected() );
}


// CReportListControl::CDiffColumnPair implementation

const str::TMatchSequence* CReportListControl::CDiffColumnPair::FindRowSequence( TRowKey rowKey ) const
{
	return utl::FindValuePtr( m_rowSequences, rowKey );
}


namespace lv
{
	// TScopedStatus_ByIndex aka CScopedStatus<int> - by-index explicit template instantiation

	template<>
	TScopedStatus_ByIndex::CScopedStatus( CReportListControl* pListCtrl, bool useTopItem /*= false*/ )
		: m_pListCtrl( pListCtrl )
		, m_topVisibleItem( -1 )
		, m_caretItem( -1 )
	{
		ASSERT_PTR( m_pListCtrl->GetSafeHwnd() );

		if ( m_pListCtrl->GetItemCount() != 0 )
		{
			m_pListCtrl->GetSelection( m_selItems, &m_caretItem, &m_topVisibleItem );

			if ( !useTopItem )
				m_topVisibleItem = -1;
		}
	}

	template<>
	void TScopedStatus_ByIndex::Restore( void )
	{
		if ( m_pListCtrl != nullptr && m_pListCtrl->GetItemCount() != 0 )
		{
			//CScopedInternalChange internalChange( m_pListCtrl );
			m_pListCtrl->SetSelection( m_selItems, m_caretItem );

			if ( m_topVisibleItem != -1 )
				m_pListCtrl->EnsureVisible( m_topVisibleItem, FALSE );
		}

		m_pListCtrl = nullptr;
	}


	// TScopedStatus_ByObject aka CScopedStatus<utl::ISubject*> - by-object explicit template instantiation

	template<>
	TScopedStatus_ByObject::CScopedStatus( CReportListControl* pListCtrl, bool useTopItem /*= false*/ )
		: m_pListCtrl( pListCtrl )
		, m_topVisibleItem( nullptr )
		, m_caretItem( nullptr )
	{
		ASSERT_PTR( m_pListCtrl->GetSafeHwnd() );

		if ( m_pListCtrl->GetItemCount() != 0 )
		{
			REQUIRE( m_pListCtrl->IsObjectBased() || 0 == m_pListCtrl->GetItemCount() );

			if ( useTopItem )
			{
				int topIndex = m_pListCtrl->GetTopIndex();
				if ( topIndex != -1 )
					m_topVisibleItem = m_pListCtrl->GetSubjectAt( topIndex );
			}

			m_caretItem = m_pListCtrl->GetCaretAs<utl::ISubject>();
			m_pListCtrl->QuerySelectionAs( m_selItems );
		}
	}

	template<>
	void TScopedStatus_ByObject::Restore( void )
	{
		if ( m_pListCtrl != nullptr && m_pListCtrl->GetItemCount() != 0 )
		{
			//CScopedInternalChange internalChange( m_pListCtrl );

			if ( m_pListCtrl->IsMultiSelectionList() )
				m_pListCtrl->SelectObjects( m_selItems );
			else if ( 1 == m_selItems.size() )
				m_pListCtrl->Select( m_selItems.front() );

			if ( m_caretItem != nullptr && m_caretItem != m_pListCtrl->GetCaretAs<utl::ISubject>() )
			{
				int caretIndex = m_pListCtrl->FindItemIndex( m_caretItem );
				if ( caretIndex != -1 )
					m_pListCtrl->SetCaretIndex( caretIndex );
			}

			if ( m_topVisibleItem != nullptr )
				m_pListCtrl->EnsureVisibleObject( m_topVisibleItem );
		}

		m_pListCtrl = nullptr;
	}


	// TScopedStatus_ByText aka CScopedStatus<std::tstring> - by-text explicit template instantiation

	template<>
	TScopedStatus_ByText::CScopedStatus( CReportListControl* pListCtrl, bool useTopItem /*= false*/ )
		: m_pListCtrl( pListCtrl )
	{
		ASSERT_PTR( m_pListCtrl->GetSafeHwnd() );

		if ( m_pListCtrl->GetItemCount() != 0 )
		{
			if ( useTopItem )
			{
				int topIndex = m_pListCtrl->GetTopIndex();
				if ( topIndex != -1 )
					m_topVisibleItem = m_pListCtrl->GetItemText( topIndex, 0 ).GetString();
			}

			int caretIndex;
			std::vector<int> selIndexes;

			m_pListCtrl->GetSelection( selIndexes, &caretIndex );
			m_pListCtrl->QueryItemsText( m_selItems, selIndexes );

			if ( caretIndex != -1 )
				m_caretItem = m_pListCtrl->GetItemText( caretIndex, 0 ).GetString();
		}
	}

	template<>
	void TScopedStatus_ByText::Restore( void )
	{
		if ( m_pListCtrl != nullptr && m_pListCtrl->GetItemCount() != 0 )
		{
			//CScopedInternalChange internalChange( m_pListCtrl );

			std::vector<int> selIndexes;
			selIndexes.reserve( m_selItems.size() );
			for ( std::vector<std::tstring>::const_iterator itSelText = m_selItems.begin(); itSelText != m_selItems.end(); ++itSelText )
			{
				int foundIndex = m_pListCtrl->FindItemIndex( *itSelText );
				if ( foundIndex != -1 )
					selIndexes.push_back( foundIndex );
			}
			std::sort( selIndexes.begin(), selIndexes.end() );

			int caretIndex = !m_caretItem.empty() ? m_pListCtrl->FindItemIndex( m_caretItem ) : -1;
			m_pListCtrl->SetSelection( selIndexes, caretIndex );

			if ( !m_topVisibleItem.empty() )
			{
				int topIndex = m_pListCtrl->FindItemIndex( m_topVisibleItem );
				if ( topIndex != -1 )
					m_pListCtrl->EnsureVisible( topIndex, FALSE );
			}
		}

		m_pListCtrl = nullptr;
	}

} //namespace lv


// CSelFlowSequence implementation

CSelFlowSequence::CSelFlowSequence( CReportListControl* pListCtrl )
	: m_pListCtrl( pListCtrl )
	, m_itemCount( m_pListCtrl->GetItemCount() )
	, m_caretIndex( m_pListCtrl->GetCaretIndex() )
	, m_topIndex( m_pListCtrl->GetTopIndex() )
	, m_colsPerPage( 1 )
	, m_rowsPerPage( 1 )
	, m_seqDirection( EqFlag( m_pListCtrl->GetStyle(), LVS_ALIGNLEFT ) ? TopToBottom : LeftToRight )
{
	ASSERT_PTR( m_pListCtrl );

	if ( m_itemCount != 0 )
	{
		CRect itemRect;
		m_pListCtrl->GetItemRect( 0, &itemRect, LVIR_BOUNDS );

		CRect clientRect;
		m_pListCtrl->GetClientRect( &clientRect );

		m_colsPerPage = clientRect.Width() / itemRect.Width();
		m_rowsPerPage = clientRect.Height() / itemRect.Height();
	}

#if 0
	int hSp, vSp, hSpSm, vSpSm;
	m_pListCtrl->GetItemSpacing( FALSE, &hSp, &vSp );
	m_pListCtrl->GetItemSpacing( TRUE, &hSpSm, &vSpSm );

	static int cnt = 0;
	TRACE( _T(" (%d) list-ctrl: caretIndex=%d, topIndex=%d, colsPerPage=%d, rowsPerPage=%d  hSp=%d, vSp=%d, hSpSm=%d, vSpSm=%d\n"),
		cnt++, m_caretIndex, m_topIndex, m_colsPerPage, m_rowsPerPage,
		hSp, vSp, hSpSm, vSpSm );
#endif
}

std::auto_ptr<CSelFlowSequence> CSelFlowSequence::MakeFlow( CReportListControl* pListCtrl )
{
	std::auto_ptr<CSelFlowSequence> pSelFlow;
	if ( pListCtrl->GetItemCount() > 1 )
		switch ( pListCtrl->GetView() )
		{
			case LV_VIEW_ICON:
			case LV_VIEW_SMALLICON:
			case LV_VIEW_TILE:
				pSelFlow.reset( new CSelFlowSequence( pListCtrl ) );		// key flow navigation necessary only in icon view mode
		}

	return pSelFlow;
}

bool CSelFlowSequence::HandleKeyDown( UINT chr )
{
	if ( m_itemCount != 0 )
		switch ( chr )
		{
			case VK_RIGHT:
				if ( LeftToRight == m_seqDirection )
					if ( m_caretIndex == m_itemCount - 1 )
						return false;						// list-ctrl bug workaround: avoid handling next caret when caret on last item
				break;
			case VK_DOWN:
				if ( TopToBottom == m_seqDirection )
					if ( m_caretIndex == m_itemCount - 1 )
						return false;
				break;
			case VK_PRIOR:
				break;
			case VK_NEXT:
				if ( TopToBottom == m_seqDirection )
					NavigateCaret( m_caretIndex + m_colsPerPage * m_rowsPerPage );
				return false;
		}

	return true;
}

bool CSelFlowSequence::PostKeyDown( UINT chr )
{
	if ( m_pListCtrl->GetCaretIndex() != m_caretIndex )
		return false;			// caret was changed by the list, navigation is done

	// stale caret, try navigating in sequence flow
	int newCaretIndex = m_caretIndex;
	switch ( chr )
	{
		case VK_LEFT:
			if ( LeftToRight == m_seqDirection )
				--newCaretIndex;
			break;
		case VK_RIGHT:
			if ( LeftToRight == m_seqDirection )
				++newCaretIndex;
			break;
		case VK_UP:
			if ( TopToBottom == m_seqDirection )
				--newCaretIndex;
			break;
		case VK_DOWN:
			if ( TopToBottom == m_seqDirection )
				++newCaretIndex;
			break;
		default:
			return false;
	}
	return NavigateCaret( newCaretIndex );
}

bool CSelFlowSequence::NavigateCaret( int newCaretIndex )
{
	if ( newCaretIndex == m_caretIndex ||											// no new caret (not a key of interest)
		 Range<int>( 0, m_itemCount - 1 ).Constrain( newCaretIndex ) )			// nowhere to go
		return false;			// caret changed, navigation done by the list ctrl

	if ( !ui::IsKeyPressed( VK_SHIFT ) && !ui::IsKeyPressed( VK_CONTROL ) )
	{
		CScopedInternalChange internalChange( m_pListCtrl );
		m_pListCtrl->ClearSelection();
	}

	UINT state = m_pListCtrl->GetItemState( newCaretIndex, LVNI_SELECTED );
	UINT stateMask = ui::IsKeyPressed( VK_CONTROL ) ? LVNI_FOCUSED : ( LVNI_SELECTED | LVNI_FOCUSED );

	SetFlag( state, LVNI_FOCUSED );
	SetFlag( state, LVNI_SELECTED, ui::IsKeyPressed( VK_SHIFT ) || !HasFlag( state, LVNI_SELECTED ) );

	m_pListCtrl->SetItemState( newCaretIndex, state, stateMask );
	m_pListCtrl->EnsureVisible( newCaretIndex, FALSE );
	return true;
}


// CListSelectionData implementation

CListSelectionData::CListSelectionData( CWnd* pSrcWnd /*= nullptr*/ )
	: ole::CTransferBlob( GetClipFormat() )
	, m_pSrcWnd( pSrcWnd )
{
}

CListSelectionData::CListSelectionData( CReportListControl* pSrcListCtrl )
	: ole::CTransferBlob( GetClipFormat() )
	, m_pSrcWnd( pSrcListCtrl )
{
	safe_ptr( pSrcListCtrl )->GetSelection( m_selIndexes );
}

CLIPFORMAT CListSelectionData::GetClipFormat( void )
{
	static const CLIPFORMAT clipFormat = static_cast<CLIPFORMAT>( ::RegisterClipboardFormat( _T("cfListSelIndexes") ) );
	return clipFormat;
}

void CListSelectionData::Save( CArchive& archive ) throws_( CException* )
{
	archive << (ULONGLONG)m_pSrcWnd->GetSafeHwnd();
	serial::SerializeValues( archive, m_selIndexes );
}

void CListSelectionData::Load( CArchive& archive ) throws_( CException* )
{
	ULONGLONG hSrcWnd;
	archive >> hSrcWnd;
	serial::SerializeValues( archive, m_selIndexes );

	// will work with windows within the same process
	m_pSrcWnd = CWnd::FromHandlePermanent( reinterpret_cast<HWND>( hSrcWnd ) );
}
