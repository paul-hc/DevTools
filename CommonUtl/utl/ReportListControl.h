#ifndef ReportListControl_h
#define ReportListControl_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "ISubject.h"
#include "SubjectPredicates.h"
#include "CustomDrawImager_fwd.h"
#include "ObjectCtrlBase.h"
#include "OleUtils.h"
#include "MatchSequence.h"
#include "Resequence.h"
#include "TextEffect.h"
#include "vector_map.h"
#include <vector>
#include <list>
#include <hash_map>
#include <afxcmn.h>


class CListSelectionData;
class CReportListCustomDraw;
namespace ole { class CDataSource; }
class CBaseCustomDrawImager;

namespace ui
{
	interface ISubjectAdapter;
	class CFontEffectCache;
}

namespace ds
{
	enum DataSourceFlags
	{
		Indexes		= BIT_FLAG( 0 ),		// "cfListSelIndexes"
		ItemsText	= BIT_FLAG( 1 ),		// CF_TEXT, CF_UNICODETEXT
		ShellFiles	= BIT_FLAG( 2 )			// cfHDROP
	};
}

namespace lv
{
	enum SubPopup { NowhereSubPopup, OnSelectionSubPopup, PathItemNowhereSubPopup, PathItemOnSelectionSubPopup };

	// list view states, 1-based indexes
	enum CheckState
	{
		// predefined check states: existing images in GetImageList( LVSIL_STATE )
		LVIS_UNCHECKED			= INDEXTOSTATEIMAGEMASK( 1 ),		// BST_UNCHECKED + 1
		LVIS_CHECKED			= INDEXTOSTATEIMAGEMASK( 2 ),		// BST_CHECKED + 1

		// extended check states
		LVIS_CHECKEDGRAY		= INDEXTOSTATEIMAGEMASK( 3 ),		// BST_INDETERMINATE + 1
		LVIS_RADIO_UNCHECKED	= INDEXTOSTATEIMAGEMASK( 4 ),
		LVIS_RADIO_CHECKED		= INDEXTOSTATEIMAGEMASK( 5 )
	};

	enum
	{
		DefaultStyleEx = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
			LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_AUTOAUTOARRANGE /*| LVS_EX_JUSTIFYCOLUMNS*/

		// note: LVS_EX_DOUBLEBUFFER causes problems of scaling artifacts with CustomImageDraw; remove this styleEx for accurate scaling.
	};


	enum Notification
	{
		// via WM_COMMAND:
		LVN_ItemsReorder = 1000,

		// via WM_NOTIFY:
		LVN_CustomSortList,					// pass NMHDR; clients return TRUE if handled custom sorting, using current sorting criteria GetSortByColumn()
		LVN_DropFiles						// pass lv::CNmDropFiles
	};


	struct CNmHdr : public tagNMHDR
	{
		CNmHdr( const CListCtrl* pListCtrl, Notification notifyCode )
		{
			hwndFrom = pListCtrl->GetSafeHwnd();
			idFrom = pListCtrl->GetDlgCtrlID();
			code = notifyCode;
		}
	};


	struct CNmDropFiles
	{
		CNmDropFiles( const CListCtrl* pListCtrl, const CPoint& dropPoint, int dropItemIndex )
			: m_nmhdr( pListCtrl, LVN_DropFiles )
			, m_dropPoint( dropPoint )
			, m_dropItemIndex( dropItemIndex )
		{
		}
	public:
		CNmHdr m_nmhdr;
		CPoint m_dropPoint;							// client coordinates
		int m_dropItemIndex;
		std::vector< std::tstring > m_filePaths;	// sorted intuitively
	};
}


abstract class CListTraits			// defines types shared between CReportListControl and CReportListCustomDraw classes
{
public:
	typedef LPARAM TRowKey;			// row keys are invariant to sorting
	typedef int TColumn;

	enum StdColumn { Code, EntireRecord = (TColumn)-1 };

	struct CMatchEffects
	{
		CMatchEffects( std::vector< ui::CTextEffect >& rMatchEffects )
			: m_rEqual( rMatchEffects[ str::MatchEqual ] )
			, m_rEqualDiffCase( rMatchEffects[ str::MatchEqualDiffCase ] )
			, m_rNotEqual( rMatchEffects[ str::MatchNotEqual ] )
		{
		}
	public:
		ui::CTextEffect& m_rEqual;
		ui::CTextEffect& m_rEqualDiffCase;
		ui::CTextEffect& m_rNotEqual;
	};
protected:
	enum DiffSide { SrcDiff, DestDiff };

	struct CDiffColumnPair
	{
		CDiffColumnPair( TColumn srcColumn = -1, TColumn destColumn = -1 ) : m_srcColumn( srcColumn ), m_destColumn( destColumn ) {}

		bool HasColumn( TColumn column ) const { return m_srcColumn == column || m_destColumn == column; }
		const str::TMatchSequence* FindRowSequence( TRowKey rowKey ) const;

