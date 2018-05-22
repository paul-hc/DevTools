
#include "stdafx.h"
#include "ReportListControl.h"
#include "ui_fwd.h"
#include "Image_fwd.h"
#include "ImageStore.h"
#include "MenuUtilities.h"
#include "ShellDragImager.h"
#include "OleDataSource.h"
#include "StringUtilities.h"
#include "StringRange.h"
#include "ThemeItem.h"
#include "UtilitiesEx.h"
#include "VisualTheme.h"
#include "ComparePredicates.h"
#include "ContainerUtilities.h"
#include "Resequence.hxx"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
}


static ACCEL keys[] =
{
	{ FVIRTKEY | FCONTROL, _T('A'), ID_EDIT_SELECT_ALL },
	{ FVIRTKEY | FCONTROL, _T('C'), ID_EDIT_COPY },
	{ FVIRTKEY | FCONTROL, VK_INSERT, ID_EDIT_COPY }
};

const TCHAR CReportListControl::s_fmtRegColumnLayout[] = _T("Width=%d, Order=%d");


CReportListControl::CReportListControl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= DefaultStyleEx*/ )
	: CListCtrl()
	, m_columnLayoutId( 0 )
	, m_listStyleEx( listStyleEx )
	, m_useExplorerTheme( true )
	, m_useAlternateRowColoring( false )
	, m_sortByColumn( -1 )			// no sorting by default
	, m_sortAscending( true )
	, m_sortInternally( true )
	, m_pCompareFunc( NULL )		// use text compare by default
	, m_pTextEffectCallback( NULL )
	, m_pImageList( NULL )
	, m_pLargeImageList( NULL )
	, m_useTriStateAutoCheck( false )
	, m_listAccel( keys, COUNT_OF( keys ) )
	, m_pDataSourceFactory( ole::GetStdDataSourceFactory() )
	, m_parentHandlesCustomDraw( -1 )
{
	m_pPopupMenu[ OnSelection ] = &GetStdPopupMenu( OnSelection );
	m_pPopupMenu[ Nowhere ] = &GetStdPopupMenu( Nowhere );

	if ( columnLayoutId != 0 )
		SetLayoutInfo( columnLayoutId );
}

CReportListControl::~CReportListControl()
{
}

bool CReportListControl::DeleteAllItems( void )
{
	m_markedCells.clear();
	return CListCtrl::DeleteAllItems() != FALSE;
}

void CReportListControl::SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, ImageListPos transpImgPos /*= -1*/ )
{
	ASSERT_NULL( m_hWnd );

	if ( pCustomImageDraw != NULL )
		m_pCustomImager.reset( new CCustomImager( pCustomImageDraw, transpImgPos ) );
	else
		m_pCustomImager.reset();
}

CMenu& CReportListControl::GetStdPopupMenu( ListPopup popupType )
{
	static CMenu stdPopupMenu[ _ListPopupCount ];
	CMenu& rMenu = stdPopupMenu[ popupType ];
	if ( NULL == rMenu.GetSafeHmenu() )
		ui::LoadPopupMenu( rMenu, IDR_STD_CONTEXT_MENU, OnSelection == popupType ? ui::ListViewSelectionPopup : ui::ListViewNowherePopup );
	return rMenu;
}

ui::CFontEffectCache* CReportListControl::GetFontEffectCache( void )
{
	if ( NULL == m_pFontCache.get() )
		m_pFontCache.reset( new ui::CFontEffectCache( GetFont() ) );
	return m_pFontCache.get();
}

void CReportListControl::SetupControl( void )
{
	if ( m_listStyleEx != 0 )
		SetExtendedStyle( m_listStyleEx );

	if ( m_useExplorerTheme )
		CVisualTheme::SetWindowTheme( m_hWnd, L"Explorer", NULL );		// enable Explorer theme

	if ( m_pCustomImager.get() != NULL )				// uses custom images?
		if ( -1 == m_pCustomImager->m_transpPos )		// no transparent entry is part of the image lists?
			AddTransparentImage();

	if ( m_pImageList != NULL )
		SetImageList( m_pImageList, LVSIL_SMALL );
	if ( m_pLargeImageList != NULL )
		SetImageList( m_pLargeImageList, LVSIL_NORMAL );

	if ( !m_regSection.empty() )
		LoadFromRegistry();
	else
		SetupColumnLayout( NULL );

	UpdateColumnSortHeader();		// update current sorting order
}

void CReportListControl::AddTransparentImage( void )
{
	ASSERT_PTR( m_pCustomImager.get() );
	ASSERT( -1 == m_pCustomImager->m_transpPos );

	if ( NULL == m_pImageList )
	{
		CSize imageSize = m_pCustomImager->m_pRenderer->GetItemImageSize( ui::ICustomImageDraw::SmallImage );
		m_pImageList = &m_pCustomImager->m_imageListSmall;
		m_pImageList->Create( imageSize.cx, imageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );
	}
	if ( NULL == m_pLargeImageList )
	{
		CSize imageSize = m_pCustomImager->m_pRenderer->GetItemImageSize( ui::ICustomImageDraw::LargeImage );
		m_pLargeImageList = &m_pCustomImager->m_imageListLarge;
		m_pLargeImageList->Create( imageSize.cx, imageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );
	}

	const CIcon* pTranspIcon = CImageStore::SharedStore()->RetrieveIcon( ID_TRANSPARENT );
	m_pCustomImager->m_transpPos = m_pImageList->Add( pTranspIcon->GetHandle() );
	m_pLargeImageList->Add( pTranspIcon->GetHandle() );
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
		if ( LV_VIEW_DETAILS == viewMode )
			ResizeFlexColumns();			// restretch flex columns
}

