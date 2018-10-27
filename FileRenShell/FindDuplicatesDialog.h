#ifndef FindDuplicatesDialog_h
#define FindDuplicatesDialog_h
#pragma once

#include "utl/FileState.h"
#include "utl/ISubject.h"
#include "utl/ReportListControl.h"
#include "utl/DateTimeControl.h"
#include "FileEditorBaseDialog.h"


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

	CDuplicateFileItem* FindItemWithKey( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	enum FileType { All, Images, Audio, Video, Custom };
	static const CEnumTags& GetTags_FileType( void );
private:
	std::vector< fs::CPath > m_sourcePaths;
	std::vector< CDuplicateFilesGroup* > m_duplicateGroups;

	bool m_anyChanges;
private:
	// enum { IDD = IDD_FIND_DUPLICATES_DIALOG };
	enum DupFileColumn { FileName, DirPath, Size, CRC32 };

	CReportListControl m_srcPathsListCtrl;
	CReportListControl m_dupsListCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnUpdateUndoRedo( CCmdUI* pCmdUI );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnBnClicked_PasteDestStates( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnUpdateSelListItem( CCmdUI* pCmdUI );
	afx_msg void OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // FindDuplicatesDialog_h