		DiffSide GetDiffSide( TColumn subItem ) const { REQUIRE( m_srcColumn == subItem || m_destColumn == subItem ); return m_srcColumn == subItem ? SrcDiff : DestDiff; }
	public:
		TColumn m_srcColumn;
		TColumn m_destColumn;
		stdext::hash_map< TRowKey, str::TMatchSequence > m_rowSequences;		// TRowKey is invariant to sorting
	};
};


class CReportListControl : public CListCtrl
						 , public CInternalChange
						 , public CListTraits
						 , public CObjectCtrlBase
						 , public ICustomDrawControl
{
	friend class CReportListCustomDraw;
public:
	CReportListControl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CReportListControl();

	DWORD& RefListStyleEx( void ) { return m_listStyleEx; }
	bool ModifyListStyleEx( DWORD dwRemove, DWORD dwAdd, UINT swpFlags = 0 );

	void SaveToRegistry( void );
	bool LoadFromRegistry( void );

	void Set_ImageList( CImageList* pImageList, CImageList* pLargeImageList = NULL ) { m_pImageList = pImageList; m_pLargeImageList = pLargeImageList; }
	void ChangeListViewMode( DWORD viewMode );

	bool IsMultiSelectionList( void ) const { return !HasFlag( GetStyle(), LVS_SINGLESEL ); }

	bool GetUseExplorerTheme( void ) const { return HasFlag( m_optionFlags, UseExplorerTheme ); }
	void SetUseExplorerTheme( bool useExplorerTheme = true );

	bool GetUseAlternateRowColoring( void ) const { return HasFlag( m_optionFlags, UseAlternateRowColoring ); }
	void SetUseAlternateRowColoring( bool useAlternateRowColoring = true ) { SetOptionFlag( UseAlternateRowColoring, useAlternateRowColoring ); }

	bool GetHighlightTextDiffsFrame( void ) const { return HasFlag( m_optionFlags, HighlightTextDiffsFrame ); }
	void SetHighlightTextDiffsFrame( bool highlightTextDiffsFrame = true ) { SetOptionFlag( HighlightTextDiffsFrame, highlightTextDiffsFrame ); }

	bool GetAcceptDropFiles( void ) const { return HasFlag( m_optionFlags, AcceptDropFiles ); }
	void SetAcceptDropFiles( bool acceptDropFiles = true ) { SetOptionFlag( AcceptDropFiles, acceptDropFiles ); }

	const std::tstring& GetSection( void ) const { return m_regSection; }
	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }
public:
	enum ListPopup { Nowhere, OnSelection, _ListPopupCount };

	void SetPopupMenu( ListPopup popupType, CMenu* pPopupMenu ) { m_pPopupMenu[ popupType ] = pPopupMenu; }		// set pPopupMenu to NULL to allow tracking context menu by parent dialog
	void SetTrackMenuTarget( CWnd* pTrackMenuTarget ) { m_pTrackMenuTarget = pTrackMenuTarget; }

	virtual CMenu* GetPopupMenu( ListPopup popupType );

	static CMenu& GetStdPopupMenu( ListPopup popupType );

	ole::IDataSourceFactory* GetDataSourceFactory( void ) const { return m_pDataSourceFactory; }
	void SetDataSourceFactory( ole::IDataSourceFactory* pDataSourceFactory ) { ASSERT_PTR( pDataSourceFactory ); m_pDataSourceFactory = pDataSourceFactory; }

	typedef int ImageListIndex;

	ui::GlyphGauge GetViewModeGlyphGauge( void ) const { return GetViewModeGlyphGauge( GetView() ); }
	static ui::GlyphGauge GetViewModeGlyphGauge( DWORD listViewMode );

	// ICustomDrawControl interface
	virtual CBaseCustomDrawImager* GetCustomDrawImager( void ) const;
	virtual void SetCustomFileGlyphDraw( bool showGlyphs = true );

	// To prevent icon scaling dithering when displaying shell item icon thumbnails, specify small/large image bounds size from the image list.
	// Otherwise when displaying only image file thumbs, let the thumbnailer drives bounds sizes.
	// Remove LVS_EX_DOUBLEBUFFER for accurate image scaling, especially for transparent PNGs.
	//
	void SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, const CSize& smallImageSize = CSize( 0, 0 ), const CSize& largeImageSize = CSize( 0, 0 ) );
	void SetCustomIconDraw( ui::ICustomImageDraw* pCustomIconDraw, IconStdSize smallIconStdSize = SmallIcon, IconStdSize largeIconStdSize = LargeIcon );

	enum StdMetrics { IconViewEdgeX = 4 };

	bool SetCompactIconSpacing( int iconEdgeWidth = IconViewEdgeX );		// to compact items width in icon views

	int GetTopIndex( void ) const;
	void SetTopIndex( int topIndex );

	enum MyHitTest { LVHT_MY_PASTEND = 0x00080000 };

	int HitTest( CPoint pos, UINT* pFlags = NULL ) const;
	int GetDropIndexAtPoint( const CPoint& point ) const;
	bool IsItemFullyVisible( int index ) const;
	bool GetIconItemRect( CRect* pIconRect, int index ) const;				// returns true if item visible

	static bool IsSelectionChangedNotify( const NMLISTVIEW* pNmList, UINT selMask = LVIS_SELECTED | LVIS_FOCUSED );