bool CReportListControl::SetCompactIconSpacing( int iconEdgeWidth /*= IconViewEdgeX*/ )
{
	CSize spacing( iconEdgeWidth * 2, 0 );
	if ( m_pLargeImageList != NULL )
		spacing.cx += gdi::GetImageSize( *m_pLargeImageList ).cx;
	else if ( m_pImageList != NULL )
		spacing.cx += gdi::GetImageSize( *m_pImageList ).cx;
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
			return CListCtrl::GetTopIndex();			// only works in report/list view mode
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

int CReportListControl::HitTest( CPoint pos, UINT* pFlags /*= NULL*/ ) const
{
	int result = CListCtrl::HitTest( pos, pFlags );

	if ( HasFlag( *pFlags, LVHT_NOWHERE ) )
		if ( int itemCount = GetItemCount() )
		{
			CRect lastItemRect;
			GetItemRect( itemCount - 1, &lastItemRect, LVIR_BOUNDS );

			if ( pos.y > lastItemRect.bottom )
				SetFlag( *pFlags, LVHT_MY_PASTEND );
			else
				switch ( GetView() )
				{
					case LV_VIEW_ICON:
					case LV_VIEW_SMALLICON:
					case LV_VIEW_TILE:
						if ( !EqFlag( GetStyle(), LVS_ALIGNLEFT ) )
							if ( pos.x > lastItemRect.right && pos.y > lastItemRect.top )				// to the right of last item?
								SetFlag( *pFlags, LVHT_MY_PASTEND );
				}
		}
		else
			SetFlag( *pFlags, LVHT_MY_PASTEND );

	return result;
}

int CReportListControl::GetDropIndexAtPoint( const CPoint& point ) const
{
	if ( this == WindowFromPoint( ui::ClientToScreen( m_hWnd, point ) ) )
	{
		UINT hitFlags;
		int newDropIndex = HitTest( point, &hitFlags );
		if ( HasFlag( hitFlags, LVHT_MY_PASTEND ) )
			newDropIndex = GetItemCount();					// drop past last item
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

void CReportListControl::SetSortByColumn( TColumn sortByColumn, bool sortAscending /*= true*/ )
{
	m_sortByColumn = sortByColumn;
	m_sortAscending = sortAscending;

	if ( m_hWnd != NULL )
		if ( IsSortingEnabled() )
			UpdateColumnSortHeader();
}

bool CReportListControl::SortList( void )
{
	UpdateColumnSortHeader();
	if ( -1 == m_sortByColumn )
		return false;

	if ( m_sortInternally )										// otherwise was sorted externally, just update sort header
		if ( m_pCompareFunc != NULL )
			SortItems( m_pCompareFunc, (LPARAM)this );			// passes item LPARAMs as left/right
		else
			SortItemsEx( m_pCompareFunc, (LPARAM)this );		// passes item indexes as left/right LPARAMs

	int caretIndex = GetCaretIndex();
	if ( caretIndex != -1 )
		EnsureVisible( caretIndex, FALSE );
	return true;
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

int CALLBACK CReportListControl::TextCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl )
{
	ASSERT_PTR( pListCtrl );
	return pListCtrl->CompareSubItems( leftParam, rightParam );
}

pred::CompareResult CReportListControl::CompareSubItems( LPARAM leftParam, LPARAM rightParam ) const
{
	REQUIRE( m_sortByColumn != -1 ); // must be sorted by one column

	int leftIndex = static_cast< int >( leftParam ), rightIndex = static_cast< int >( rightParam );
	ASSERT( leftIndex != -1 && rightIndex != -1 );

	CString leftText = GetItemText( leftIndex, m_sortByColumn );
	CString rightText = GetItemText( rightIndex, m_sortByColumn );

	pred::CompareResult result = str::IntuitiveCompare( leftText.GetString(), rightText.GetString() );

	enum { CodeColumn = 0 }; // first column, the one with the icon

	if ( pred::Equal == result && m_sortByColumn != CodeColumn )
	{	// equal by a secondary column -> compare by "code" column (first column)
		leftText = GetItemText( leftIndex, CodeColumn );
		rightText = GetItemText( rightIndex, CodeColumn );

		result = str::IntuitiveCompare( leftText.GetString(), rightText.GetString() );
	}

	return pred::GetResultInOrder( result, m_sortAscending );
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

void CReportListControl::QueryItemsText( std::vector< std::tstring >& rItemsText, const std::vector< int >& indexes, int subItem /*= 0*/ ) const
{
	rItemsText.reserve( rItemsText.size() + indexes.size() );

	for ( std::vector< int >::const_iterator itIndex = indexes.begin(); itIndex != indexes.end(); ++itIndex )
		rItemsText.push_back( GetItemText( *itIndex, subItem ).GetString() );
}

void CReportListControl::QuerySelectedItemsText( std::vector< std::tstring >& rItemsText, int subItem /*= 0*/ ) const
{
	std::vector< int > selIndexes;
	int caretIndex;
	GetSelection( selIndexes, &caretIndex );
	QueryItemsText( rItemsText, selIndexes, subItem );
}

void CReportListControl::QueryAllItemsText( std::vector< std::tstring >& rItemsText, int subItem /*= 0*/ ) const
{
	size_t itemCount = GetItemCount();
	rItemsText.clear();
	rItemsText.reserve( itemCount );

	for ( UINT i = 0; i != itemCount; ++i )
		rItemsText.push_back( GetItemText( i, subItem ).GetString() );
}

bool CReportListControl::IsSelectionChangedNotify( const NMLISTVIEW* pNmList, UINT selMask /*= LVIS_SELECTED | LVIS_FOCUSED*/ )
{
	ASSERT_PTR( pNmList );

	if ( HasFlag( pNmList->uChanged, LVIF_STATE ) )
		if ( ( pNmList->uNewState & selMask ) != ( pNmList->uOldState & selMask ) )
			return true;

	return false;
}

void CReportListControl::SetLayoutInfo( UINT columnLayoutId )
{
	REQUIRE( columnLayoutId != 0 );
	m_columnLayoutId = columnLayoutId;
	SetLayoutInfo( str::LoadStrings( columnLayoutId ) );
}

void CReportListControl::SetLayoutInfo( const std::vector< std::tstring >& columnSpecs )
{
	if ( m_hWnd != NULL )
		DeleteAllColumns();

	ParseColumnLayout( m_columnInfos, columnSpecs );

	if ( m_hWnd != NULL )
		InsertAllColumns();
}

void CReportListControl::ParseColumnLayout( std::vector< CColumnInfo >& rColumnInfos, const std::vector< std::tstring >& columnSpecs )
{
	ASSERT( !columnSpecs.empty() );

	/*	format examples:
			"Label"
			"Label=120" - 120 is the default column width;
			"Label=-1" - stretchable column width;
			"Label=-1/50" - stretchable column width, 50 minimum stretchable width;
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
		Range< size_t > sepPos;
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
	return pHeaderCtrl != NULL ? pHeaderCtrl->GetItemCount() : 0;
}

void CReportListControl::ResetColumnLayout( void )
{
	CScopedLockRedraw freeze( this );

	std::vector< std::tstring > columnSpecs = str::LoadStrings( m_columnLayoutId );
	ParseColumnLayout( m_columnInfos, columnSpecs );

	SetupColumnLayout( NULL );
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

void CReportListControl::SetupColumnLayout( const std::vector< std::tstring >* pRegColumnLayoutItems )
{
	if ( !HasLayoutInfo() )
		return;

	bool insertMode = 0 == GetColumnCount();
	bool ignoreSavedLayout = !insertMode || NULL == pRegColumnLayoutItems || pRegColumnLayoutItems->size() != m_columnInfos.size();

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
		tileViewInfo.cLines = static_cast< int >( m_tileColumns.size() );		// besides the Code column

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
	if ( NULL == m_hWnd )
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

			fitWidth = std::max< int >( colInfo.m_minWidth, fitWidth );
			SetColumnWidth( i, fitWidth );

			remainderWidth -= fitWidth;
		}
	}

	return true;
}

void CReportListControl::NotifyParent( Notification notifCode )
{
	ui::SendCommandToParent( m_hWnd, notifCode );
}

void CReportListControl::OnFinalReleaseInternalChange( void )
{
	ResizeFlexColumns();		// layout flexible columns after list content has changed
}

void CReportListControl::InputColumnLayout( std::vector< std::tstring >& rRegColumnLayoutItems )
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

	std::vector< std::tstring > regColumnLayoutItems;
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

		if ( IsSortingEnabled() )
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
	pApp->WriteProfileInt( m_regSection.c_str(), _T(""), (int)m_columnInfos.size() ); // saved column count

	std::vector< std::tstring > regColumnLayoutItems;
	InputColumnLayout( regColumnLayoutItems );
	ASSERT( regColumnLayoutItems.size() == m_columnInfos.size() );

	for ( UINT i = 0; i != m_columnInfos.size(); ++i )
		pApp->WriteProfileString( m_regSection.c_str(), m_columnInfos[ i ].m_label.c_str(), regColumnLayoutItems[ i ].c_str() );

	pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_viewMode, GetView() );
	pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_viewStacking, GetStyle() & LVS_ALIGNMASK );
	if ( IsSortingEnabled() )
	{
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_sortByColumn, m_sortByColumn );
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_sortAscending, m_sortAscending );
	}
}

int CReportListControl::InsertObjectItem( int index, utl::ISubject* pObject, int imageIndex /*= No_Image*/ )
{
	std::tstring displayCode;
	if ( pObject != NULL )
		displayCode = pObject->GetDisplayCode();

	if ( Transparent_Image == imageIndex )
		imageIndex = safe_ptr( m_pCustomImager.get() )->m_transpPos;

	int insertIndex = InsertItem( index, displayCode.c_str(), imageIndex );
	ASSERT( insertIndex != -1 );

	SetItemData( insertIndex, reinterpret_cast< DWORD_PTR >( pObject ) );
	SetItemTileInfo( insertIndex );
	return insertIndex;
}

void CReportListControl::SetSubItemText( int index, int subItem, const std::tstring& text, int imageIndex /*= No_Image*/ )
{	// it keeps 'text' argument alive
	ASSERT( subItem > 0 );

	if ( Transparent_Image == imageIndex )
		imageIndex = safe_ptr( m_pCustomImager.get() )->m_transpPos;
	else if ( text.empty() )
		imageIndex = No_Image;			// clear the image for empty text

	VERIFY( SetItem( index, subItem, LVIF_TEXT | LVIF_IMAGE, text.c_str(), imageIndex, 0, 0, 0 ) );
}

void CReportListControl::SetSubItemImage( int index, int subItem, int imageIndex )
{
	ASSERT( subItem > 0 );
	VERIFY( SetItem( index, subItem, LVIF_IMAGE, NULL, imageIndex, 0, 0, 0 ) );
}

bool CReportListControl::SetItemTileInfo( int index )
{
	if ( m_tileColumns.empty() )
		return false;

	LVTILEINFO tileInfo = { sizeof( LVTILEINFO ) };
	tileInfo.iItem = index;
	tileInfo.cColumns = static_cast< UINT >( m_tileColumns.size() );
	tileInfo.puColumns = &m_tileColumns.front();
	tileInfo.piColFmt = NULL;

	return SetTileInfo( &tileInfo ) != FALSE;
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

	while ( ui::IsValidWindow( hEdit ) && m_pLabelEdit.get() != NULL && !m_pLabelEdit->m_done )
		if ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
			if ( !AfxGetThread()->PumpMessage() )
			{
				::PostQuitMessage( 0 );
				return NULL;
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

void CReportListControl::DropMoveItems( int destIndex, const std::vector< int >& selIndexes )
{
	// assume that selIndexes are pre-sorted
	ASSERT( destIndex >= 0 && destIndex <= GetItemCount() );

	std::vector< CItemData > datas;
	datas.resize( selIndexes.size() );

	for ( size_t i = selIndexes.size(); i-- != 0; )
	{
		int index = selIndexes[ i ];

		GetItemDataAt( datas[ i ], index );
		DeleteItem( index );
		if ( index < destIndex )
			--destIndex;
	}

	for ( std::vector< CItemData >::const_iterator itItem = datas.begin(); itItem != datas.end(); ++itItem, ++destIndex )
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

int CReportListControl::Find( const void* pObject ) const
{
	for ( int i = 0, count = GetItemCount(); i != count; ++i )
		if ( pObject == GetPtrAt< void >( i ) )
			return i;

	return -1;
}

bool CReportListControl::IsCheckedState( UINT state )
{
	ASSERT( HasCheckState( state ) );
	switch ( state & LVIS_STATEIMAGEMASK )
	{
		case LVIS_CHECKED:
		case LVIS_CHECKEDGRAY:
		case LVIS_RADIO_CHECKED:
			return true;
	}
	return false;
}

bool CReportListControl::UseExtendedCheckStates( void ) const
{
	if ( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) )
		if ( CImageList* pStateList = GetImageList( LVSIL_STATE ) )
			if ( pStateList->GetImageCount() > 2 )
				return true;

	return false;
}

bool CReportListControl::SetupExtendedCheckStates( void )
{
	ASSERT( !UseExtendedCheckStates() );		// setup once
	ASSERT( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) );

	CImageList* pStateList = GetImageList( LVSIL_STATE );
	if ( NULL == pStateList )
		return false;

	CSize imageSize = CIconId::GetStdSize( SmallIcon );
	COLORREF transpColor = RGB( 255, 255, 254 );	// almost white: so that themes that render with alpha blending don't show weird colours (such as radio button)
	CBitmap imageBitmap, maskBitmap;

	CThemeItem checkedDisabled( L"BUTTON", BP_CHECKBOX, CBS_IMPLICITNORMAL );
	checkedDisabled.MakeBitmap( imageBitmap, transpColor, imageSize, H_AlignLeft | V_AlignTop );
	gdi::CreateBitmapMask( maskBitmap, imageBitmap, transpColor );
	pStateList->Add( &imageBitmap, &maskBitmap );

	CThemeItem radioUnchecked( L"BUTTON", BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL );
	radioUnchecked.MakeBitmap( imageBitmap, transpColor, imageSize, H_AlignLeft | V_AlignTop );
	gdi::CreateBitmapMask( maskBitmap, imageBitmap, transpColor );
	pStateList->Add( &imageBitmap, &maskBitmap );

	CThemeItem radioChecked( L"BUTTON", BP_RADIOBUTTON, RBS_CHECKEDNORMAL );
	radioChecked.MakeBitmap( imageBitmap, transpColor, imageSize, H_AlignLeft | V_AlignTop );
	gdi::CreateBitmapMask( maskBitmap, imageBitmap, transpColor );
	pStateList->Add( &imageBitmap, &maskBitmap );

	return true;
}

bool CReportListControl::SetCaretIndex( int index, bool doSet /*= true*/ )
{
	if ( !SetItemState( index, doSet ? LVIS_FOCUSED : 0, LVIS_FOCUSED ) )
		return false;

	if ( doSet )
		EnsureVisible( index, FALSE );
	return true;
}

int CReportListControl::GetCurSel( void ) const
{
	if ( !IsMultiSelectionList() )
		return FindItemWithState( LVNI_SELECTED );

	int selIndex = FindItemWithState( LVNI_SELECTED | LVNI_FOCUSED );
	if ( -1 == selIndex )
		selIndex = FindItemWithState( LVNI_SELECTED );

	return selIndex;
}

bool CReportListControl::SetCurSel( int index, bool doSelect /*= true*/ )
{
	if ( !SetItemState( index, doSelect ? ( LVIS_SELECTED | LVIS_FOCUSED ) : 0, LVIS_SELECTED | LVIS_FOCUSED ) )
		return false;

	if ( doSelect )
		EnsureVisible( index, FALSE );
	return true;
}

bool CReportListControl::GetSelection( std::vector< int >& rSelIndexes, int* pCaretIndex /*= NULL*/, int* pTopIndex /*= NULL*/ ) const
{
	rSelIndexes.reserve( rSelIndexes.size() + GetSelectedCount() );

	POSITION pos = GetFirstSelectedItemPosition();

	for ( int index = 0; pos != NULL; ++index )
		rSelIndexes.push_back( GetNextSelectedItem( pos ) );

	if ( pCaretIndex != NULL )
		*pCaretIndex = FindItemWithState( LVNI_FOCUSED );

	if ( pTopIndex != NULL )
		*pTopIndex = GetTopIndex();

	std::sort( rSelIndexes.begin(), rSelIndexes.end() );		// sort indexes ascending
	return !rSelIndexes.empty();
}

void CReportListControl::Select( const void* pObject )
{
	int indexFound = Find( pObject );
	if ( IsMultiSelectionList() )
		if ( indexFound != -1 )
		{
			std::vector< int > selIndexes;
			selIndexes.push_back( indexFound );

			SetSelection( selIndexes, indexFound );
		}
		else
			ClearSelection();
	else
		SetCurSel( indexFound );
}

void CReportListControl::SetSelection( const std::vector< int >& selIndexes, int caretIndex /*= -1*/ )
{
	// clear the selection
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		SetItemState( i, 0, LVNI_SELECTED | LVNI_FOCUSED );

	// select new items
	for ( size_t i = 0; i != selIndexes.size(); ++i )
		SetItemState( selIndexes[ i ], LVNI_SELECTED, LVNI_SELECTED );

	if ( -1 == caretIndex && !selIndexes.empty() )
		caretIndex = selIndexes.back(); // caret on the last selected

	if ( caretIndex != -1 )
	{
		SetItemState( caretIndex, LVNI_FOCUSED, LVNI_FOCUSED );
		SetSelectionMark( caretIndex );
		EnsureVisible( caretIndex, false );
	}
}

void CReportListControl::ClearSelection( void )
{
	std::vector< int > selIndexes;
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

void CReportListControl::MoveSelectionTo( seq::MoveTo moveTo )
{
	{
		CScopedInternalChange internalChange( this );

		std::vector< int > selIndexes;
		GetSelection( selIndexes );

		CListCtrlSequence sequence( this );
		seq::Resequence( sequence, selIndexes, moveTo );
	}

	NotifyParent( LVN_ItemsReorder );
}

bool CReportListControl::CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags, const CListSelectionData& selData ) const
{
	ASSERT_PTR( pDataSource );

	if ( !selData.IsValid() || !HasFlag( sourceFlags, ListSourcesMask ) )
		return false;

	if ( HasFlag( sourceFlags, ds::Indexes ) )
		selData.CacheTo( pDataSource );

	std::vector< std::tstring > codes;
	if ( HasFlag( sourceFlags, ds::ItemsText | ds::ShellFiles ) )
		for ( std::vector< int >::const_iterator itSelIndex = selData.m_selIndexes.begin(); itSelIndex != selData.m_selIndexes.end(); ++itSelIndex )
			if ( utl::ISubject* pObject = GetObjectAt( *itSelIndex ) )
				codes.push_back( pObject->GetCode() );		// assume GetCode() represents a full path
			else
				codes.push_back( GetItemText( *itSelIndex, Code ).GetString() );

	if ( HasFlag( sourceFlags, ds::ItemsText ) )
		ole_utl::CacheTextData( pDataSource, str::Join( codes, _T("\r\n") ) );

	if ( HasFlag( sourceFlags, ds::ShellFiles ) )
		pDataSource->CacheShellFilePaths( codes );

	return true;
}

bool CReportListControl::Copy( int sourceFlags /*= ListSourcesMask*/ )
{
	// inspired from COleServerItem::CopyToClipboard() in MFC
	std::auto_ptr< ole::CDataSource > pDataSource( m_pDataSourceFactory->NewDataSource() );
	if ( CacheSelectionData( pDataSource.get(), sourceFlags ) )
	{	// put it on the clipboard and let the clipboard manage (own) the data source
		pDataSource->SetClipboard();
		pDataSource.release();
		return true;
	}
	return false;
}

std::auto_ptr< CImageList > CReportListControl::CreateDragImageMulti( const std::vector< int >& indexes, CPoint* pFrameOrigin /*= NULL*/ )
{
	std::auto_ptr< CImageList > pDragImage;			// imagelist with the merged drag images
	if ( !indexes.empty() )
	{
		SHDRAGIMAGE shDragImage;
		if ( shell::GetDragImage( shDragImage, m_hWnd ) != NULL )
		{
			pDragImage.reset( new CImageList );
			pDragImage->Create( shDragImage.sizeDragImage.cx, shDragImage.sizeDragImage.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );		// 1 image with all items

			// API level call to skip attaching a temp CBitmap; note that shell drag images owns the bitmap.
		#if _MSC_VER > 1500		// >MSVC++ 9.0 (Visual Studio 2008)
			ImageList_AddMasked( pDragImage->GetSafeHandle(), shDragImage.hbmpDragImage, shDragImage.crColorKey );
		#else
			AfxImageList_AddMasked( pDragImage->GetSafeHandle(), shDragImage.hbmpDragImage, shDragImage.crColorKey );
		#endif

			if ( pFrameOrigin != NULL )
				*pFrameOrigin = GetFrameBounds( indexes ).TopLeft();	// offset of the current mouse cursor to the imagelist we can use in BeginDrag()
		}
	}
	return pDragImage;
}

std::auto_ptr< CImageList > CReportListControl::CreateDragImageSelection( CPoint* pFrameOrigin /*= NULL*/ )
{
	std::vector< int > selIndexes;
	GetSelection( selIndexes );
	return CreateDragImageMulti( selIndexes, pFrameOrigin );
}

CRect CReportListControl::GetFrameBounds( const std::vector< int >& indexes ) const
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
	utl::ISubject* pSubject = GetObjectAt( index );
	ASSERT_PTR( pSubject );

	if ( subItem != EntireRecord )
		m_markedCells[ std::make_pair( pSubject, subItem ) ] = textEfect;
	else
		for ( TColumn subItem = 0; subItem != (TColumn)m_columnInfos.size(); ++subItem )
			MarkCellAt( index, subItem, textEfect );
}

void CReportListControl::UnmarkCellAt( int index, TColumn subItem )
{
	if ( subItem != EntireRecord )
	{
		stdext::hash_map< TCellPair, ui::CTextEffect >::iterator itFound = m_markedCells.find( std::make_pair( GetObjectAt( index ), subItem ) );
		if ( itFound != m_markedCells.end() )
			m_markedCells.erase( itFound );
	}
	else
	{
		for ( TColumn subItem = 0; subItem != (TColumn)m_columnInfos.size(); ++subItem )
			UnmarkCellAt( index, subItem );
	}
}

void CReportListControl::ClearMarkedCells( void )
{
	m_markedCells.clear();
	Invalidate();
}

const ui::CTextEffect* CReportListControl::FindTextEffectAt( utl::ISubject* pSubject, TColumn subItem ) const
{
	return utl::FindValuePtr( m_markedCells, std::make_pair( pSubject, subItem ) );
}

const ui::CTextEffect& CReportListControl::LookupTextEffectAt( utl::ISubject* pSubject, TColumn subItem ) const
{
	if ( const ui::CTextEffect* pFoundEffect = FindTextEffectAt( pSubject, subItem ) )
		return *pFoundEffect;

	// since we do sub-item level custom drawing, we need to rollback to default text effect for the subsequent sub-items
	static const ui::CTextEffect defaultEffect;
	return defaultEffect;
}

bool CReportListControl::ApplyTextEffectAt( NMLVCUSTOMDRAW* pDraw, utl::ISubject* pSubject, TColumn subItem )
{
	ASSERT_PTR( pDraw );
	int index = static_cast< int >( pDraw->nmcd.dwItemSpec ); index;		// for debugging

	if ( m_markedCells.empty() && NULL == m_pTextEffectCallback )
		return false;

	ui::CTextEffect textEffect = LookupTextEffectAt( pSubject, subItem );				// cell effect

	if ( m_pTextEffectCallback != NULL )
		m_pTextEffectCallback->CombineTextEffectAt( textEffect, pSubject, subItem );	// combine with calback effect

	bool modified = false;

	if ( CFont* pFont = GetFontEffectCache()->Lookup( textEffect.m_fontEffect ) )
		if ( ::SelectObject( pDraw->nmcd.hdc, *pFont ) != *pFont )
			modified = true;

	// when assigning CLR_NONE, the list view uses the default colour properly: this->GetTextColor(), this->GetBkColor()
	if ( textEffect.m_textColor != pDraw->clrText )
	{
		pDraw->clrText = textEffect.m_textColor;
		modified = true;
	}

	if ( textEffect.m_bkColor != pDraw->clrTextBk )
	{
		pDraw->clrTextBk = textEffect.m_bkColor;
		modified = true;
	}

	return modified;
}

bool CReportListControl::ParentHandlesCustomDraw( void )
{
	if ( -1 == m_parentHandlesCustomDraw )
		m_parentHandlesCustomDraw = ui::ParentContainsMessageHandler( this, WM_NOTIFY, NM_CUSTOMDRAW );

	return m_parentHandlesCustomDraw != FALSE;
}


// groups

#if ( _WIN32_WINNT >= 0x0600 ) && defined( UNICODE )

int CReportListControl::GetGroupId( int groupIndex ) const
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_GROUPID;
	VERIFY( GetGroupInfoByIndex( groupIndex, &group ) );
	return group.iGroupId;
}

int CReportListControl::GetRowGroupId( int rowIndex ) const
{
	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = rowIndex;
	VERIFY( GetItem( &item ) );
	return item.iGroupId;
}

bool CReportListControl::SetRowGroupId( int rowIndex, int groupId )
{
	// rows not assigned to a group will not show in group-view
	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = rowIndex;
	item.iSubItem = 0;
	item.iGroupId = groupId;
	return SetItem( &item ) != FALSE;
}

std::tstring CReportListControl::GetGroupHeaderText( int groupId ) const
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_HEADER | LVGF_GROUPID;
	group.iGroupId = groupId;
	VERIFY( GetGroupInfo( groupId, &group ) != -1 );
	return group.pszHeader;
}

int CReportListControl::InsertGroupHeader( int index, int groupId, const std::tstring& header, DWORD state /*= LVGS_NORMAL*/, DWORD align /*= LVGA_HEADER_LEFT*/ )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_GROUPID | LVGF_STATE | LVGF_ALIGN | LVGF_HEADER;
	group.iGroupId = groupId;
	group.state = state;
	group.uAlign = align;
	group.pszHeader = const_cast< TCHAR* >( header.c_str() );

	return InsertGroup( index, &group );
}

bool CReportListControl::SetGroupFooter( int groupId, const std::tstring& footer, DWORD align /*= LVGA_FOOTER_CENTER*/ )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_FOOTER | LVGF_ALIGN;
	group.uAlign = align;
	group.pszFooter = const_cast< TCHAR* >( footer.c_str() );

	return SetGroupInfo( groupId, &group ) != -1;
}

