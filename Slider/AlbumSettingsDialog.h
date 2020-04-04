#ifndef AlbumSettingsDialog_h
#define AlbumSettingsDialog_h
#pragma once

#include "FileList.h"
#include "ThumbPreviewCtrl.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/DragListCtrl.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/OleUtils.h"
#include "utl/UI/SpinEdit.h"


class CImageView;


class CAlbumSettingsDialog : public CLayoutDialog
						   , public ole::IDataSourceFactory
{
public:
	CAlbumSettingsDialog( const CFileList& fileList, int currentIndex = -1, CWnd* pParent = NULL );
	virtual ~CAlbumSettingsDialog();
private:
	static CMenu& GetFileListPopupMenu( void );
	bool InitSymbolFont( void );

	void SetDirty( bool dirty = true );
	void UpdateFileSortOrder( void );

	enum Column { FileName, Folder, Dimensions, Size, Date, Unordered = -1 };
	static std::pair< Column, bool > ToListSortOrder( CFileList::Order fileOrder );		// < sortByColumn, sortAscending >

	bool DropSearchSpec( const fs::CFlexPath& filePath, bool doPrompt = true );
	bool DeleteSearchSpec( int index, bool doPrompt = true );
	bool MoveSearchSpec( int moveBy );
	bool AddSearchSpec( int index );
	bool ModifySearchSpec( int index );

	int CheckForDuplicates( const TCHAR* pFilePath, int ignoreIndex = -1 );

	bool SearchSourceFiles( void );
	void SetupFoundListView( void );

	void QueryFoundListSelection( std::vector< std::tstring >& rSelFilePaths, bool clearInvalidFiles = true );
	int GetCheckStateAutoRegen( void ) const;

	// ole::IDataSourceFactory interface
	virtual ole::CDataSource* NewDataSource( void );
private:
	CAccelTable m_dlgAccel, m_searchListAccel;
	enum { Undefined = -1, False, True, Toggle } m_isDirty;
	CFont m_symbolFont;
public:
	CFileList m_fileList;
	int m_currentIndex;
private:
	// enum { IDD = IDD_ALBUM_SETTINGS_DIALOG };
	enum { InplaceSortMaxCount = 5000 };

	CSpinEdit m_minSizeEdit;
	CSpinEdit m_maxSizeEdit;
	CButton m_moveDownButton;
	CButton m_moveUpButton;
	CListBox m_searchSpecListBox;
	CComboBox m_sortOrderCombo;
	CThumbPreviewCtrl m_thumbPreviewCtrl;
	CDialogToolBar m_toolbar;
	CDragListCtrl< CPathItemListCtrl > m_foundFilesListCtrl;

	// generated stuff
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	virtual void OnOK( void );
	virtual void OnCancel( void );
	afx_msg void OnCBnSelChange_SortOrder( void );
	afx_msg void OnToggle_MinSize( void );
	afx_msg void OnToggle_MaxSize( void );
	afx_msg void OnEnChange_MinMaxSize( void );
	afx_msg void OnToggle_AutoRegenerate( void );
	afx_msg void OnToggle_AutoDrop( void );
	afx_msg void OnLBnSelChange_SearchSpec( void );
	afx_msg void OnLBnDblclk_SearchSpec( void );
	afx_msg void OnMoveUp_SearchSpec( void );
	afx_msg void OnMoveDown_SearchSpec( void );
	afx_msg void OnAdd_SearchSpec( void );
	afx_msg void OnModify_SearchSpec( void );
	afx_msg void OnDelete_SearchSpec( void );
	afx_msg void OnSearchSourceFiles( void );
	afx_msg void OnOrderRandomShuffle( void );
	afx_msg void OnUpdateOrderRandomShuffle( CCmdUI* pCmdUI );
	afx_msg void OnLVnColumnClick_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemChanged_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnGetDispInfo_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemsReorder_FoundFiles( void );
	afx_msg void OnImageFileOp( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumSettingsDialog_h
