#ifndef FindDuplicatesDialog_h
#define FindDuplicatesDialog_h
#pragma once

#include "utl/FileState.h"
#include "utl/ISubject.h"
#include "utl/DialogToolBar.h"
#include "utl/HistoryComboBox.h"
#include "utl/ReportListControl.h"
#include "utl/TextEdit.h"
#include "FileEditorBaseDialog.h"


class CSrcPathItem;
class CDuplicateFilesGroup;
class CDuplicateFileItem;
class CEnumTags;

class CFindDuplicatesDialog : public CFileEditorBaseDialog
							, private CReportListControl::ITextEffectCallback
{
public:
	CFindDuplicatesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CFindDuplicatesDialog();
private:
	// IFileEditor interface
	virtual void PostMakeDest( bool silent = false );
	virtual void PopStackTop( cmd::StackType stackType );

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;
private:
	void SwitchMode( Mode mode );

	bool DeleteDuplicateFiles( void );
	void SetupDialog( void );

	// data

	// output
	void SetupSrcPathsList( void );
	void SetupDuplicateFileList( void );

	// input
	void ClearDuplicates( void );
	void SearchForDuplicateFiles( void );
	void SearchForFiles( std::vector< fs::CPath >& rFoundPaths ) const;

	CDuplicateFileItem* FindItemWithKey( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	enum FileType { All, Images, Audio, Video, Custom };
	static const CEnumTags& GetTags_FileType( void );
private:
	std::vector< CSrcPathItem* > m_srcPathItems;
	std::vector< CDuplicateFilesGroup* > m_duplicateGroups;
	std::vector< std::tstring > m_fileTypeSpecs;
private:
	// enum { IDD = IDD_FIND_DUPLICATES_DIALOG };
	enum DupFileColumn { FileName, DirPath, Size, Crc32, DateModified };

	CReportListControl m_srcPathsListCtrl;
	CReportListControl m_dupsListCtrl;
	CDialogToolBar m_srcPathsToolbar;
	CComboBox m_fileTypeCombo;
	CTextEdit m_fileSpecEdit;
	CHistoryComboBox m_minFileSizeCombo;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnUpdateUndoRedo( CCmdUI* pCmdUI );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnEditSrcPaths( void );
	afx_msg void OnUpdateEditSrcPaths( CCmdUI* pCmdUI );
	afx_msg void OnCbnSelChange_FileType( void );
	afx_msg void OnEnChange_FileSpec( void );
	afx_msg void OnCbnChanged_MinFileSize( void );
	afx_msg void OnBnClicked_CheckSelectDuplicates( void );
	afx_msg void OnBnClicked_DeleteDuplicates( void );
	afx_msg void OnBnClicked_MoveDuplicates( void );
	afx_msg void OnBnClicked_ClearCrc32Cache( void );
	afx_msg void OnUpdateSelListItem( CCmdUI* pCmdUI );
	afx_msg void OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // FindDuplicatesDialog_h
