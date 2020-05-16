#ifndef AlbumSettingsDialog_h
#define AlbumSettingsDialog_h
#pragma once

#include "AlbumModel.h"
#include "utl/Resequence.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/DragListCtrl.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/IconButton.h"
#include "utl/UI/OleUtils.h"
#include "utl/UI/SpinEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "utl/UI/ThumbPreviewCtrl.h"


class CImageView;


class CAlbumSettingsDialog : public CLayoutDialog
						   , public CInternalChange
						   , public ole::IDataSourceFactory
						   , private ui::ITextEffectCallback
{
public:
	CAlbumSettingsDialog( const CAlbumModel& model, size_t currentPos, CWnd* pParent = NULL );
	virtual ~CAlbumSettingsDialog();

	const CAlbumModel& GetModel( void ) const { return m_model; }
	int GetCurrentIndex( void ) const;
private:
	static CMenu& GetAlbumModelPopupMenu( void );
	bool InitSymbolFont( void );

	// ole::IDataSourceFactory interface
	virtual ole::CDataSource* NewDataSource( void );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
private:
	bool SetDirty( bool dirty = true );

	void OutputAll( void );
	void InputAll( void );

	enum Column { FileName, Folder, Dimensions, Size, Date, Unordered = -1 };
	static std::pair< Column, bool > ToListSortOrder( fattr::Order fileOrder );		// < sortByColumn, sortAscending >

	bool DropSearchSpec( const fs::CFlexPath& filePath, bool doPrompt = true );
	bool DeleteSearchSpec( int index, bool doPrompt = true );
	bool MoveSearchSpec( seq::Direction moveBy );
	bool AddSearchSpec( int index );
	bool ModifySearchSpec( int index );

	int CheckForDuplicates( const TCHAR* pFilePath, int ignoreIndex = -1 );

	bool SearchSourceFiles( void );
	void SetupFoundListView( void );

	void UpdateCurrentFile( void );
	int GetCheckStateAutoRegen( void ) const;
private:
	CAlbumModel m_model;
	const CFileAttr* m_pCaretFileAttr;
	bool m_isDirty;

	std::vector< fs::CFlexPath > m_newFilePaths;		// for highlighting new found images

	CAccelTable m_dlgAccel, m_searchListAccel;
	CFont m_symbolFont;
private:
	// enum { IDD = IDD_ALBUM_SETTINGS_DIALOG };

	CSpinEdit m_maxFileCountEdit;
	CSpinEdit m_minSizeEdit;
	CSpinEdit m_maxSizeEdit;
	CButton m_moveDownButton;
	CButton m_moveUpButton;
	CListBox m_searchSpecListBox;
	CEnumComboBox m_sortOrderCombo;
	CThumbPreviewCtrl m_thumbPreviewCtrl;
	CRegularStatic m_docVersionLabel;
	CRegularStatic m_docVersionStatic;
	CDialogToolBar m_toolbar;
	CDragListCtrl< CPathItemListCtrl > m_foundFilesListCtrl;
	CIconButton m_okButton;					// overloaded text/icon, depending on dirtyness

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	virtual void OnOK( void );
	virtual void OnCancel( void );
	afx_msg void OnCBnSelChange_SortOrder( void );
	afx_msg void OnToggle_MaxFileCount( void );
	afx_msg void OnToggle_MinSize( void );
	afx_msg void OnToggle_MaxSize( void );
	afx_msg void OnEnChange_MaxFileCount( void );
	afx_msg void OnEnChange_MinMaxSize( void );
	afx_msg void OnToggle_AutoRegenerate( void );
	afx_msg void OnToggle_AutoDrop( void );
	afx_msg void OnLBnSelChange_SearchSpec( void );
	afx_msg void OnLBnDblclk_SearchSpec( void );
	afx_msg void On_MoveUp_SearchSpec( void );
	afx_msg void On_MoveDown_SearchSpec( void );
	afx_msg void OnAdd_SearchSpec( void );
	afx_msg void OnModify_SearchSpec( void );
	afx_msg void OnDelete_SearchSpec( void );
	afx_msg void OnSearchSourceFiles( void );
	afx_msg void On_OrderRandomShuffle( UINT cmdId );
	afx_msg void OnUpdate_OrderRandomShuffle( CCmdUI* pCmdUI );
	afx_msg void OnLVnColumnClick_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemChanged_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnGetDispInfo_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemsReorder_FoundFiles( void );
	afx_msg void OnImageFileOp( UINT cmdId );
	afx_msg void OnStnDblClk_ThumbPreviewStatic( void );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumSettingsDialog_h
