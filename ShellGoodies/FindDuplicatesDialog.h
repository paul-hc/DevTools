#ifndef FindDuplicatesDialog_h
#define FindDuplicatesDialog_h
#pragma once

#include "utl/ISubject.h"
#include "utl/FileSystem_fwd.h"
#include "utl/UI/TandemControls.h"
#include "utl/UI/EnumComboBox.h"
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
	static void SetupPathsListItems( CPathItemListCtrl* pPathsListCtrl, const std::vector< CPathItem* >& pathItems );
	static bool InputNewPath( std::vector< CPathItem* >& rPathItems, CPathItem* pTargetItem, const fs::CPath& newPath );
	bool EditPathsListItems( CPathItemListCtrl* pPathsListCtrl, std::vector< CPathItem* >& rPathItems );
	void PushNewPathsListItems( CPathItemListCtrl* pPathsListCtrl, std::vector< CPathItem* >& rPathItems, const std::vector< fs::CPath >& newPaths, size_t atPos = utl::npos );

	void SetupDuplicateFileList( void );

	// input
	void ClearDuplicates( void );
	bool SearchForDuplicateFiles( void );

	CDuplicateFileItem* FindItemWithKey( const fs::CPath& keyPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	enum FileType { All, Images, Audio, Video, Custom };
	static const CEnumTags& GetTags_FileType( void );

	std::tstring FormatReport( const CDupsOutcome& outcome ) const;
	void DisplayCheckedGroupsInfo( void );

	// duplicates
	static CMenu& GetDupListPopupMenu( CReportListControl::ListPopup popupType );
	bool IsGroupFullyChecked( const CDuplicateFilesGroup* pGroup ) const;
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
	std::vector< CPathItem* > m_searchPathItems;
	std::vector< CPathItem* > m_ignorePathItems;
	std::vector< CDuplicateFilesGroup* > m_duplicateGroups;
	std::vector< std::tstring > m_fileTypeSpecs;
private:
	// enum { IDD = IDD_FIND_DUPLICATES_DIALOG };
	enum DupFileColumn { FileName, FolderPath, Size, Crc32, DateModified, DuplicateCount };
	enum SubPopup { DupListNowhere, DupListOnSelection };

	CHostToolbarCtrl<CPathItemListCtrl> m_searchPathsListCtrl;
	CHostToolbarCtrl<CPathItemListCtrl> m_ignorePathsListCtrl;
	CEnumComboBox m_fileTypeCombo;
	CTextEdit m_fileSpecEdit;
	CHistoryComboBox m_minFileSizeCombo;

	CHostToolbarCtrl<CPathItemListCtrl> m_dupsListCtrl;
	CStatusStatic m_outcomeStatic;
	CRegularStatic m_commitInfoStatic;
	CAccelTable m_accel;

	bool m_highlightDuplicates;
	static const ui::CItemContent s_pathItemsContent;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	virtual void OnIdleUpdateControls( void );
	afx_msg void OnUpdate_UndoRedo( CCmdUI* pCmdUI );
	afx_msg void OnFieldChanged( void );

	afx_msg void On_EditSearchPathsList( void );
	afx_msg void OnUpdate_EditSearchPathsList( CCmdUI* pCmdUI );
	afx_msg void On_EditIgnorePathsList( void );
	afx_msg void OnUpdate_EditIgnorePathsList( CCmdUI* pCmdUI );
	afx_msg void OnCbnSelChange_FileType( void );
	afx_msg void OnEnChange_FileSpec( void );
	afx_msg void OnCbnChanged_MinFileSize( void );

	afx_msg void On_CheckAllDuplicates( UINT cmdId );
	afx_msg void OnUpdate_CheckAllDuplicates( CCmdUI* pCmdUI );
	afx_msg void On_ToggleCheckGroupDups( void );
	afx_msg void OnUpdate_ToggleCheckGroupDups( CCmdUI* pCmdUI );
	afx_msg void On_KeepAsOriginalFile( void );
	afx_msg void OnUpdate_KeepAsOriginalFile( CCmdUI* pCmdUI );
	afx_msg void On_PickAsOriginalFolder( void );
	afx_msg void OnUpdate_PickAsOriginalFolder( CCmdUI* pCmdUI );
	afx_msg void On_DemoteAsDuplicateFolder( void );
	afx_msg void OnUpdate_DemoteAsDuplicateFolder( CCmdUI* pCmdUI );
	afx_msg void On_PushIgnoreFile( void );
	afx_msg void OnUpdate_PushIgnoreFile( CCmdUI* pCmdUI );
	afx_msg void On_PushIgnoreFolder( void );
	afx_msg void OnUpdate_PushIgnoreFolder( CCmdUI* pCmdUI );
	afx_msg void On_ClearCrc32Cache( void );
	afx_msg void OnUpdate_ClearCrc32Cache( CCmdUI* pCmdUI );
	afx_msg void On_ToggleHighlightDuplicates( void );
	afx_msg void OnUpdate_ToggleHighlightDuplicates( CCmdUI* pCmdUI );

	afx_msg void OnBnClicked_DeleteDuplicates( void );
	afx_msg void OnBnClicked_MoveDuplicates( void );
	afx_msg void OnUpdate_SelListItem( CCmdUI* pCmdUI );
	afx_msg void OnLvnDropFiles_PathsList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnEndLabelEdit_PathsList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnLinkClick_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCheckStatesChanged_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCustomSortList_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // FindDuplicatesDialog_h
