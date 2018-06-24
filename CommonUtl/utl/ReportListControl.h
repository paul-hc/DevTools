#ifndef ReportListControl_h
#define ReportListControl_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "ISubject.h"
#include "SubjectPredicates.h"
#include "Image_fwd.h"
#include "OleUtils.h"
#include "MatchSequence.h"
#include "Resequence.h"
#include "vector_map.h"
#include "ui_fwd.h"
#include <vector>
#include <list>
#include <hash_map>
#include <afxcmn.h>


class CListSelectionData;
class CReportListCustomDraw;
namespace ui { class CFontEffectCache; }
namespace ole { class CDataSource; }

namespace ds
{
	enum DataSourceFlags
	{
		Indexes		= BIT_FLAG( 0 ),		// "cfListSelIndexes"
		ItemsText	= BIT_FLAG( 1 ),		// CF_TEXT, CF_UNICODETEXT
		ShellFiles	= BIT_FLAG( 2 )			// cfHDROP
	};
}


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


class CReportListControl : public CListCtrl
						 , public CInternalChange
{
	friend class CReportListCustomDraw;
public:
	enum
	{
		DefaultStyleEx = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
			LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_AUTOAUTOARRANGE /*| LVS_EX_JUSTIFYCOLUMNS*/
	};
	enum Notification { LVN_ItemsReorder = 1000 };

	CReportListControl( UINT columnLayoutId = 0, DWORD listStyleEx = DefaultStyleEx );
	virtual ~CReportListControl();

	void SaveToRegistry( void );
	bool LoadFromRegistry( void );

	void Set_ImageList( CImageList* pImageList, CImageList* pLargeImageList = NULL ) { m_pImageList = pImageList; m_pLargeImageList = pLargeImageList; }
	void ChangeListViewMode( DWORD viewMode );

	bool IsMultiSelectionList( void ) const { return !HasFlag( GetStyle(), LVS_SINGLESEL ); }

	bool UseExplorerTheme( void ) const { return m_useExplorerTheme; }
	void SetUseExplorerTheme( bool useExplorerTheme = true );

	bool UseAlternateRowColoring( void ) const { return m_useAlternateRowColoring; }
	void SetUseAlternateRowColoring( bool useAlternateRowColoring = true ) { m_useAlternateRowColoring = useAlternateRowColoring; }

	const std::tstring& GetSection( void ) const { return m_regSection; }
	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }
public:
	typedef int ImageListPos;

	void SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, ImageListPos transpImgPos = -1 );

	enum ListPopup { OnSelection, Nowhere, _ListPopupCount };

	void SetPopupMenu( ListPopup popupType, CMenu* pPopupMenu ) { m_pPopupMenu[ popupType ] = pPopupMenu; }
	static CMenu& GetStdPopupMenu( ListPopup popupType );

	ole::IDataSourceFactory* GetDataSourceFactory( void ) const { return m_pDataSourceFactory; }
	void SetDataSourceFactory( ole::IDataSourceFactory* pDataSourceFactory ) { ASSERT_PTR( pDataSourceFactory ); m_pDataSourceFactory = pDataSourceFactory; }

	enum StdMetrics { IconViewEdgeX = 4 };

	bool SetCompactIconSpacing( int iconEdgeWidth = IconViewEdgeX );		// to compact items width in icon views

	int GetTopIndex( void ) const;
	void SetTopIndex( int topIndex );

	enum MyHitTest { LVHT_MY_PASTEND = 0x00080000 };

	int HitTest( CPoint pos, UINT* pFlags = NULL ) const;
	int GetDropIndexAtPoint( const CPoint& point ) const;
	bool IsItemFullyVisible( int index ) const;

	static bool IsSelectionChangedNotify( const NMLISTVIEW* pNmList, UINT selMask = LVIS_SELECTED | LVIS_FOCUSED );
