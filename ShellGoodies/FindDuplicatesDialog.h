#ifndef FindDuplicatesDialog_h
#define FindDuplicatesDialog_h
#pragma once

#include "utl/ISubject.h"
#include "utl/FileSystem_fwd.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "FileEditorBaseDialog.h"


class CPathItem;
class CDuplicateFileItem;
class CDuplicateFilesGroup;
class CDuplicateGroupStore;
class CEnumTags;
struct CDupsOutcome;
namespace utl { interface ISubject; }
namespace fs { interface IEnumerator; }


class CFindDuplicatesDialog : public CFileEditorBaseDialog
							, private ui::ITextEffectCallback
{
public:
	CFindDuplicatesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CFindDuplicatesDialog();
protected:
	// IFileEditor interface
	virtual void PostMakeDest( bool silent = false );
	virtual void PopStackTop( svc::StackType stackType );

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;

	virtual void SwitchMode( Mode mode );
private:
	bool DeleteDuplicateFiles( void );
	bool MoveDuplicateFiles( void );
	bool ExecuteDuplicatesCmd( utl::ICommand* pCmd );
	void SetupDialog( void );

	// data
	bool QueryCheckedDupFilePaths( std::vector< fs::CPath >& rDupFilePaths ) const;

	// output
	void SetupSrcPathsList( void );
	void SetupDuplicateFileList( void );

	// input
	void ClearDuplicates( void );
	bool SearchForDuplicateFiles( void );

	CDuplicateFileItem* FindItemWithKey( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	enum FileType { All, Images, Audio, Video, Custom };
	static const CEnumTags& GetTags_FileType( void );

	std::tstring FormatReport( const CDupsOutcome& outcome ) const;
	void DisplayCheckedGroupsInfo( void );

	// duplicates
	static CMenu& GetDupListPopupMenu( CReportListControl::ListPopup popupType );
	void ToggleCheckGroupDuplicates( unsigned int groupId );

	static pred::CompareResult CALLBACK CompareGroupFileName( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );
	static pred::CompareResult CALLBACK CompareGroupFolderPath( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );
	static pred::CompareResult CALLBACK CompareGroupFileSize( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );
	static pred::CompareResult CALLBACK CompareGroupFileCrc32( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );
	static pred::CompareResult CALLBACK CompareGroupDateModified( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );
	static pred::CompareResult CALLBACK CompareGroupDuplicateCount( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis );

	template< typename CompareGroupPtr >
	pred::CompareResult CompareGroupsBy( int leftGroupId, int rightGroupId, CompareGroupPtr compareGroup ) const;

	template< typename CompareItemPtr >
	pred::CompareResult CompareGroupsByItemField( int leftGroupId, int rightGroupId, CompareItemPtr compareItem ) const;
private:
	std::vector< CPathItem* > m_srcPathItems;
	std::vector< CDuplicateFilesGroup* > m_duplicateGroups;
	std::vector< std::tstring > m_fileTypeSpecs;
private:
	// enum { IDD = IDD_FIND_DUPLICATES_DIALOG };
	enum DupFileColumn { FileName, FolderPath, Size, Crc32, DateModified, DuplicateCount };
	enum SubPopup { DupListNowhere, DupListOnSelection };

	CPathItemListCtrl m_srcPathsListCtrl;
	CDialogToolBar m_srcPathsToolbar;
	CComboBox m_fileTypeCombo;
	CTextEdit m_fileSpecEdit;
	CHistoryComboBox m_minFileSizeCombo;

	CPathItemListCtrl m_dupsListCtrl;
	CDialogToolBar m_dupsToolbar;
	CStatusStatic m_outcomeStatic;
	CRegularStatic m_commitInfoStatic;

	bool m_highlightDuplicates;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	virtual void OnIdleUpdateControls( void );
	afx_msg void OnUpdateUndoRedo( CCmdUI* pCmdUI );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnEditSrcPaths( void );
	afx_msg void OnUpdateEditSrcPaths( CCmdUI* pCmdUI );
	afx_msg void OnCbnSelChange_FileType( void );
	afx_msg void OnEnChange_FileSpec( void );
	afx_msg void OnCbnChanged_MinFileSize( void );

	afx_msg void OnCheckAllDuplicates( UINT cmdId );
	afx_msg void OnUpdateCheckAllDuplicates( CCmdUI* pCmdUI );
	afx_msg void OnToggleCheckGroupDups( void );
	afx_msg void OnUpdateToggleCheckGroupDups( CCmdUI* pCmdUI );
	afx_msg void OnKeepAsOriginalFile( void );
	afx_msg void OnUpdateKeepAsOriginalFile( CCmdUI* pCmdUI );
	afx_msg void OnPickAsOriginalFolder( void );
	afx_msg void OnUpdatePickAsOriginalFolder( CCmdUI* pCmdUI );
	afx_msg void OnClearCrc32Cache( void );
	afx_msg void OnUpdateClearCrc32Cache( CCmdUI* pCmdUI );
	afx_msg void OnToggleHighlightDuplicates( void );
	afx_msg void OnUpdateHighlightDuplicates( CCmdUI* pCmdUI );

	afx_msg void OnBnClicked_DeleteDuplicates( void );
	afx_msg void OnBnClicked_MoveDuplicates( void );
	afx_msg void OnUpdateSelListItem( CCmdUI* pCmdUI );
	afx_msg void OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnLinkClick_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCheckStatesChanged_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCustomSortList_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // FindDuplicatesDialog_h