public:
	TRowKey MakeRowKeyAt( int index ) const;			// favour item data (LPARAM), or fall back to index (int)

	// sorting
	std::pair< TColumn, bool > GetSortByColumn( void ) const { return std::make_pair( m_sortByColumn, m_sortAscending ); }
	void SetSortByColumn( TColumn sortByColumn, bool sortAscending = true );

	bool GetSortInternally( void ) const { return HasFlag( m_optionFlags, SortInternally ); }
	void SetSortInternally( bool sortInternally = true ) { SetFlag( m_optionFlags, SortInternally, sortInternally ); }

	PFNLVCOMPARE GetCompareFunc( void ) const { return m_pComparePtrFunc; }
	void SetCompareFunc( PFNLVCOMPARE pComparePtrFunc ) { m_pComparePtrFunc = pComparePtrFunc; }

	bool IsSortingEnabled( void ) const { return !HasFlag( GetStyle(), LVS_NOSORTHEADER ); }
	bool SortList( void );
	void InitialSortList( void );

	void AddColumnCompare( TColumn column, const pred::IComparator* pComparator, bool defaultAscending = true );
	void AddRecordCompare( const pred::IComparator* pComparator ) { AddColumnCompare( EntireRecord, pComparator ); }	// default record comparator
	const pred::IComparator* FindCompare( TColumn column ) const;
protected:
	virtual pred::CompareResult CompareSubItems( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const;

	virtual bool TrackContextMenu( ListPopup popupType, const CPoint& screenPos );
private:
	void UpdateColumnSortHeader( void );
	bool IsDefaultAscending( TColumn column ) const;				// e.g. allows date-time descending ordering by default

	static pred::CompareResult CALLBACK ObjectCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pThis );

	static pred::CompareResult CALLBACK InitialOrderCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pThis );
	pred::CompareResult CompareInitialOrder( TRowKey leftKey, TRowKey rightKey ) const;

	static pred::CompareResult CALLBACK InitialGroupOrderCompareProc( int leftGroupId, int rightGroupId, CReportListControl* pThis );

	static int CALLBACK TextCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pThis );
	pred::CompareResult CompareSubItemsByIndex( UINT leftIndex, UINT rightIndex ) const;
	pred::CompareResult CompareSubItemsText( UINT leftIndex, UINT rightIndex, TColumn byColumn ) const;

	struct CColumnComparator 							// compare sub-item values per columns using object accessors
	{
		CColumnComparator( void ) {}
		CColumnComparator( TColumn column, bool defaultAscending, const pred::IComparator* pComparator ) : m_column( column ), m_defaultAscending( defaultAscending ), m_pComparator( pComparator ) {}
	public:
		TColumn m_column;
		bool m_defaultAscending;						// allows descending ordering by default (e.g. for date-time)
		const pred::IComparator* m_pComparator;
	};
public:
	enum ColumnWidth
	{
		ColumnWidth_AutoSize = 0,
		ColumnWidth_StretchToFit = -1,
		HalfColumnSpacing = 6,
		ColumnSpacing = HalfColumnSpacing * 2,
		MinColumnWidth = ColumnSpacing
	};

	enum CustomDrawTextMetrics { ItemSpacingX = 2, SubItemSpacingX = HalfColumnSpacing };

	// column layout
	struct CColumnInfo
	{
		CColumnInfo( void ) : m_alignment( LVCFMT_LEFT ), m_defaultWidth( 0 ), m_minWidth( MinColumnWidth ) {}
	public:
		std::tstring m_label;
		int m_alignment;
		int m_defaultWidth;
		int m_minWidth;
	};

	static void ParseColumnLayout( std::vector< CColumnInfo >& rColumnInfos, const std::vector< std::tstring >& columnSpecs );

	bool HasLayoutInfo( void ) const { return !m_columnInfos.empty(); }
	void SetLayoutInfo( UINT columnLayoutId );
	void SetLayoutInfo( const std::vector< std::tstring >& columnSpecs );

	CReportListControl& AddTileColumn( UINT tileColumn );

	unsigned int GetColumnCount( void ) const;
	void ResetColumnLayout( void );

	int GetMinColumnWidth( TColumn column ) const { ASSERT( HasLayoutInfo() ); return m_columnInfos[ column ].m_minWidth; }
	void SetMinColumnWidth( TColumn column, int minWidth ) { ASSERT( HasLayoutInfo() ); m_columnInfos[ column ].m_minWidth = minWidth; }