public:
	typedef int TColumn;
	typedef LPARAM TRowKey;								// row keys are invariant to sorting

	enum { EntireRecord = (TColumn)-1 };

	TRowKey MakeRowKeyAt( int index ) const;			// favour item data (LPARAM), or fall back to index (int)

	// sorting
	std::pair< TColumn, bool > GetSortByColumn( void ) const { return std::make_pair( m_sortByColumn, m_sortAscending ); }
	void SetSortByColumn( TColumn sortByColumn, bool sortAscending = true );

	void SetSortInternally( bool sortInternally ) { m_sortInternally = sortInternally; }

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
private:
	void UpdateColumnSortHeader( void );
	bool IsDefaultAscending( TColumn column ) const;				// e.g. allows date-time descending ordering by default

	static pred::CompareResult CALLBACK ObjectCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl );

	static pred::CompareResult CALLBACK InitialOrderCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl );
	pred::CompareResult CompareInitialOrder( TRowKey leftKey, TRowKey rightKey ) const;

	static int CALLBACK TextCompareProc( LPARAM leftParam, LPARAM rightParam, CReportListControl* pListCtrl );
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
	void NotifyParent( Notification notifCode );

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

	enum Column { Code };

	virtual int InsertObjectItem( int index, const utl::ISubject* pObject, int imageIndex = No_Image );

	void SetSubItemText( int index, int subItem, const std::tstring& text, int imageIndex = No_Image );
	void SetSubItemImage( int index, int subItem, int imageIndex );

	bool SetItemTileInfo( int index );

	CString GetItemText( int index, int subItem ) const;		// override CListCtrl for stored diff-items text

	void SwapItems( int index1, int index2 );
	void DropMoveItems( int destIndex, const std::vector< int >& selIndexes );
	bool ForceRearrangeItems( void );				// for icon view modes

	int Find( const void* pObject ) const;
	int FindItemIndex( const std::tstring& itemText ) const;
	int FindItemIndex( LPARAM lParam ) const;

	void QueryItemsText( std::vector< std::tstring >& rItemsText, const std::vector< int >& indexes, int subItem = 0 ) const;
	void QuerySelectedItemsText( std::vector< std::tstring >& rItemsText, int subItem = 0 ) const;
	void QueryAllItemsText( std::vector< std::tstring >& rItemsText, int subItem = 0 ) const;

	// item state
	bool HasItemState( int index, UINT stateMask ) const { return ( GetItemState( index, stateMask ) & stateMask ) != 0; }
	bool HasItemStrictState( int index, UINT stateMask ) const { return stateMask == ( GetItemState( index, stateMask ) & stateMask ); }
	int FindItemWithState( UINT stateMask ) const { return GetNextItem( -1, LVNI_ALL | stateMask ); }

	bool IsChecked( int index ) const { return GetCheck( index ) != FALSE; }
	bool SetChecked( int index, bool check = true ) { return SetCheck( index, check ) != FALSE; }

	// extended check state (includes radio check states)
	CheckState GetCheckState( int index ) const { return static_cast< CheckState >( GetItemState( index, LVIS_STATEIMAGEMASK ) ); }
	void SetCheckState( int index, CheckState checkState ) { SetItemState( index, checkState, LVIS_STATEIMAGEMASK ); }

	bool UseExtendedCheckStates( void ) const;
	bool SetupExtendedCheckStates( void );

	bool UseTriStateAutoCheck( void ) const { return m_useTriStateAutoCheck; }
	void SetUseTriStateAutoCheck( bool useTriStateAutoCheck ) { m_useTriStateAutoCheck = useTriStateAutoCheck; }

	static bool StateChanged( UINT newState, UINT oldState, UINT stateMask ) { return ( newState & stateMask ) != ( oldState & stateMask ); }
	static bool HasCheckState( UINT state ) { return ( state & LVIS_STATEIMAGEMASK ) != 0; }

	static CheckState AsCheckState( UINT state ) { return static_cast< CheckState >( state & LVIS_STATEIMAGEMASK ); }
	static bool IsCheckedState( UINT state );

	// caret and selection
	int GetCaretIndex( void ) const { return FindItemWithState( LVNI_FOCUSED ); }
	bool SetCaretIndex( int index, bool doSet = true );

	int GetCurSel( void ) const;
	bool SetCurSel( int index, bool doSelect = true );		// caret and selection

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
		virtual void ModifyDiffTextEffectAt( std::vector< ui::CTextEffect >& rMatchEffects, LPARAM rowKey, int subItem ) const { rMatchEffects, rowKey, subItem; }
	};

	void SetTextEffectCallback( ITextEffectCallback* pTextEffectCallback ) { m_pTextEffectCallback = pTextEffectCallback; }

	// diff columns
	template< typename MatchFunc >
	void SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc );		// call after the list items are set up; by default pass str::GetMatch()
protected:
	struct CDiffColumnPair
	{
		CDiffColumnPair( TColumn srcColumn = -1, TColumn destColumn = -1 ) : m_srcColumn( srcColumn ), m_destColumn( destColumn ) {}

		const str::TMatchSequence* FindRowSequence( TRowKey rowKey ) const;
	public:
		TColumn m_srcColumn;
		TColumn m_destColumn;
		stdext::hash_map< TRowKey, str::TMatchSequence > m_rowSequences;		// TRowKey is invariant to sorting
	};

	const CDiffColumnPair* FindDiffColumnPair( TColumn column ) const;
	bool IsDiffColumn( TColumn column ) const { return FindDiffColumnPair( column ) != NULL; }

	ui::CFontEffectCache* GetFontEffectCache( void );
	bool ParentHandlesCustomDraw( void );
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

#if ( _WIN32_WINNT >= 0x0600 ) && defined( UNICODE )	// Vista or greater

	int GetGroupId( int groupIndex ) const;

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
	void AddTransparentImage( void );