bool CReportListControl::SetGroupTask( int groupId, const std::tstring& task )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_TASK;
	group.pszTask = const_cast< TCHAR* >( task.c_str() );

	return SetGroupInfo( groupId, &group ) != -1;
}

bool CReportListControl::SetGroupSubtitle( int groupId, const std::tstring& subtitle )
{
	LVGROUP group = { sizeof( LVGROUP ) };
	group.mask = LVGF_SUBTITLE;
	group.pszSubtitle = const_cast< TCHAR* >( subtitle.c_str() );

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
		group.pszDescriptionTop = const_cast< TCHAR* >( topDescr.c_str() );
	}
	if ( !bottomDescr.empty() )
	{	// bottom description is drawn under the top description text when there is a title image, no extended image, and uAlign==LVGA_HEADER_CENTER
		group.mask |= LVGF_DESCRIPTIONBOTTOM;
		group.pszDescriptionBottom = const_cast< TCHAR* >( bottomDescr.c_str() );
	}

	return SetGroupInfo( groupId, &group ) != -1;
}

void CReportListControl::DeleteEntireGroup( int groupId )
{
	for ( int rowIndex = 0, count = GetItemCount(); rowIndex != count; ++rowIndex )
		if ( GetRowGroupId( rowIndex ) == groupId )
			DeleteItem( rowIndex-- );

	RemoveGroup( groupId );
}

