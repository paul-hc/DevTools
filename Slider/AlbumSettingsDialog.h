#ifndef AlbumSettingsDialog_h
#define AlbumSettingsDialog_h
#pragma once

#include "AlbumModel.h"
#include "utl/Resequence.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/DragListCtrl.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/IconButton.h"
#include "utl/UI/OleUtils.h"
#include "utl/UI/SpinEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "utl/UI/ThumbPreviewCtrl.h"
#include <unordered_set>


class CImageView;
class CListCtrlEditorFrame;
typedef CDragListCtrl<CPathItemListCtrl> TDragPathItemListCtrl;


class CAlbumSettingsDialog : public CLayoutDialog
						   , public CInternalChange
						   , public ole::IDataSourceFactory
						   , private ui::ITextEffectCallback
{
public:
	CAlbumSettingsDialog( const CAlbumModel& model, size_t currentPos, CWnd* pParent = nullptr );
	virtual ~CAlbumSettingsDialog();

	const CAlbumModel& GetModel( void ) const { return m_model; }
	int GetCurrentIndex( void ) const;
private:
	static CMenu& GetAlbumModelPopupMenu( void );

	// ole::IDataSourceFactory interface
	virtual ole::CDataSource* NewDataSource( void );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
private:
	bool SetDirty( bool dirty = true );

	void OutputAll( void );
	void InputAll( void );

	enum PatternsColumn { PatternPath, PatternType, PatternDepth };

	void SetupPatternsListView( void );

	enum ImagesColumn { FileName, Folder, Dimensions, Size, Date, Unordered = -1 };
	static std::pair<ImagesColumn, bool> ToListSortOrder( fattr::Order fileOrder );		// <sortByColumn, sortAscending>

	void SetupFoundImagesListView( void );

	bool SearchSourceFiles( void );
	void UpdateCurrentFile( void );
	int GetCheckStateAutoRegen( void ) const;
	bool IsNewFilePath( const fs::CPath& filePath ) const { return m_origFilePaths.find( filePath ) == m_origFilePaths.end(); }
private:
	CAlbumModel m_model;					// works on a copy-by-value of the entire model (so that all editing happens in isolation from the document's album model)
	const CFileAttr* m_pCaretFileAttr;
	bool m_isDirty;

	std::unordered_set<fs::CPath> m_origFilePaths;	// both patterns + found images (for highlighting new patterns/found images)
private:
	// enum { IDD = IDD_ALBUM_SETTINGS_DIALOG };

	TDragPathItemListCtrl m_patternsListCtrl;
	CDialogToolBar m_patternsToolbar;

	CSpinEdit m_maxFileCountEdit;
	CSpinEdit m_minSizeEdit;
	CSpinEdit m_maxSizeEdit;

	TDragPathItemListCtrl m_imagesListCtrl;
	CDialogToolBar m_imagesToolbar;
	CEnumComboBox m_sortOrderCombo;

	CThumbPreviewCtrl m_thumbPreviewCtrl;
	CRegularStatic m_docVersionLabel;
	CRegularStatic m_docVersionStatic;
	CIconButton m_okButton;					// overloaded text/icon, depending on dirtyness

	std::auto_ptr<CListCtrlEditorFrame> m_pPatternsEditor;		// lazy instantiation, after lists & toolbars are created
	std::auto_ptr<CListCtrlEditorFrame> m_pImagesEditor;

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	virtual void OnOK( void );
	virtual void OnIdleUpdateControls( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );

	afx_msg void OnLVnDblClk_Patterns( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnDropFiles_Patterns( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemsRemoved_Patterns( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemsReorder_Patterns( void );
	afx_msg void On_AddSearchPattern( void );
	afx_msg void On_ModifySearchPattern( void );
	afx_msg void OnSearchSourceFiles( void );

	afx_msg void OnToggle_MaxFileCount( void );
	afx_msg void OnToggle_MinSize( void );
	afx_msg void OnToggle_MaxSize( void );
	afx_msg void OnEnChange_MaxFileCount( void );
	afx_msg void OnEnChange_MinMaxSize( void );
	afx_msg void OnToggle_AutoRegenerate( void );
	afx_msg void OnToggle_AutoDrop( void );

	afx_msg void OnLVnColumnClick_FoundImages( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemChanged_FoundImages( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnGetDispInfo_FoundImages( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLVnItemsReorder_FoundImages( void );
	afx_msg void OnStnDblClk_ThumbPreviewStatic( void );

	afx_msg void OnImageOrder( UINT cmdId );
	afx_msg void OnUpdateImageOrder( CCmdUI* pCmdUI );
	afx_msg void OnCBnSelChange_ImageOrder( void );

	afx_msg void On_ImageOpen( void );
	afx_msg void On_ImageSaveAs( void );
	afx_msg void On_ImageRemove( void );
	afx_msg void On_ImageExplore( void );
	afx_msg void OnUpdate_ImageFileOp( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumSettingsDialog_h
