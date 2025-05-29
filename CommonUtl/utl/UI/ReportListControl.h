#ifndef ReportListControl_h
#define ReportListControl_h
#pragma once

#include "utl/StdHashValue.h"
#include "utl/Range.h"
#include "BaseTrackMenuWnd.h"
#include "CtrlInterfaces.h"
#include "ListLikeCtrlBase.h"
#include "Path.h"
#include "SubjectPredicates.h"
#include "OleUtils.h"
#include "MatchSequence.h"
#include "Resequence.h"
#include "ui_fwd.h"				// for ui::CNmHdr
#include "vector_map.h"
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <afxcmn.h>


#if ( _WIN32_WINNT >= 0x0600 ) && defined( UNICODE )	// Vista+
	#define UTL_VISTA_GROUPS
#endif


class CListSelectionData;
class CReportListCustomDraw;
namespace ole { class CDataSource; }
namespace ui { interface ICheckStatePolicy; }


namespace lv
{
	enum SubPopup
	{
		NowhereSubPopup, OnSelectionSubPopup, OnGroupSubPopup,
		PathItemNowhereSubPopup, PathItemOnSelectionSubPopup
	};

	enum
	{
		DefaultStyleEx = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
			LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_AUTOAUTOARRANGE /*| LVS_EX_JUSTIFYCOLUMNS*/

		// note: LVS_EX_DOUBLEBUFFER causes problems of scaling artifacts with CustomImageDraw; remove this styleEx for accurate scaling.
	};


	enum NotifyCode
	{
		// via WM_COMMAND:
		LVN_ItemsReorder = 1000,
			_LastCmd,						// derived classes may define new WM_COMMAND notifications starting from this value

		// via WM_NOTIFY:
		LVN_CanSortByColumn = 1100,			// pass NMHDR; clients return TRUE if sorting is disabled for the clicked column (by default sorting is enabled on all columns)
		LVN_CustomSortList,					// pass NMHDR; clients return TRUE if handled custom sorting, using current sorting criteria GetSortByColumn()
		LVN_ListSorted,						// pass NMHDR
		LVN_ToggleCheckState,				// pass lv::CNmToggleCheckState; client could return TRUE to reject default toggle
		LVN_CheckStatesChanged,				// pass lv::CNmCheckStatesChanged
		LVN_DropFiles,						// pass lv::CNmDropFiles
		LVN_ItemsRemoved,					// pass lv::CNmItemsRemoved
			_LastNotify						// derived classes may define new WM_NOTIFY notifications starting from this value
	};


	struct CNmDropFiles
	{
		CNmDropFiles( const CListCtrl* pListCtrl, const CPoint& dropPoint, int dropItemIndex )
			: m_nmHdr( pListCtrl, lv::LVN_DropFiles ), m_dropPoint( dropPoint ), m_dropItemIndex( dropItemIndex ) {}
	public:
		ui::CNmHdr m_nmHdr;
		CPoint m_dropPoint;							// client coordinates
		int m_dropItemIndex;
		std::vector<fs::CPath> m_filePaths;		// sorted
	};


	struct CNmItemsRemoved
	{
		CNmItemsRemoved( const CListCtrl* pListCtrl, int minSelIndex = -1 )
			: m_nmHdr( pListCtrl, lv::LVN_ItemsRemoved ), m_minSelIndex( minSelIndex ) {}
	public:
		ui::CNmHdr m_nmHdr;
		int m_minSelIndex;
		std::vector<utl::ISubject*> m_removedObjects;
	};


	struct CNmToggleCheckState
	{
		CNmToggleCheckState( const CListCtrl* pListCtrl, NMLISTVIEW* pListView, const ui::ICheckStatePolicy* pCheckStatePolicy );
	public:
		ui::CNmHdr m_nmHdr;
		NMLISTVIEW* m_pListView;
		const int m_oldCheckState;
		int m_newCheckState;			// in/out
	};


	struct CNmCheckStatesChanged
	{
		CNmCheckStatesChanged( const CListCtrl* pListCtrl ) : m_nmHdr( pListCtrl, lv::LVN_CheckStatesChanged ) {}

		bool AddIndex( int itemIndex );
	public:
		ui::CNmHdr m_nmHdr;
		std::vector<int> m_itemIndexes;		// all items impacted by the toggle; first index is the toggled reference
	};
}