void CReportListControl::CheckEntireGroup( int groupId, bool check /*= true*/ )
{
	if ( HasFlag( GetExtendedStyle(), LVS_EX_CHECKBOXES ) )
		for ( int rowIndex = 0, count = GetItemCount(); rowIndex != count; ++rowIndex )
			if ( GetRowGroupId( rowIndex ) == groupId )
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

int CReportListControl::GroupHitTest( const CPoint& point ) const
{
	if ( !IsGroupViewEnabled() || 0 == GetItemCount() || HitTest( point ) != -1 )
		return -1;

	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
	{
		int groupId = GetGroupId( i );
		CRect rect( 0, LVGGR_HEADER, 0, 0 );
		VERIFY( GetGroupRect( groupId, &rect, LVGGR_HEADER ) );
		if ( rect.PtInRect( point ) )
			return groupId;
	}
	return -1;			// don't try other ways to find the group
}

#endif // Vista groups


void CReportListControl::PreSubclassWindow( void )
{
	CListCtrl::PreSubclassWindow();
	SetupControl();
}

BOOL CReportListControl::PreTranslateMessage( MSG* pMsg )
{
	return
		m_listAccel.Translate( pMsg, m_hWnd, m_hWnd ) ||
		CListCtrl::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CReportListControl, CListCtrl )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_KEYDOWN()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_INITMENUPOPUP()
	ON_NOTIFY_REFLECT_EX( LVN_ITEMCHANGING, OnLvnItemChanging_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_ITEMCHANGED, OnLvnItemChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( HDN_ITEMCHANGING, OnHdnItemChanging_Reflect )
	ON_NOTIFY_REFLECT_EX( HDN_ITEMCHANGED, OnHdnItemChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_COLUMNCLICK, OnLvnColumnClick_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_BEGINLABELEDIT, OnLvnBeginLabelEdit_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_ENDLABELEDIT, OnLvnEndLabelEdit_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_CUSTOMDRAW, OnNmCustomDraw_Reflect )
	ON_COMMAND_RANGE( ID_LIST_VIEW_ICON_LARGE, ID_LIST_VIEW_TILE, OnListViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_LIST_VIEW_ICON_LARGE, ID_LIST_VIEW_TILE, OnUpdateListViewMode )
	ON_COMMAND_RANGE( ID_LIST_VIEW_STACK_LEFTTORIGHT, ID_LIST_VIEW_STACK_TOPTOBOTTOM, OnListViewStacking )
	ON_UPDATE_COMMAND_UI_RANGE( ID_LIST_VIEW_STACK_LEFTTORIGHT, ID_LIST_VIEW_STACK_TOPTOBOTTOM, OnUpdateListViewStacking )
	ON_COMMAND( ID_RESET_DEFAULT, OnResetColumnLayout )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdateResetColumnLayout )
	ON_COMMAND( ID_EDIT_COPY, OnCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopy )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnSelectAll )
	ON_UPDATE_COMMAND_UI( ID_EDIT_SELECT_ALL, OnUpdateSelectAll )
	ON_COMMAND_RANGE( ID_MOVE_UP_ITEM, ID_MOVE_BOTTOM_ITEM, OnMoveTo )
	ON_UPDATE_COMMAND_UI_RANGE( ID_MOVE_UP_ITEM, ID_MOVE_BOTTOM_ITEM, OnUpdateMoveTo )
