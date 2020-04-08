#ifndef AlbumThumbListView_h
#define AlbumThumbListView_h
#pragma once

#include "FileList.h"
#include "ListViewState.h"
#include "utl/UI/ObjectCtrlBase.h"
#include "utl/UI/OleDragDrop_fwd.h"
#include "utl/UI/OleDropTarget.h"
#include "utl/UI/WindowTimer.h"


class CAlbumImageView;
class CAlbumDoc;
class CSplitterWindow;
class CWicDibSection;


class CAlbumThumbListView : public CCtrlView
						  , public CObjectCtrlBase
{
	DECLARE_DYNCREATE( CAlbumThumbListView )
protected:
	CAlbumThumbListView( void );
	virtual ~CAlbumThumbListView();
public:
	void StorePeerView( CAlbumImageView* pPeerImageView );

	CAlbumImageView* GetAlbumImageView( void ) const { return safe_ptr( m_pPeerImageView ); }
	CAlbumDoc* GetAlbumDoc( void ) const;

	const CFileList* GetFileList( void ) const { return m_pFileList; }
	void SetupFileList( const CFileList* pFileList, bool doRedraw = true );

	CListBox* AsListBox( void ) const { return (CListBox*)this; }

	bool IsMultiSelection( void ) const { return HasFlag( AsListBox()->GetStyle(), LBS_EXTENDEDSEL | LBS_MULTIPLESEL ); }
	int GetCurSel( void ) const;
	bool SetCurSel( int selIndex, bool notifySelChanged = false );
	bool QuerySelItemPaths( std::vector< fs::CPath >& rSelFilePaths ) const;

	void GetListViewState( CListViewState& rLvState, bool filesMustExist = true, bool sortAscending = true ) const;
	void SetListViewState( const CListViewState& lvState, bool notifySelChanged = false, const TCHAR* pDoRestore = _T("TSC") );
	void SelectAll( bool notifySelChanged = false );

	int GetPointedImageIndex( void ) const;

	bool BackupSelection( bool currentSelection = true );
	void RestoreSelection( void );
	bool SelectionOverlapsWith( const std::vector< int >& displayIndexes = s_toMoveIndexes ) const;

	// splitter width quantification
	int QuantifyListWidth( int listWidth );
	int GetListClientWidth( int listWidth );

	enum CheckLayoutMode { SplitterTrack, AlbumViewInit, ShowCommand };

	bool CheckListLayout( CheckLayoutMode checkMode = SplitterTrack );

	CSize GetPageScrollExtent( void ) const;
	static CRect GetListWindowRect( int columnCount = 1, CWnd* pListWnd = NULL );
private:
	CWicDibSection* GetItemThumb( int displayIndex ) const throws_();
	const fs::CFlexPath* GetItemPath( int displayIndex ) const;

	bool DoDragDrop( void );
	void CancelDragCapture( void );

	int GetImageIndexFromPoint( CPoint& clientPos ) const;
	bool IsValidImageIndex( size_t displayIndex ) const { return m_pFileList != NULL && displayIndex < m_pFileList->GetFileAttrCount(); }
	bool IsValidFileAt( int displayIndex ) const;

	bool NotifySelChange( void );

	bool RecreateView( int columnCount );

	static CSize GetInitialSize( int columnCount = 1 );
	static CRect GetNcExtentRect( int columnCount = 1, CWnd* pListWnd = NULL );
	static int ComputeColumnCount( int listClientWidth ) { return int( double( listClientWidth ) / GetInitialSize().cx + 0.5 ); }
	static CMenu& GetContextMenu( void );
	static void EnsureCaptionFontCreated( void );
	static DWORD GetListCreationStyle( int columnCount ) { return 1 == columnCount ? ( WS_VSCROLL | LBS_DISABLENOSCROLL ) : ( LBS_MULTICOLUMN | WS_HSCROLL ); }
public:
	enum Metrics { cxSide = 2, cyTop = 2, cyTextSpace = 2, ID_BEGIN_DRAG_TIMER = 1000 };
private:
	bool m_autoDelete;
	const CFileList* m_pFileList;

	CAlbumImageView* m_pPeerImageView;
	CSplitterWindow* m_pSplitterWnd;
	CWindowTimer m_beginDragTimer;
	int m_userChangeSel;							// true during user selection operation
	CRect m_startDragRect;

	// custom order drag & drop
	ole::CDropTarget m_dropTarget;					// view is registered as drop target to this data-member
	CPoint m_scrollTimerCounter;					// counter for custom order scrolling (D&D timer auto-scroll)
	CListViewState m_selectionBackup;				// used for saving the selection (near or current)

	static CFont s_fontCaption;
	static int s_fontHeight;
	static DWORD s_listCreationStyle;
public:
	static std::vector< int > s_toMoveIndexes;	// contains indexes to be dropped in custom order
	static CSize scrollTimerDivider;				// divider for custom order scrolling (D&D timer auto-scroll)
private:
	struct CBackupData
	{
		CBackupData( const CAlbumThumbListView* pSrcThumbView );
		~CBackupData();

		void Restore( CAlbumThumbListView* pDestThumbView );
	public:
		const CFileList* m_pFileList;
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
public:
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
	afx_msg void OnToggleShowThumbView( void );
	afx_msg void OnUpdateShowThumbView( CCmdUI* pCmdUI );
	afx_msg void OnLBnSelChange( void );

	DECLARE_MESSAGE_MAP()
};


// CAlbumThumbListView inline code

inline bool CAlbumThumbListView::IsValidFileAt( int displayIndex ) const
{
	if ( !IsValidImageIndex( displayIndex ) )
		return false;
	return m_pFileList->GetFileAttr( displayIndex ).GetPath().FileExist();
}


#endif // AlbumThumbListView_h