namespace lv
{
	struct CMatchEffects
	{
		CMatchEffects( std::vector<ui::CTextEffect>& rMatchEffects )
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


abstract class CListTraits			// defines types shared between CReportListControl and CReportListCustomDraw classes
{
public:
	typedef LPARAM TRowKey;			// row keys are invariant to sorting; they represent item data (LPARAM), or fall back to index (int)
	typedef int TColumn;
	typedef int TGroupId;

	enum StdColumn { Code, EntireRecord = (TColumn)-1 };


	struct CNmCanSortByColumn
	{
		CNmCanSortByColumn( const CListCtrl* pListCtrl, TColumn sortByColumn )
			: m_nmHdr( pListCtrl, lv::LVN_CanSortByColumn ), m_sortByColumn( sortByColumn ) {}
	public:
		ui::CNmHdr m_nmHdr;
		TColumn m_sortByColumn;
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
		std::unordered_map<TRowKey, str::TMatchSequence> m_rowSequences;		// TRowKey is invariant to sorting
	};
};


class CReportListControl
	: public CBaseTrackMenuWnd<CListCtrl>
	, public CListLikeCtrlBase
	, public CListTraits
{
	friend class CReportListCustomDraw;
public:
	CReportListControl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CReportListControl();

	DWORD& RefListStyleEx( void ) { return m_listStyleEx; }
	bool ModifyListStyleEx( DWORD dwRemove, DWORD dwAdd, UINT swpFlags = 0 );

	void SaveToRegistry( void );
	bool LoadFromRegistry( void );

	void StoreImageLists( CImageList* pImageList, CImageList* pLargeImageList = nullptr );

	void ChangeListViewMode( DWORD viewMode );

	bool IsMultiSelectionList( void ) const { return !HasFlag( GetStyle(), LVS_SINGLESEL ); }

	bool GetUseAlternateRowColoring( void ) const { return HasFlag( m_optionFlags, UseAlternateRowColoring ); }
	void SetUseAlternateRowColoring( bool useAlternateRowColoring = true ) { SetOptionFlag( UseAlternateRowColoring, useAlternateRowColoring ); }

	bool GetHighlightTextDiffsFrame( void ) const { return HasFlag( m_optionFlags, HighlightTextDiffsFrame ); }
	void SetHighlightTextDiffsFrame( bool highlightTextDiffsFrame = true ) { SetOptionFlag( HighlightTextDiffsFrame, highlightTextDiffsFrame ); }

	bool GetAcceptDropFiles( void ) const { return HasFlag( m_optionFlags, AcceptDropFiles ); }
	void SetAcceptDropFiles( bool acceptDropFiles = true ) { SetOptionFlag( AcceptDropFiles, acceptDropFiles ); }

	bool GetUseExternalImages( void ) const { return HasFlag( m_optionFlags, UseExternalImages ); }
	void SetUseExternalImages( bool useSmallImages = true, bool useLargeImages = true ) { SetOptionFlag( UseExternalImagesSmall, useSmallImages ); SetOptionFlag( UseExternalImagesLarge, useLargeImages ); }

	bool IsCommandFrame( void ) const { return HasFlag( m_optionFlags, CommandFrame ); }
	void SetCommandFrame( bool isCommandFrame = true ) { SetTrackMenuTarget( this ); SetOptionFlag( CommandFrame, isCommandFrame ); }
	void SetFrameEditor( ui::ICommandFrame* pFrameEditor ) { m_pFrameEditor = pFrameEditor; SetCommandFrame( m_pFrameEditor != nullptr ); }

	const std::tstring& GetSection( void ) const { return m_regSection; }
	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }

	const TCHAR* GetTabularTextSep( void ) const { return m_pTabularSep; }
	void SetTabularTextSep( const TCHAR* pTabularSep ) { m_pTabularSep = pTabularSep; }

	COLORREF GetActualTextColor( void ) const;
public:
	enum ListPopup { Nowhere, OnSelection, OnGroup, _ListPopupCount };

	void SetPopupMenu( ListPopup popupType, CMenu* pPopupMenu ) { m_pPopupMenu[ popupType ] = pPopupMenu; }		// set pPopupMenu to NULL to allow tracking context menu by parent dialog