END_MESSAGE_MAP()

int CReportListControl::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CListCtrl::OnCreate( pCreateStruct ) )
		return -1;

	SetupControl();
	return 0;
}

void CReportListControl::OnDestroy( void )
{
	if ( !m_regSection.empty() )
		SaveToRegistry();

	CListCtrl::OnDestroy();
}

void CReportListControl::OnWindowPosChanged( WINDOWPOS* pWndPos )
{
	CListCtrl::OnWindowPosChanged( pWndPos );

	if ( !HasFlag( pWndPos->flags, SWP_NOMOVE ) || !HasFlag( pWndPos->flags, SWP_NOSIZE ) )
		ResizeFlexColumns();
}

void CReportListControl::OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags )
{
	std::auto_ptr< CSelFlowSequence > pSelFlow = CSelFlowSequence::MakeFlow( this );

	if ( NULL == pSelFlow.get() || pSelFlow->HandleKeyDown( chr ) )
	{
		CListCtrl::OnKeyDown( chr, repCnt, vkFlags );

		if ( pSelFlow.get() != NULL )
			pSelFlow->PostKeyDown( chr );
	}
}

void CReportListControl::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	CListCtrl::OnNcLButtonDown( hitTest, point );
}

void CReportListControl::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	CMenu* pPopupMenu = NULL;
	UINT flags;
	int hitIndex = HitTest( ui::ScreenToClient( m_hWnd, screenPos ), &flags );

	if ( pWnd == GetHeaderCtrl() || -1 == hitIndex )		// clicked on header or nowhere?
		pPopupMenu = m_pPopupMenu[ Nowhere ];
	else if ( HasFlag( flags, LVHT_ONITEM ) )
		pPopupMenu = m_pPopupMenu[ OnSelection ];

	if ( pPopupMenu != NULL )
	{
		ui::TrackPopupMenu( *pPopupMenu, this, screenPos );
		return;					// supress rising WM_CONTEXTMENU to the parent
	}

	Default();
}