private:
	UINT m_columnLayoutId;
	DWORD m_listStyleEx;
	std::tstring m_regSection;
	std::vector< CColumnInfo > m_columnInfos;
	std::vector< UINT > m_tileColumns;					// columns to be displayed as tile additional text (in gray)
	bool m_useExplorerTheme;
	bool m_useAlternateRowColoring;
	bool m_subjectBased;								// objects stored as pointers are derived from utl::ISubject (polymorphic type)

	TColumn m_sortByColumn;
	bool m_sortAscending;
	bool m_sortInternally;
	PFNLVCOMPARE m_pComparePtrFunc;						// compare by object ptr such as: static CompareResult CALLBACK CompareObjects( const CFoo* pLeft, const CFoo* pRight, CReportListControl* pListCtrl );

	std::vector< CColumnComparator > m_comparators;
	utl::vector_map< TRowKey, int > m_initialItemsOrder;	// stores items initial order just after list set-up

	typedef std::pair< TRowKey, TColumn > TCellPair;	// invariant to sorting: favour LPARAMs instead of indexes
	stdext::hash_map< TCellPair, ui::CTextEffect > m_markedCells;
	std::auto_ptr< ui::CFontEffectCache > m_pFontCache;		// self-encapsulated
	ITextEffectCallback* m_pTextEffectCallback;

	std::list< CDiffColumnPair > m_diffColumnPairs;

	CImageList* m_pImageList;
	CImageList* m_pLargeImageList;

	CMenu* m_pPopupMenu[ _ListPopupCount ];				// used when right clicking nowhere - on header or no list item
	bool m_useTriStateAutoCheck;						// extended check state: allows toggling LVIS_UNCHECKED -> LVIS_CHECKED -> LVIS_CHECKEDGRAY
	std::auto_ptr< CLabelEdit > m_pLabelEdit;			// stores the label info during inline editing

	CAccelTable m_listAccel;
	ole::IDataSourceFactory* m_pDataSourceFactory;		// creates ole::CDataSource for clipboard and drag-drop
private:
	struct CCustomImager
	{
		CCustomImager( ui::ICustomImageDraw* pRenderer, ImageListPos transpPos ) : m_pRenderer( pRenderer ), m_transpPos( transpPos ) {}
	public:
		ui::ICustomImageDraw* m_pRenderer;				// thumbnail renderer
		ImageListPos m_transpPos;						// pos of the transparent entry in the image list
		CImageList m_imageListSmall;
		CImageList m_imageListLarge;
	};

	std::auto_ptr< CCustomImager > m_pCustomImager;

	BOOL m_parentHandlesCustomDraw;						// self-encapsulated
public:
	ui::CTextEffect m_listTextEffect;					// for all items in the list

	ui::CTextEffect m_deleteSrc_DiffEffect;				// text diffs: text removed from SRC (red)
	ui::CTextEffect m_mismatchDest_DiffEffect;			// text diffs: text mismatched in DEST (blue)
	ui::CTextEffect m_matchDest_DiffEffect;				// text diffs: text matched in DEST (gray)

	static const COLORREF s_deleteSrcTextColor = color::Red;
	static const COLORREF s_mismatchDestTextColor = color::Blue;
private:
	static const TCHAR s_fmtRegColumnLayout[];
	static const TCHAR s_diffColumnTextPlaceholder[];
public:
	// generated stuff
	public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnWindowPosChanged( WINDOWPOS* pWndPos );
	afx_msg void OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags );
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	virtual BOOL OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnHdnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnHdnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnColumnClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnBeginLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnLvnEndLabelEdit_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
public:
	afx_msg void OnListViewMode( UINT cmdId );
	afx_msg void OnUpdateListViewMode( CCmdUI* pCmdUI );
	afx_msg void OnListViewStacking( UINT cmdId );
	afx_msg void OnUpdateListViewStacking( CCmdUI* pCmdUI );
	afx_msg void OnResetColumnLayout( void );
	afx_msg void OnUpdateResetColumnLayout( CCmdUI* pCmdUI );
	afx_msg void OnCopy( void );
	afx_msg void OnUpdateCopy( CCmdUI* pCmdUI );
	afx_msg void OnSelectAll( void );
	afx_msg void OnUpdateSelectAll( CCmdUI* pCmdUI );
	afx_msg void OnMoveTo( UINT cmdId );
	afx_msg void OnUpdateMoveTo( CCmdUI* pCmdUI );

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
		SetSelection( selIndexes, selIndexes.front(), false );
	else
		ClearSelection( false );
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

		// Replace with empty text to allow custom draw without default sub-item draw interference (on CDDS_ITEMPOSTPAINT | CDDS_SUBITEM).
		// Sub-item text still accessible with GetItemText() method.
		VERIFY( SetItemText( index, srcColumn, s_diffColumnTextPlaceholder ) );
		VERIFY( SetItemText( index, destColumn, s_diffColumnTextPlaceholder ) );
	}
}


#endif // ReportListControl_h