	virtual CMenu* GetPopupMenu( ListPopup popupType );

	static CMenu& GetStdPopupMenu( ListPopup popupType );

	ole::IDataSourceFactory* GetDataSourceFactory( void ) const { return m_pDataSourceFactory; }
	void SetDataSourceFactory( ole::IDataSourceFactory* pDataSourceFactory ) { ASSERT_PTR( pDataSourceFactory ); m_pDataSourceFactory = pDataSourceFactory; }

	ui::GlyphGauge GetViewModeGlyphGauge( void ) const { return GetViewModeGlyphGauge( GetView() ); }
	static ui::GlyphGauge GetViewModeGlyphGauge( DWORD listViewMode );

	// ICustomDrawControl overrides
	virtual void SetCustomFileGlyphDraw( bool showGlyphs = true );

	// To prevent icon scaling dithering when displaying shell item icon thumbnails, specify small/large image bounds size from the image list.
	// Otherwise when displaying only image file thumbs, let the thumbnailer drive bounds sizes.
	// Remove LVS_EX_DOUBLEBUFFER for accurate image scaling, especially for transparent PNGs.
	//
	void SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, const CSize& smallImageSize = CSize( 0, 0 ), const CSize& largeImageSize = CSize( 0, 0 ) );
	void SetCustomIconDraw( ui::ICustomImageDraw* pCustomIconDraw, IconStdSize smallIconStdSize = SmallIcon, IconStdSize largeIconStdSize = LargeIcon );

	enum StdMetrics { IconViewEdgeX = 4 };

	bool SetCompactIconSpacing( int iconEdgeWidth = IconViewEdgeX );		// to compact items width in icon views

	bool IsValidIndex( int index ) const { return index >= 0 && index < GetItemCount(); }

	int GetTopIndex( void ) const;
	void SetTopIndex( int topIndex );

	enum MyHitTest { LVHT_MY_PASTEND = 0x00080000 };

	int HitTest( CPoint point, UINT* pFlags = nullptr, TGroupId* pGroupId = nullptr ) const;
	TGroupId GroupHitTest( const CPoint& point, int groupType = LVGGR_HEADER ) const;

	int GetDropIndexAtPoint( const CPoint& point ) const;
	bool IsItemFullyVisible( int index ) const;
	bool GetIconItemRect( CRect* pIconRect, int index ) const;				// returns true if item visible
public:
	TRowKey MakeRowKeyAt( int index ) const			// favour item data (LPARAM), or fall back to index (int)
	{
		TRowKey rowKey = static_cast<TRowKey>( GetItemData( index ) );
		return rowKey != 0 ? rowKey : static_cast<TRowKey>( index );
	}

	// sorting
	std::pair<TColumn, bool> GetSortByColumn( void ) const { return std::make_pair( m_sortByColumn, m_sortAscending ); }
	void SetSortByColumn( TColumn sortByColumn, bool sortAscending = true );		// do sort the list
	void StoreSortByColumn( TColumn sortByColumn, bool sortAscending = true );		// update the list column header sort order

	bool GetSortInternally( void ) const { return HasFlag( m_optionFlags, SortInternally ); }
	void SetSortInternally( bool sortInternally = true ) { SetFlag( m_optionFlags, SortInternally, sortInternally ); }

	bool GetPersistSorting( void ) const { return HasFlag( m_optionFlags, PersistSorting ); }
	void SetPersistSorting( bool persistSorting = true ) { SetFlag( m_optionFlags, PersistSorting, persistSorting ); }

	PFNLVCOMPARE GetCompareFunc( void ) const { return m_pComparePtrFunc; }
	void SetCompareFunc( PFNLVCOMPARE pComparePtrFunc ) { m_pComparePtrFunc = pComparePtrFunc; }

	bool IsSortingEnabled( void ) const { return !HasFlag( GetStyle(), LVS_NOSORTHEADER ); }
	void SortList( void );
	void InitialSortList( void );

	void AddColumnCompare( TColumn column, const pred::IComparator* pComparator, bool defaultAscending = true );
	void AddRecordCompare( const pred::IComparator* pComparator ) { AddColumnCompare( EntireRecord, pComparator ); }	// default record comparator
	void ReleaseSharedComparators( void ) { m_comparators.clear(); }		// call before destruction to prevent deleting shared comparators (externally managed)

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
		CColumnComparator( void ) : m_column( -1 ), m_defaultAscending( false ), m_pComparator( nullptr ) {}
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