void CReportListControl::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	CListCtrl::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

BOOL CReportListControl::OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pListInfo = (NMLISTVIEW*)pNmHdr;
	UNUSED_ALWAYS( pListInfo );

	if ( !m_useTriStateAutoCheck )		// must prevent prevent tri-state transition?
		if ( HasFlag( pListInfo->uChanged, LVIF_STATE ) && StateChanged( pListInfo->uNewState, pListInfo->uOldState, LVIS_STATEIMAGEMASK ) )	// check state changed
			if ( LVIS_CHECKEDGRAY == AsCheckState( pListInfo->uNewState ) )
			{
				SetCheckState( pListInfo->iItem, LVIS_UNCHECKED );
				*pResult = 1;
				return TRUE;
			}

	*pResult = 0L;
	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CReportListControl::OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pListInfo = (NMLISTVIEW*)pNmHdr;
	UNUSED_ALWAYS( pListInfo );
	*pResult = 0L;
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
	NM_LISTVIEW* pNmListView = (NM_LISTVIEW*)pNmHdr;

	*pResult = 0;

	if ( ui::IsKeyPressed( VK_CONTROL ) || ui::IsKeyPressed( VK_SHIFT ) )
		SetSortByColumn( -1 );											// switch to no sorting
	else
	{
		int sortByColumn = pNmListView->iSubItem;

		// switch sorting
		if ( sortByColumn != m_sortByColumn )
			SetSortByColumn( sortByColumn );							// sort ascending by a new column
		else
			if ( sortByColumn != -1 )
				SetSortByColumn( sortByColumn, !m_sortAscending );		// toggle ascending/descending on the same column
	}

	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnLvnBeginLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNmHdr;

	m_pLabelEdit.reset( new CLabelEdit( pDispInfo->item.iItem, GetItemText( pDispInfo->item.iItem, 0 ).GetString() ) );
	*pResult = 0;
	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnLvnEndLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNmHdr;
	*pResult = 0;

	if ( m_pLabelEdit.get() != NULL )
		if ( pDispInfo->item.pszText != NULL )
		{
			m_pLabelEdit->m_done = true;
			m_pLabelEdit->m_newLabel = pDispInfo->item.pszText;
			*pResult = TRUE;		// assume valid input
			return TRUE;
		}
		else
			m_pLabelEdit.reset();

	return FALSE;			// raise the notification to parent
}