private:
	void InsertAllColumns( void );
	void DeleteAllColumns( void );

	void SetupColumnLayout( const std::vector< std::tstring >* pRegColumnLayoutItems );
	void PostColumnLayout( void );
	void InputColumnLayout( std::vector< std::tstring >& rRegColumnLayoutItems );
protected:
	void ClearData( void );

	virtual void SetupControl( void );
	virtual bool CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags, const CListSelectionData& selData ) const;

	bool ResizeFlexColumns( void );
	void NotifyParent( lv::Notification notifCode );

	// base overrides
	virtual void OnFinalReleaseInternalChange( void );
public:
	// items and sub-items
	enum { No_Image = -1, Transparent_Image = -2 };

	template< typename Type >
	static Type* AsPtr( LPARAM data ) { return reinterpret_cast< Type* >( data ); }

	template< typename Type >
	Type* GetPtrAt( int index ) const;

	bool SetPtrAt( int index, const void* pData ) { return SetItemData( index, (DWORD_PTR)pData ) != FALSE; }

	utl::ISubject* GetObjectAt( int index ) const;
	static inline utl::ISubject* ToSubject( LPARAM data ) { return checked_static_cast< utl::ISubject* >( (utl::ISubject*)data ); }

	bool DeleteAllItems( void );

	virtual int InsertObjectItem( int index, const utl::ISubject* pObject, int imageIndex = No_Image, const TCHAR* pText = NULL );		// pText could be LPSTR_TEXTCALLBACK

	void SetSubItemTextPtr( int index, int subItem, const TCHAR* pText = LPSTR_TEXTCALLBACK, int imageIndex = No_Image );
	void SetSubItemText( int index, int subItem, const std::tstring& text, int imageIndex = No_Image ) { SetSubItemTextPtr( index, subItem, text.c_str(), imageIndex ); }
	void SetSubItemImage( int index, int subItem, int imageIndex );

	bool SetItemTileInfo( int index );

	static void StoreDispInfoItemText( NMLVDISPINFO* pDispInfo, const std::tstring& text );		// rarely used, normally client owns text buffers

	void SwapItems( int index1, int index2 );
	void DropMoveItems( int destIndex, const std::vector< int >& selIndexes );
	bool ForceRearrangeItems( void );				// for icon view modes

	int Find( const void* pObject ) const;
	int FindItemIndex( const std::tstring& itemText ) const;
	int FindItemIndex( LPARAM lParam ) const;
	int FindItemIndex( const utl::ISubject* pObject ) const { return FindItemIndex( (LPARAM)pObject ); }

	bool EnsureVisibleObject( const utl::ISubject* pObject );

	void QueryItemsText( std::vector< std::tstring >& rItemsText, const std::vector< int >& indexes, int subItem = 0 ) const;
	void QuerySelectedItemsText( std::vector< std::tstring >& rItemsText, int subItem = 0 ) const;
	void QueryAllItemsText( std::vector< std::tstring >& rItemsText, int subItem = 0 ) const;

	// item state
	bool HasItemState( int index, UINT stateMask ) const { return ( GetItemState( index, stateMask ) & stateMask ) != 0; }
	bool HasItemStrictState( int index, UINT stateMask ) const { return stateMask == ( GetItemState( index, stateMask ) & stateMask ); }
	int FindItemWithState( UINT stateMask ) const { return GetNextItem( -1, LVNI_ALL | stateMask ); }
	bool HasItemWithState( UINT stateMask ) const { return GetNextItem( -1, LVNI_ALL | stateMask ) != -1; }

	bool IsChecked( int index ) const { return GetCheck( index ) != FALSE; }
	bool SetChecked( int index, bool check = true ) { return SetCheck( index, check ) != FALSE; }

	template< typename Type >
	bool QueryCheckedItems( std::vector< Type* >& rCheckedItems ) const { return QueryCheckedStateItems( rCheckedItems, lv::LVIS_CHECKED ); }

	template< typename Type >
	void SetCheckedItems( const std::vector< Type* >& items, bool check = true, bool uncheckOthers = true ) { SetCheckedStateItems( &items, check ? lv::LVIS_CHECKED : lv::LVIS_UNCHECKED, uncheckOthers ); }

	template< typename Type >
	void SetCheckedAll( bool check = true ) { SetCheckedStateItems( NULL, check ? lv::LVIS_CHECKED : lv::LVIS_UNCHECKED, false ); }

	// extended check state (includes radio check states)
	lv::CheckState GetCheckState( int index ) const { return static_cast< lv::CheckState >( GetItemState( index, LVIS_STATEIMAGEMASK ) ); }
	bool SetCheckState( int index, lv::CheckState checkState ) { return SetItemState( index, checkState, LVIS_STATEIMAGEMASK ) != FALSE; }
	bool ModifyCheckState( int index, lv::CheckState checkState );

	bool GetUseExtendedCheckStates( void ) const;
	bool SetupExtendedCheckStates( void );
	CImageList* GetStateImageList( void ) const { return GetImageList( LVSIL_STATE ); }

	bool GetToggleCheckSelected( void ) const { return HasFlag( m_optionFlags, ToggleCheckSelected ); }
	void SetToggleCheckSelected( bool toggleCheckSelected = true ) { SetOptionFlag( ToggleCheckSelected, toggleCheckSelected ); }

	bool GetUseTriStateAutoCheck( void ) const { return HasFlag( m_optionFlags, UseTriStateAutoCheck ); }
	void SetUseTriStateAutoCheck( bool useTriStateAutoCheck = true ) { SetOptionFlag( UseTriStateAutoCheck, useTriStateAutoCheck ); }

	static bool StateChanged( UINT newState, UINT oldState, UINT stateMask ) { return ( newState & stateMask ) != ( oldState & stateMask ); }
	static bool HasCheckState( UINT state ) { return ( state & LVIS_STATEIMAGEMASK ) != 0; }

	static lv::CheckState AsCheckState( UINT state ) { return static_cast< lv::CheckState >( state & LVIS_STATEIMAGEMASK ); }
	static bool IsCheckedState( UINT state );

	template< typename Type >
	bool QueryCheckedStateItems( std::vector< Type* >& rCheckedItems, lv::CheckState checkState = lv::LVIS_CHECKED ) const;

	template< typename Type >
	void SetCheckedStateItems( const std::vector< Type* >* pItems, lv::CheckState checkState = lv::LVIS_CHECKED, bool uncheckOthers = true );

	// caret and selection
	int GetCaretIndex( void ) const { return FindItemWithState( LVNI_FOCUSED ); }
	bool SetCaretIndex( int index, bool doSet = true );

	int GetCurSel( void ) const;
	bool SetCurSel( int index, bool doSelect = true );		// caret and selection

	bool AnySelected( UINT stateMask = LVIS_SELECTED ) const { return HasItemWithState( stateMask ); }

	bool IsSelected( int index ) const { return HasItemState( index, LVIS_SELECTED ); }
	bool SetSelected( int index, bool doSelect = true ) { return SetItemState( index, doSelect ? LVIS_SELECTED : 0, LVIS_SELECTED ) != FALSE; }

	// multiple selection
	bool GetSelection( std::vector< int >& rSelIndexes, int* pCaretIndex = NULL, int* pTopIndex = NULL ) const;
	void SetSelection( const std::vector< int >& selIndexes, int caretIndex = -1 );
	void ClearSelection( void );
	void SelectAll( void );

	void Select( const void* pObject );

	template< typename Type >
	bool QuerySelectionAs( std::vector< Type* >& rSelPtrs ) const;

	template< typename Type >
	void SelectItems( const std::vector< Type* >& rPtrs );

	// resequence
	void MoveSelectionTo( seq::MoveTo moveTo );