	static void ParseColumnLayout( std::vector<CColumnInfo>& rColumnInfos, const std::vector<std::tstring>& columnSpecs );

	bool HasLayoutInfo( void ) const { return !m_columnInfos.empty(); }
	void SetLayoutInfo( UINT columnLayoutId );
	void SetLayoutInfo( const std::vector<std::tstring>& columnSpecs );

	CReportListControl& AddTileColumn( UINT tileColumn );

	unsigned int GetColumnCount( void ) const;
	void ResetColumnLayout( void );

	int GetMinColumnWidth( TColumn column ) const { ASSERT( HasLayoutInfo() ); return m_columnInfos[ column ].m_minWidth; }
	void SetMinColumnWidth( TColumn column, int minWidth ) { ASSERT( HasLayoutInfo() ); m_columnInfos[ column ].m_minWidth = minWidth; }
private:
	void InsertAllColumns( void );
	void DeleteAllColumns( void );

	void SetupColumnLayout( const std::vector<std::tstring>* pRegColumnLayoutItems );
	void PostColumnLayout( void );
	void InputColumnLayout( std::vector<std::tstring>& rRegColumnLayoutItems );
protected:
	void ClearData( void );

	virtual void SetupControl( void );
	virtual bool CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags, const CListSelectionData& selData ) const;
	virtual std::tstring FormatItemLine( int index ) const;

	bool ResizeFlexColumns( void );

	// base overrides
	virtual void OnFinalReleaseInternalChange( void );