BOOL CReportListControl::OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pDraw = (NMLVCUSTOMDRAW*)pNmHdr;
	int index = static_cast< int >( pDraw->nmcd.dwItemSpec );

	*pResult = CDRF_DODEFAULT;

	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			if ( m_useAlternateRowColoring )
				if ( index & 0x01 )
				{
					pDraw->clrTextBk = color::GhostWhite;
					*pResult = CDRF_NEWFONT;
				}

			*pResult = CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;

			if ( m_pCustomImager.get() != NULL )
				if ( IsItemVisible( index ) )
					*pResult |= CDRF_NOTIFYPOSTPAINT;		// will superimpose the thumbnails on top of transparent image

			break;
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			if ( ApplyTextEffectAt( pDraw, AsPtr< utl::ISubject >( pDraw->nmcd.lItemlParam ), pDraw->iSubItem ) )
				*pResult |= CDRF_NEWFONT;
			break;
		case CDDS_ITEMPOSTPAINT:
			if ( m_pCustomImager.get() != NULL )
			{
				CRect itemImageRect;
				if ( GetItemRect( index, itemImageRect, LVIR_ICON ) )		// item is visible?
					if ( m_pCustomImager->m_pRenderer->CustomDrawItemImage( &pDraw->nmcd, itemImageRect ) )
						return TRUE;		// handled
			}
			break;
	}

	if ( !ParentHandlesCustomDraw() )
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
	bool modeHasImages = ( LV_VIEW_ICON == viewMode ? m_pLargeImageList : m_pImageList ) != NULL;

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

