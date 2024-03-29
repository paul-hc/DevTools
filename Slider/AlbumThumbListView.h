#ifndef AlbumThumbListView_h
#define AlbumThumbListView_h
#pragma once

#include "AlbumModel.h"
#include "ListViewState.h"
#include "utl/UI/BaseItemTooltipsCtrl.h"
#include "utl/UI/ObjectCtrlBase.h"
#include "utl/UI/OleDragDrop_fwd.h"
#include "utl/UI/OleDropTarget.h"
#include "utl/UI/ThemeItem.h"
#include "utl/UI/WindowTimer.h"


class CAlbumImageView;
class CAlbumDoc;
class CSplitterWindow;
class CWicDibSection;


class CAlbumThumbListView : public CBaseItemTooltipsCtrl<CBaseCtrlView>		// a view encapsulating a CListBox
	, public CObjectCtrlBase
{
	typedef CBaseItemTooltipsCtrl<CBaseCtrlView> TBaseClass;

	DECLARE_DYNCREATE( CAlbumThumbListView )
protected:
	CAlbumThumbListView( void );
	virtual ~CAlbumThumbListView();
private:
	// CBaseItemTooltipsCtrl item interface
	virtual utl::ISubject* GetItemSubjectAt( int index ) const;
	virtual CRect GetItemRectAt( int index ) const;
	virtual int GetItemFromPoint( const CPoint& clientPos ) const;
public:
	void StorePeerView( CAlbumImageView* pPeerImageView );

	CAlbumImageView* GetAlbumImageView( void ) const { return safe_ptr( m_pPeerImageView ); }
	CAlbumDoc* GetAlbumDoc( void ) const;

	const CAlbumModel* GetAlbumModel( void ) const { return m_pAlbumModel; }
	void SetupAlbumModel( const CAlbumModel* pAlbumModel, bool doRedraw = true );

	CListBox* AsListBox( void ) const { return (CListBox*)this; }

	bool IsMultiSelection( void ) const { return HasFlag( AsListBox()->GetStyle(), LBS_EXTENDEDSEL | LBS_MULTIPLESEL ); }
	int GetCurSel( void ) const;
	bool SetCurSel( int selIndex, bool notifySelChanged = false );
	bool QuerySelItemPaths( std::vector<fs::CFlexPath>& rSelFilePaths ) const;

	void GetListViewState( CListViewState& rLvState, bool filesMustExist = true, bool sortAscending = true ) const;
	void SetListViewState( const CListViewState& lvState, bool notifySelChanged = false, const TCHAR* pDoRestore = _T("TSC") );
	void SelectAll( bool notifySelChanged = false );

	int GetPointedImageIndex( void ) const;

	bool BackupSelection( bool currentSelection = true );
	void RestoreSelection( void );
	bool SelectionOverlapsWith( const std::vector<int>& displayIndexes ) const;

	// splitter width quantification
	int QuantifyListWidth( int listWidth );
	int GetListClientWidth( int listWidth );

	enum CheckLayoutMode { SplitterTrack, AlbumViewInit, ShowCommand };

	bool CheckListLayout( CheckLayoutMode checkMode = SplitterTrack );

	CSize GetPageItemCounts( void ) const;		// item counts in the list: (horizontal, vertical)
	static CRect GetListWindowRect( int columnCount = 1, CWnd* pListWnd = nullptr );

	const std::vector<int>& GetDragSelIndexes( void ) const { return m_dragSelIndexes; }
private:
	CWicDibSection* GetItemThumb( int displayIndex ) const throws_();
	const fs::CFlexPath* GetItemPath( int displayIndex ) const;

	bool DoDragDrop( void );
	void CancelDragCapture( void );

	bool IsValidImageIndex( size_t displayIndex ) const { return m_pAlbumModel != nullptr && displayIndex < m_pAlbumModel->GetFileAttrCount(); }
	bool IsValidFileAt( size_t displayIndex ) const;

	bool NotifySelChange( void );

	bool RecreateView( int columnCount );

	static CSize GetInitialSize( int columnCount = 1 );
	static CRect GetNcExtentRect( int columnCount = 1, CWnd* pListWnd = nullptr );
	static int ComputeColumnCount( int listClientWidth ) { return int( double( listClientWidth ) / GetInitialSize().cx + 0.5 ); }
	static void EnsureCaptionFontCreated( void );
	static DWORD GetListCreationStyle( int columnCount ) { return 1 == columnCount ? ( WS_VSCROLL | LBS_DISABLENOSCROLL ) : ( LBS_MULTICOLUMN | WS_HSCROLL ); }
public:
	enum Metrics { cxSide = 2, cyTop = 2, cyTextSpace = 2, ID_BEGIN_DRAG_TIMER = 1000 };
private:
	bool m_autoDelete;
	const CAlbumModel* m_pAlbumModel;

	CAlbumImageView* m_pPeerImageView;
	CSplitterWindow* m_pSplitterWnd;
	CThemeItem m_selBkThemeItem;
	CWindowTimer m_beginDragTimer;
	int m_userChangeSel;							// true during user selection operation
	CRect m_startDragRect;

	// custom order drag & drop
	std::vector<int> m_dragSelIndexes;				// contains indexes to be dropped in custom order
	ole::CDropTarget m_dropTarget;					// view is registered as drop target to this data-member
	CPoint m_scrollTimerCounter;					// counter for custom order scrolling (D&D timer auto-scroll)
	CListViewState m_selectionBackup;				// used for saving the selection (near or current)

	static CFont s_fontCaption;
	static int s_fontHeight;
	static DWORD s_listCreationStyle;

	static CSize scrollTimerDivider;				// divider for custom order scrolling (D&D timer auto-scroll)
private:
	struct CBackupData
	{
		CBackupData( const CAlbumThumbListView* pSrcThumbView );
		~CBackupData();

		void Restore( CAlbumThumbListView* pDestThumbView );
	public:
		const CAlbumModel* m_pAlbumModel;
		int m_topIndex;
		int m_currIndex;
		DWORD m_listCreationStyle;
	};

	friend struct CBackupData;
protected:
	// custom order drag & drop overrides
	virtual DROPEFFECT OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point );
	virtual DROPEFFECT OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point );
	virtual BOOL OnDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
	virtual void OnDragLeave( void );

	// generated stuff
public:
	virtual BOOL OnChildNotify( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL OnScroll( UINT scrollCode, UINT pos, BOOL doScroll = TRUE );
	virtual void MeasureItem( MEASUREITEMSTRUCT* pMIS );
	virtual void DrawItem( DRAWITEMSTRUCT* pDIS );
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& rCS );
	virtual void PostNcDestroy( void );
	virtual void OnActivateView( BOOL activate, CView* pActivateView, CView* pDeactiveView );
	virtual BOOL OnScrollBy( CSize sizeScroll, BOOL doScroll = TRUE );
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );
	HBRUSH CtlColor( CDC* pDC, UINT ctlColor );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnLButtonDown( UINT mkFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT mkFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT mkFlags, CPoint point );
	afx_msg void OnMouseMove( UINT mkFlags, CPoint point );
	afx_msg BOOL OnMouseWheel( UINT mkFlags, short zDelta, CPoint pt );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg void OnToggle_ShowThumbView( void );
	afx_msg void OnUpdate_ShowThumbView( CCmdUI* pCmdUI );
	afx_msg void OnLBnSelChange( void );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumThumbListView_h