public:
	bool IsObjectBased( void ) const { return m_subjectBased; }		// list content based on objects stored as utl::ISubject pointers?

	// items and sub-items
	template< typename Type >
	Type* GetPtrAt( int index ) const { ASSERT( IsValidIndex( index ) ); return AsPtr<Type>( GetItemData( index ) ); }

	bool SetPtrAt( int index, const void* pData ) { return SetItemData( index, (DWORD_PTR)pData ) != FALSE; }

	utl::ISubject* GetSubjectAt( int index ) const;

	template< typename ObjectT >
	ObjectT* GetObjectAt( int index ) const { return checked_static_cast<ObjectT*>( GetSubjectAt( index ) ); }

	template< typename ObjectT >
	void QueryObjectsSequence( std::vector<ObjectT*>& rObjects ) const;

	template< typename ObjectT >
	void QueryObjectsByIndex( std::vector<ObjectT*>& rObjects, const std::vector<int>& itemIndexes ) const;

	bool DeleteAllItems( void );
	void RemoveAllGroups( void );

	virtual int InsertObjectItem( int index, const utl::ISubject* pObject, int imageIndex = ui::No_Image, const TCHAR* pText = nullptr );		// pText could be LPSTR_TEXTCALLBACK

	void SetSubItemTextPtr( int index, int subItem, const TCHAR* pText = LPSTR_TEXTCALLBACK, int imageIndex = ui::No_Image );
	void SetSubItemText( int index, int subItem, const std::tstring& text, int imageIndex = ui::No_Image ) { SetSubItemTextPtr( index, subItem, text.c_str(), imageIndex ); }
	void SetSubItemImage( int index, int subItem, int imageIndex );

	bool SetItemTileInfo( int index );

	static void StoreDispInfoItemText( NMLVDISPINFO* pDispInfo, const std::tstring& text );		// rarely used, normally client owns text buffers

	void SwapItems( int index1, int index2 );
	void DropMoveItems( int destIndex, const std::vector<int>& selIndexes );
	bool ForceRearrangeItems( void );				// for icon view modes

	int FindItemIndex( const std::tstring& itemText ) const;
	int FindItemIndex( LPARAM lParam ) const;
	int FindItemIndex( const void* pObject ) const { return FindItemIndex( (LPARAM)pObject ); }

	bool EnsureVisibleObject( const utl::ISubject* pObject );

	void QueryItemsText( std::vector<std::tstring>& rItemsText, const std::vector<int>& indexes, int subItem = 0 ) const;
	void QuerySelectedItemsText( std::vector<std::tstring>& rItemsText, int subItem = 0 ) const;
	void QueryAllItemsText( std::vector<std::tstring>& rItemsText, int subItem = 0 ) const;

	// item state
	bool HasItemState( int index, UINT stateMask ) const { return ( GetItemState( index, stateMask ) & stateMask ) != 0; }
	bool HasItemStrictState( int index, UINT stateMask ) const { return stateMask == ( GetItemState( index, stateMask ) & stateMask ); }
	int FindItemWithState( UINT stateMask ) const { return GetNextItem( -1, LVNI_ALL | stateMask ); }
	bool HasItemWithState( UINT stateMask ) const { return GetNextItem( -1, LVNI_ALL | stateMask ) != -1; }

	static bool StateChanged( UINT newState, UINT oldState, UINT stateMask ) { return ( newState & stateMask ) != ( oldState & stateMask ); }
	static bool IsStateChangeNotify( const NMLISTVIEW* pNmListView, UINT selMask );

	static bool IsSelectionChangeNotify( const NMLISTVIEW* pNmListView, UINT selMask = LVIS_SELECTED | LVIS_FOCUSED ) { return IsStateChangeNotify( pNmListView, selMask ); }
	static bool IsCheckStateChangeNotify( const NMLISTVIEW* pNmListView ) { return IsStateChangeNotify( pNmListView, LVIS_STATEIMAGEMASK ); }

	// check-box
	bool GetToggleCheckSelItems( void ) const { return HasFlag( m_optionFlags, ToggleCheckSelItems ); }
	void SetToggleCheckSelItems( bool toggleCheckSelected = true ) { SetOptionFlag( ToggleCheckSelItems, toggleCheckSelected ); }

	bool IsChecked( int index ) const;
	bool SetChecked( int index, bool check = true ) { return SetCheck( index, check ) != FALSE; }

	template< typename ObjectT >
	bool QueryCheckedObjects( std::vector<ObjectT*>& rCheckedObjects ) const { return QueryObjectsWithCheckedState( rCheckedObjects, BST_CHECKED ); }

	template< typename ObjectT >
	void SetCheckedObjects( const std::vector<ObjectT*>& objects, bool check = true, bool uncheckOthers = true ) { SetObjectsCheckedState( &objects, check ? BST_CHECKED : BST_UNCHECKED, uncheckOthers ); }

	void SetCheckedAll( bool check = true ) { SetObjectsCheckedState( (const std::vector<utl::ISubject*>*)nullptr, check ? BST_CHECKED : BST_UNCHECKED, false ); }

	// Extended check-state (may include radio check-states).
	//	Internally, the list-ctrl toggles through the sequence of states. That behaviour is replaced with custom togging when using a m_pCheckStatePolicy.
	//	Clients can:
	//		handle LVN_ToggleCheckState and override that behaviour;
	//		handle LVN_CheckStatesChanged to correlate other items with the new toggled state.
	//
	const ui::ICheckStatePolicy* GetCheckStatePolicy( void ) const { return m_pCheckStatePolicy; }
	void SetCheckStatePolicy( const ui::ICheckStatePolicy* pCheckStatePolicy );

	CImageList* GetStateImageList( void ) const { return GetImageList( LVSIL_STATE ); }
	CSize GetStateIconSize( void ) const;

	int GetCheckState( int index ) const { return ui::CheckStateFromRaw( GetRawCheckState( index ) ); }
	bool SetCheckState( int index, int checkState );
	bool ModifyCheckState( int index, int checkState );

	int GetObjectCheckState( const utl::ISubject* pObject ) const;
	bool ModifyObjectCheckState( const utl::ISubject* pObject, int checkState );

	template< typename ObjectT >
	bool QueryObjectsWithCheckedState( std::vector<ObjectT*>& rCheckedObjects, int checkState = BST_CHECKED ) const;

	template< typename ObjectT >
	void SetObjectsCheckedState( const std::vector<ObjectT*>* pObjects, int checkState = BST_CHECKED, bool uncheckOthers = true );