void CReportListControl::OnUpdateCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( FindItemWithState( LVNI_SELECTED ) != -1 );
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
	std::vector< int > selIndexes;
	GetSelection( selIndexes );

	pCmdUI->Enable( seq::CanMoveSelection( GetItemCount(), selIndexes, lv::CmdIdToMoveTo( pCmdUI->m_nID ) ) );
}


// CScopedListTextSelection implementation

CScopedListTextSelection::CScopedListTextSelection( CReportListControl* pListCtrl )
	: m_pListCtrl( pListCtrl )
{
	ASSERT_PTR( m_pListCtrl );

	int caretIndex;
	std::vector< int > selIndexes;
	m_pListCtrl->GetSelection( selIndexes, &caretIndex );

	m_pListCtrl->QueryItemsText( m_selTexts, selIndexes );
	if ( caretIndex != -1 )
		m_caretText = m_pListCtrl->GetItemText( caretIndex, 0 ).GetString();
}

CScopedListTextSelection::~CScopedListTextSelection()
{
	std::vector< int > selIndexes;
	selIndexes.reserve( m_selTexts.size() );
	for ( std::vector< std::tstring >::const_iterator itSelText = m_selTexts.begin(); itSelText != m_selTexts.end(); ++itSelText )
	{
		int foundIndex = m_pListCtrl->FindItemIndex( *itSelText );
		if ( foundIndex != -1 )
			selIndexes.push_back( foundIndex );
	}
	std::sort( selIndexes.begin(), selIndexes.end() );

	int caretIndex = !m_caretText.empty() ? m_pListCtrl->FindItemIndex( m_caretText ) : -1;
	CScopedInternalChange internalChange( m_pListCtrl );
	m_pListCtrl->SetSelection( selIndexes, caretIndex );			// internal change
}


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

int hSp, vSp, hSpSm, vSpSm;
m_pListCtrl->GetItemSpacing( FALSE, &hSp, &vSp );
m_pListCtrl->GetItemSpacing( TRUE, &hSpSm, &vSpSm );

static int cnt = 0;
TRACE( _T(" (%d) list-ctrl: caretIndex=%d, topIndex=%d, colsPerPage=%d, rowsPerPage=%d  hSp=%d, vSp=%d, hSpSm=%d, vSpSm=%d\n"),
	cnt++, m_caretIndex, m_topIndex, m_colsPerPage, m_rowsPerPage,
	hSp, vSp, hSpSm, vSpSm );
}

std::auto_ptr< CSelFlowSequence > CSelFlowSequence::MakeFlow( CReportListControl* pListCtrl )
{
	std::auto_ptr< CSelFlowSequence > pSelFlow;
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
		 Range< int >( 0, m_itemCount - 1 ).Constrain( newCaretIndex ) )			// nowhere to go
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

CListSelectionData::CListSelectionData( CWnd* pSrcWnd /*= NULL*/ )
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
	static const CLIPFORMAT clipFormat = static_cast< CLIPFORMAT >( ::RegisterClipboardFormat( _T("cfListSelIndexes") ) );
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
	m_pSrcWnd = CWnd::FromHandlePermanent( reinterpret_cast< HWND >( hSrcWnd ) );
}