public:
	// clipboard and drag-n-drop data
	enum { ListSourcesMask = ds::Indexes | ds::ItemsText | ds::ShellFiles };

	bool CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags = ListSourcesMask ) const;		// for drag-drop or clipboard transfers
	bool Copy( int sourceFlags = ListSourcesMask );						// creates a new COleDataSource object owned by the cliboard

	std::auto_ptr< CImageList > CreateDragImageMulti( const std::vector< int >& indexes, CPoint* pFrameOrigin = NULL );
	std::auto_ptr< CImageList > CreateDragImageSelection( CPoint* pFrameOrigin = NULL );

	CRect GetFrameBounds( const std::vector< int >& indexes ) const;
public:
	// custom cell marking: color, bold, italic, underline
	void MarkCellAt( int index, TColumn subItem, const ui::CTextEffect& textEfect );
	void MarkRowAt( int index, const ui::CTextEffect& textEfect ) { MarkCellAt( index, EntireRecord, textEfect ); }
	void UnmarkCellAt( int index, TColumn subItem );
	void ClearMarkedCells( void );

	const ui::CTextEffect* FindTextEffectAt( TRowKey rowKey, TColumn subItem ) const;

	interface ITextEffectCallback
	{
		virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const = 0;
		virtual void ModifyDiffTextEffectAt( CListTraits::CMatchEffects& rEffects, LPARAM rowKey, int subItem ) const { rEffects, rowKey, subItem; }
	};

	void SetTextEffectCallback( ITextEffectCallback* pTextEffectCallback ) { m_pTextEffectCallback = pTextEffectCallback; }

	// diff columns
	template< typename MatchFunc >
	void SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc );		// call after the list items are set up; by default pass str::GetMatch()
protected:
	const CDiffColumnPair* FindDiffColumnPair( TColumn column ) const;
	bool IsDiffColumn( TColumn column ) const { return FindDiffColumnPair( column ) != NULL; }

	ui::CFontEffectCache* GetFontEffectCache( void );

	enum ParentNotif { PN_DispInfo, PN_CustomDraw, _PN_Count };

	bool ParentHandles( ParentNotif notif );