private:
	ui::TRawCheckState GetRawCheckState( int index ) const { return GetItemState( index, LVIS_STATEIMAGEMASK ); }
	bool SetRawCheckState( int index, ui::TRawCheckState rawCheckState ) { return SetItemState( index, rawCheckState, LVIS_STATEIMAGEMASK ) != FALSE; }

	static bool HasRawCheckState( ui::TRawCheckState state ) { return HasFlag( state, LVIS_STATEIMAGEMASK ); }

	void ToggleCheckState( int index, int newCheckState );
	void SetItemCheckState( int index, int checkState );
	void NotifyCheckStatesChanged( void );
	size_t ApplyCheckStateToSelectedItems( int toggledIndex, int checkState );
public:
	// CARET and SELECTION
	int GetCaretIndex( void ) const { return FindItemWithState( LVNI_FOCUSED ); }
	bool SetCaretIndex( int index, bool doSet = true );

	int GetCurSel( void ) const;
	bool SetCurSel( int index, bool doSelect = true );		// caret and selection

	int GetSelCaretIndex( void ) const;						// if caret is not selected returns NULL, even if we have multiple selection (to allow NULL details when toggling an item unselected)

	template< typename ObjectT >
	ObjectT* GetCaretAs( void ) const;

	bool AnySelected( UINT stateMask = LVIS_SELECTED ) const { return HasItemWithState( stateMask ); }
	bool SingleSelected( void ) const;

	bool IsSelected( int index ) const { return HasItemState( index, LVIS_SELECTED ); }
	bool SetSelected( int index, bool doSelect = true ) { return SetItemState( index, doSelect ? LVIS_SELECTED : 0, LVIS_SELECTED ) != FALSE; }

	// multiple selection
	bool GetSelection( std::vector<int>& rSelIndexes, int* pCaretIndex = nullptr, int* pTopIndex = nullptr ) const;
	void SetSelection( const std::vector<int>& selIndexes, int caretIndex = -1 );
	void ClearSelection( void );
	void SelectAll( void );

	bool GetSelIndexBounds( int* pMinSelIndex, int* pMaxSelIndex ) const;
	Range<int> GetSelIndexRange( void ) const;

	template< typename ObjectT >
	ObjectT* GetSelected( void ) const;

	bool Select( const void* pObject );

	template< typename ObjectT >
	bool QuerySelectionAs( std::vector<ObjectT*>& rSelObjects ) const;

	template< typename ObjectT >
	void SelectObjects( const std::vector<ObjectT*>& objects );

	// selection service API
	int DeleteSelection( void );						// sends lv::LVN_ItemsRemoved
	void MoveSelectionTo( seq::MoveTo moveTo );			// sends lv::LVN_ItemsReorder (resequence)
public:
	// clipboard and drag-n-drop data
	enum { ListSourcesMask = ds::Indexes | ds::ItemsText | ds::ShellFiles };

	bool CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags = ListSourcesMask ) const;		// for drag-drop or clipboard transfers
	bool Copy( int sourceFlags = ListSourcesMask );						// creates a new COleDataSource object owned by the cliboard

	std::auto_ptr<CImageList> CreateDragImageMulti( const std::vector<int>& indexes, CPoint* pFrameOrigin = nullptr );
	std::auto_ptr<CImageList> CreateDragImageSelection( CPoint* pFrameOrigin = nullptr );

	CRect GetFrameBounds( const std::vector<int>& indexes ) const;
public:
	// custom cell marking: color, bold, italic, underline
	void MarkCellAt( int index, TColumn subItem, const ui::CTextEffect& textEfect );
	void MarkRowAt( int index, const ui::CTextEffect& textEfect ) { MarkCellAt( index, EntireRecord, textEfect ); }
	void UnmarkCellAt( int index, TColumn subItem );
	void ClearMarkedCells( void );

	const ui::CTextEffect* FindTextEffectAt( TRowKey rowKey, TColumn subItem ) const;

	// diff columns
	template< typename MatchFunc >
	void SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc );		// call after the list items are set up; by default pass str::TGetMatch()
protected:
	const CDiffColumnPair* FindDiffColumnPair( TColumn column ) const;
	bool IsDiffColumn( TColumn column ) const { return FindDiffColumnPair( column ) != nullptr; }