public:
	enum SortOrder { NotSorted, Ascending, Descending };				// column sort image index

	struct CItemData
	{
		CItemData( void ) : m_state( 0 ), m_lParam( 0 ) {}
	public:
		std::vector< std::pair< std::tstring, int > > m_subItems;		// text + imageIndex
		UINT m_state;
		LPARAM m_lParam;
	};

	void GetItemDataAt( CItemData& rItemData, int index ) const;
	void SetItemDataAt( const CItemData& itemData, int index );
	void InsertItemDataAt( const CItemData& itemData, int index );

	struct CLabelEdit
	{
		CLabelEdit( int index, const std::tstring& oldLabel ) : m_index( index ), m_done( false ), m_oldLabel( oldLabel ) {}
	public:
		int m_index;
		bool m_done;
		std::tstring m_oldLabel, m_newLabel;
	};

	const CLabelEdit* EditLabelModal( int index );
	const CLabelEdit* GetLabelEdit( void ) const { return m_pLabelEdit.get(); }
public:
	// groups: rows not assigned to a group will not show in group-view

#if ( _WIN32_WINNT >= 0x0600 ) && defined( UNICODE )	// Vista+

	// groupId: typically an index in the vector of group objects maintained by the client.
	// groupIndex can change with sorting, whereas groupId is invariant to that
	//
	int GetGroupId( int groupIndex ) const;
	std::pair< int, UINT > GetGroupItemsRange( int groupId ) const;		// < firstItemIndex, itemCount >

	int GetRowGroupId( int rowIndex ) const;
	bool SetRowGroupId( int rowIndex, int groupId );

	std::tstring GetGroupHeaderText( int groupId ) const;
	int InsertGroupHeader( int index, int groupId, const std::tstring& header, DWORD state = LVGS_NORMAL, DWORD align = LVGA_HEADER_LEFT );
	bool SetGroupFooter( int groupId, const std::tstring& footer, DWORD align = LVGA_FOOTER_CENTER );
	bool SetGroupTask( int groupId, const std::tstring& task );
	bool SetGroupSubtitle( int groupId, const std::tstring& subtitle );
	bool SetGroupTitleImage( int groupId, int image, const std::tstring& topDescr, const std::tstring& bottomDescr );

	void DeleteEntireGroup( int groupId );
	bool HasGroupState( int groupId, DWORD stateMask ) const { return GetGroupState( groupId, stateMask ) == stateMask; }

	void CheckEntireGroup( int groupId, bool check = true );

	void CollapseAllGroups( void );
	void ExpandAllGroups( void );

	int GroupHitTest( const CPoint& point ) const;
#endif // Vista groups

private:
	bool UpdateCustomImagerBoundsSize( void );
private:
	enum ListOption
	{
		UseExplorerTheme			= BIT_FLAG( 0 ),
		UseAlternateRowColoring		= BIT_FLAG( 1 ),
		SortInternally				= BIT_FLAG( 2 ),
		AcceptDropFiles				= BIT_FLAG( 3 ),		// enable as Explorer drop target, send LVN_DropFiles notification when files are dropped onto the list
		HighlightTextDiffsFrame		= BIT_FLAG( 4 ),		// highlight text differences with a filled frame
		UseTriStateAutoCheck		= BIT_FLAG( 5 ),		// extended check state: allows toggling lv::LVIS_UNCHECKED -> lv::LVIS_CHECKED -> lv::LVIS_CHECKEDGRAY
		ToggleCheckSelected			= BIT_FLAG( 5 )			// multi-selection: toggle checked state for the selected items
	};

	bool SetOptionFlag( ListOption flag, bool on );
private:
	UINT m_columnLayoutId;
	DWORD m_listStyleEx;
	std::tstring m_regSection;
	std::vector< CColumnInfo > m_columnInfos;
	std::vector< UINT > m_tileColumns;						// columns to be displayed as tile additional text (in gray)
	int m_optionFlags;
	bool m_subjectBased;									// objects stored as pointers are derived from utl::ISubject (polymorphic type)

	TColumn m_sortByColumn;
	bool m_sortAscending;
	PFNLVCOMPARE m_pComparePtrFunc;							// compare by object ptr such as: static CompareResult CALLBACK CompareObjects( const CFoo* pLeft, const CFoo* pRight, CReportListControl* pThis );

	std::vector< CColumnComparator > m_comparators;
	utl::vector_map< TRowKey, int > m_initialItemsOrder;	// stores items initial order just after list set-up

	typedef std::pair< TRowKey, TColumn > TCellPair;		// invariant to sorting: favour LPARAMs instead of indexes
	stdext::hash_map< TCellPair, ui::CTextEffect > m_markedCells;
	std::auto_ptr< ui::CFontEffectCache > m_pFontCache;		// self-encapsulated
	ITextEffectCallback* m_pTextEffectCallback;

	std::list< CDiffColumnPair > m_diffColumnPairs;

	CImageList* m_pImageList;
	CImageList* m_pLargeImageList;
	std::auto_ptr< CBaseCustomDrawImager > m_pCustomImager;

	CMenu* m_pPopupMenu[ _ListPopupCount ];					// used when right clicking nowhere - on header or no list item
	std::auto_ptr< CLabelEdit > m_pLabelEdit;				// stores the label info during inline editing

	CAccelTable m_listAccel;
	ole::IDataSourceFactory* m_pDataSourceFactory;			// creates ole::CDataSource for clipboard and drag-drop
private:
	bool m_painting;										// true during OnPaint() - supresses item text callback for diff columns to prevent default list sub-item draw (diffs are custom drawn)
	BOOL m_parentHandles[ _PN_Count ];						// self-encapsulated 'parent handles' flags array
protected:
	CWnd* m_pTrackMenuTarget;								// window that receives commands when tracking the context menu
public:
	ui::CTextEffect m_listTextEffect;						// for all items in the list

	ui::CTextEffect m_deleteSrc_DiffEffect;					// text diffs: text removed from SRC (red)
	ui::CTextEffect m_mismatchDest_DiffEffect;				// text diffs: text mismatched in DEST (blue)
	ui::CTextEffect m_matchDest_DiffEffect;					// text diffs: text matched in DEST (gray)

	static const COLORREF s_deleteSrcTextColor = color::Red;
	static const COLORREF s_mismatchDestTextColor = color::Blue;
private:
	static const TCHAR s_fmtRegColumnLayout[];
public:
	// generated stuff
	public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnWindowPosChanged( WINDOWPOS* pWndPos );
	afx_msg void OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags );
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnPaint( void );
	virtual BOOL OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnHdnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnHdnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnColumnClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnBeginLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnLvnEndLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnLvnGetDispInfo_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
public:
	afx_msg void OnListViewMode( UINT cmdId );
	afx_msg void OnUpdateListViewMode( CCmdUI* pCmdUI );
	afx_msg void OnListViewStacking( UINT cmdId );
	afx_msg void OnUpdateListViewStacking( CCmdUI* pCmdUI );
	afx_msg void OnResetColumnLayout( void );
	afx_msg void OnUpdateResetColumnLayout( CCmdUI* pCmdUI );
	afx_msg void OnCopy( void );
	afx_msg void OnSelectAll( void );
	afx_msg void OnUpdateSelectAll( CCmdUI* pCmdUI );
	afx_msg void OnMoveTo( UINT cmdId );
	afx_msg void OnUpdateMoveTo( CCmdUI* pCmdUI );
	afx_msg void OnRename( void );
	afx_msg void OnUpdateRename( CCmdUI* pCmdUI );
	afx_msg void OnUpdateAnySelected( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


struct CListCtrlUiState
{
public:
	CListCtrlUiState( CReportListControl* pListCtrl )
		: m_pListCtrl( pListCtrl )
		, m_caretIndex( m_pListCtrl->GetCaretIndex() )
		, m_topVisibleIndex( m_pListCtrl->GetTopIndex() )
	{
	}

	void Restore( void )
	{
		if ( m_pListCtrl->GetItemCount() != 0 )
		{
			if ( m_caretIndex != -1 )
				m_pListCtrl->SetCaretIndex( m_caretIndex );

			if ( m_topVisibleIndex != LB_ERR )
				m_pListCtrl->EnsureVisible( m_topVisibleIndex, FALSE );
		}
	}
public:
	CReportListControl* m_pListCtrl;
	int m_caretIndex;
	int m_topVisibleIndex;
};


struct CScopedListTextSelection : private utl::noncopyable
{
	CScopedListTextSelection( CReportListControl* pListCtrl );
	~CScopedListTextSelection();
public:
	CReportListControl* m_pListCtrl;
	std::tstring m_caretText;
	std::vector< std::tstring > m_selTexts;
};


// adapter for swapping items in a list ctrl
//
class CListCtrlSequence
{
public:
	CListCtrlSequence( CReportListControl* pListCtrl ) : m_pListCtrl( pListCtrl ), m_itemCount( m_pListCtrl->GetItemCount() ) { ASSERT_PTR( m_pListCtrl ); }

	void Swap( UINT leftIndex, UINT rightIndex )
	{
		ASSERT( leftIndex < m_itemCount && rightIndex < m_itemCount );
		m_pListCtrl->SwapItems( leftIndex, rightIndex );
	}

	UINT GetSize( void ) const { return m_itemCount; }
private:
	CReportListControl* m_pListCtrl;
	UINT m_itemCount;
};


// icon view mode: navigates in a linear flow of selection/caret on arrow keys (by default the selection flow is limited on rows/columns)
//
class CSelFlowSequence
{
	CSelFlowSequence( CReportListControl* pListCtrl );
public:
	static std::auto_ptr< CSelFlowSequence > MakeFlow( CReportListControl* pListCtrl );

	bool HandleKeyDown( UINT chr );
	bool PostKeyDown( UINT chr );
private:
	bool NavigateCaret( int newCaretIndex );
private:
	enum ItemSequence { TopToBottom, LeftToRight };

	CReportListControl* m_pListCtrl;
	int m_itemCount;
	int m_caretIndex;
	int m_topIndex;
	int m_colsPerPage;
	int m_rowsPerPage;
	ItemSequence m_seqDirection;
};


// stores selected indexes for drag & drop/clipboard; works on any list-like source window

class CListSelectionData : public ole::CTransferBlob
{
public:
	CListSelectionData( CWnd* pSrcWnd = NULL );
	CListSelectionData( CReportListControl* pSrcListCtrl );

	bool IsValid( void ) const { return m_pSrcWnd != NULL && !m_selIndexes.empty(); }
	static CLIPFORMAT GetClipFormat( void );

	// serial::IStreamable interface
	virtual void Save( CArchive& archive ) throws_( CException* );
	virtual void Load( CArchive& archive ) throws_( CException* );
public:
	CWnd* m_pSrcWnd;
	std::vector< int > m_selIndexes;
};


// CReportListControl inline code

inline bool CReportListControl::CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags /*= ListSourcesMask*/ ) const
{
	CListSelectionData selData( const_cast< CReportListControl* >( this ) );
	return CacheSelectionData( pDataSource, sourceFlags, selData );
}