public:
	enum SortOrder { NotSorted, Ascending, Descending };				// column sort image index

	struct CItemData
	{
		CItemData( void ) : m_state( 0 ), m_lParam( 0 ) {}
	public:
		std::vector< std::pair<std::tstring, int> > m_subItems;		// text + imageIndex
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

#ifdef UTL_VISTA_GROUPS

	// groupId: typically an index in the vector of group objects maintained by the client.
	// groupIndex: display index, can change with sorting, whereas groupId is invariant to that.
	//
	int GetGroupId( int groupIndex ) const;

	int GetItemGroupId( int itemIndex ) const;							// itemIndex refers to a regular item in the list
	bool SetItemGroupId( int itemIndex, int groupId );

	template< typename ObjectT >
	void QueryGroupItems( std::vector<ObjectT*>& rObjectItems, int groupId ) const;

	std::pair<int, UINT> _GetGroupItemsRange( int groupId ) const;		// <firstItemIndex, itemCount> - not reliable with groups/items custom sorting, use QueryGroupItems instead (just the first item index is accurate)

	std::tstring GetGroupHeaderText( int groupId ) const;
	int InsertGroupHeader( int groupIndex, int groupId, const std::tstring& header, DWORD state = LVGS_NORMAL, DWORD align = LVGA_HEADER_LEFT );
	bool SetGroupFooter( int groupId, const std::tstring& footer, DWORD align = LVGA_FOOTER_CENTER );
	bool SetGroupTask( int groupId, const std::tstring& task );
	bool SetGroupSubtitle( int groupId, const std::tstring& subtitle );
	bool SetGroupTitleImage( int groupId, int image, const std::tstring& topDescr, const std::tstring& bottomDescr );

	void DeleteEntireGroup( int groupId );
	bool HasGroupState( int groupId, DWORD stateMask ) const { return GetGroupState( groupId, stateMask ) == stateMask; }

	void CheckEntireGroup( int groupId, bool check = true );

	void CollapseAllGroups( void );
	void ExpandAllGroups( void );
#endif //UTL_VISTA_GROUPS

private:
	bool UpdateCustomImagerBoundsSize( void );
private:
	enum ListOption
	{
		UseAlternateRowColoring		= BIT_FLAG( 0 ),
		SortInternally				= BIT_FLAG( 1 ),
		PersistSorting				= BIT_FLAG( 2 ),		// can be disabled if sorting is controlled externally (shared sorting criteria)
		AcceptDropFiles				= BIT_FLAG( 3 ),		// enable as Explorer drop target, send LVN_DropFiles notification when files are dropped onto the list
		CommandFrame				= BIT_FLAG( 4 ),		// list is the command target FIRST, also owner of the paired toolbar (for multiple lists in the same dialog, that have similar commands) - optionally may use m_pFrameEditor
		HighlightTextDiffsFrame		= BIT_FLAG( 5 ),		// highlight text differences with a filled frame
		ToggleCheckSelItems			= BIT_FLAG( 6 ),		// multi-selection: toggle checked state for the selected items
		UseExternalImagesSmall		= BIT_FLAG( 7 ),		// externally managed small images
		UseExternalImagesLarge		= BIT_FLAG( 8 ),		// externally managed large images
			UseExternalImages		= UseExternalImagesSmall | UseExternalImagesLarge
	};

	bool SetOptionFlag( ListOption flag, bool on );
private:
	UINT m_columnLayoutId;
	DWORD m_listStyleEx;
	std::tstring m_regSection;
	persist std::vector<CColumnInfo> m_columnInfos;
	std::vector<UINT> m_tileColumns;						// columns to be displayed as tile additional text (in gray)
	int m_optionFlags;
	bool m_subjectBased;									// objects stored as pointers are derived from utl::ISubject (polymorphic type)
	const TCHAR* m_pTabularSep;								// NULL by default (copy Code column text); could be set to "\t" for a tab-separated copy to clipboard

	persist TColumn m_sortByColumn;
	persist bool m_sortAscending;
	PFNLVCOMPARE m_pComparePtrFunc;							// compare by object ptr such as: static CompareResult CALLBACK CompareObjects( const CFoo* pLeft, const CFoo* pRight, CReportListControl* pThis );

	std::vector<CColumnComparator> m_comparators;
	utl::vector_map<TRowKey, int> m_initialItemsOrder;		// stores items initial order just after list set-up
	std::multimap<int, TRowKey> m_groupIdToItemsMap;		// group ID per row-key items map

	CImageList* m_pImageList;
	CImageList* m_pLargeImageList;
	const ui::ICheckStatePolicy* m_pCheckStatePolicy;		// for extended check states

	typedef std::pair<TRowKey, TColumn> TCellPair;			// invariant to sorting: favour LPARAMs instead of indexes
	typedef std::unordered_map<TCellPair, ui::CTextEffect, utl::CPairHasher> TCellTextEffectMap;
	TCellTextEffectMap m_markedCells;

	std::list<CDiffColumnPair> m_diffColumnPairs;

	CMenu* m_pPopupMenu[ _ListPopupCount ];					// used when right clicking nowhere - on header or no list item
	std::auto_ptr<CLabelEdit> m_pLabelEdit;					// stores the label info during inline editing

	ui::ICommandFrame* m_pFrameEditor;						// for frame editor command handling mode
	ole::IDataSourceFactory* m_pDataSourceFactory;			// creates ole::CDataSource for clipboard and drag-drop
private:
	bool m_painting;										// true during OnPaint() - supresses item text callback for diff columns to prevent default list sub-item draw (diffs are custom drawn)
	bool m_applyingCheckStateToSelectedItems;				// true during ApplyCheckStateToSelectedItems()
	mutable CSize m_stateIconSize;							// self-encapsulated, call GetStateIconSize(): cached size of an icon in the StateImageList
	std::auto_ptr<lv::CNmCheckStatesChanged> m_pNmToggling;	// set during OnLvnItemChanging_Reflect() - user toggling check-state with extended states
public:
	ui::CTextEffect m_deleteSrc_DiffEffect;					// text diffs: text removed from SRC (red)
	ui::CTextEffect m_mismatchDest_DiffEffect;				// text diffs: text mismatched in DEST (blue)
	ui::CTextEffect m_matchDest_DiffEffect;					// text diffs: text matched in DEST (gray)

	static const COLORREF s_deleteSrcTextColor = color::Red;
	static const COLORREF s_mismatchDestTextColor = color::Blue;
private:
	static const TCHAR s_fmtRegColumnLayout[];

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnWindowPosChanged( WINDOWPOS* pWndPos );
	afx_msg void OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags );
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnPaint( void );
	afx_msg BOOL OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
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
	afx_msg void OnExpandCollapseGroups( UINT cmdId );
	afx_msg void OnUpdateExpandCollapseGroups( CCmdUI* pCmdUI );
	afx_msg void OnUpdateAnySelected( CCmdUI* pCmdUI );
	afx_msg void OnUpdateSingleSelected( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace lv
{
	template< typename ValueT >
	struct CScopedStatus
	{
		CScopedStatus( CReportListControl* pListCtrl, bool useTopItem = false );	// pass useTopItem=true only if items order does not change during content change
		~CScopedStatus() { Restore(); }

		void Restore( void );
	public:
		CReportListControl* m_pListCtrl;
		ValueT m_topVisibleItem;
		ValueT m_caretItem;
		std::vector<ValueT> m_selItems;
	};


	typedef CScopedStatus<int> TScopedStatus_ByIndex;
	typedef CScopedStatus<utl::ISubject*> TScopedStatus_ByObject;
	typedef CScopedStatus<std::tstring> TScopedStatus_ByText;
}


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
	static std::auto_ptr<CSelFlowSequence> MakeFlow( CReportListControl* pListCtrl );

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
	CListSelectionData( CWnd* pSrcWnd = nullptr );
	CListSelectionData( CReportListControl* pSrcListCtrl );

	bool IsValid( void ) const { return m_pSrcWnd != nullptr && !m_selIndexes.empty(); }
	static CLIPFORMAT GetClipFormat( void );

	// serial::IStreamable interface
	virtual void Save( CArchive& archive ) throws_( CException* );
	virtual void Load( CArchive& archive ) throws_( CException* );
public:
	CWnd* m_pSrcWnd;
	std::vector<int> m_selIndexes;
};


// CReportListControl inline code

inline bool CReportListControl::CacheSelectionData( ole::CDataSource* pDataSource, int sourceFlags /*= ListSourcesMask*/ ) const
{
	CListSelectionData selData( const_cast<CReportListControl*>( this ) );
	return CacheSelectionData( pDataSource, sourceFlags, selData );
}


#endif // ReportListControl_h