template< typename Type >
inline Type* CReportListControl::GetPtrAt( int index ) const
{
	ASSERT( index >= 0 && index < GetItemCount() );
	return AsPtr< Type >( GetItemData( index ) );
}

template< typename Type >
bool CReportListControl::QueryCheckedStateItems( std::vector< Type* >& rCheckedItems, lv::CheckState checkState /*= lv::LVIS_CHECKED*/ ) const
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( GetCheckState( i ) == checkState )
			rCheckedItems.push_back( GetPtrAt< Type >( i ) );

	return !rCheckedItems.empty();
}

template< typename Type >
void CReportListControl::SetCheckedStateItems( const std::vector< Type* >* pItems, lv::CheckState checkState /*= lv::LVIS_CHECKED*/, bool uncheckOthers /*= true*/ )
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( NULL == pItems || utl::Contains( *pItems, GetPtrAt< Type >( i ) ) )
			ModifyCheckState( i, checkState );
		else if ( uncheckOthers )
			ModifyCheckState( i, lv::LVIS_UNCHECKED );
}


template< typename Type >
bool CReportListControl::QuerySelectionAs( std::vector< Type* >& rSelPtrs ) const
{
	std::vector< int > selIndexes;
	if ( !GetSelection( selIndexes ) )
		return false;

	rSelPtrs.clear();
	rSelPtrs.reserve( selIndexes.size() );

	size_t selCount = 0;

	for ( std::vector< int >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex, ++selCount )
		if ( Type* pSelObject = GetPtrAt< Type >( *itSelIndex ) )
			rSelPtrs.push_back( pSelObject );

	return !rSelPtrs.empty() && rSelPtrs.size() == selCount;		// homogenous selection?
}

template< typename Type >
void CReportListControl::SelectItems( const std::vector< Type* >& rPtrs )
{
	std::vector< int > selIndexes;
	for ( std::vector< Type* >::const_iterator itObject = rPtrs.begin(); itObject != rPtrs.end(); ++itObject )
	{
		int foundIndex = Find( *itObject );
		if ( foundIndex != -1 )
			selIndexes.push_back( foundIndex );
	}

	if ( !selIndexes.empty() )
		SetSelection( selIndexes, selIndexes.front() );
	else
		ClearSelection();
}

template< typename MatchFunc >
void CReportListControl::SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc )
{
	m_diffColumnPairs.push_back( CDiffColumnPair( srcColumn, destColumn ) );

	stdext::hash_map< TRowKey, str::TMatchSequence >& rRowSequences = m_diffColumnPairs.back().m_rowSequences;

	for ( int index = 0, itemCount = GetItemCount(); index != itemCount; ++index )
	{
		str::TMatchSequence& rMatchSequence = rRowSequences[ MakeRowKeyAt( index ) ];

		rMatchSequence.Init( GetItemText( index, srcColumn ).GetString(), GetItemText( index, destColumn ).GetString(), getMatchFunc );

		// Replace with LPSTR_TEXTCALLBACK to allow custom draw without default sub-item draw interference (on CDDS_ITEMPOSTPAINT | CDDS_SUBITEM).
		// Sub-item text is still accessible with GetItemText() method.
		//
		VERIFY( SetItemText( index, srcColumn, LPSTR_TEXTCALLBACK ) );
		VERIFY( SetItemText( index, destColumn, LPSTR_TEXTCALLBACK ) );
	}
}


#endif // ReportListControl_h
