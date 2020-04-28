#ifndef ArchiveImagesDialog_h
#define ArchiveImagesDialog_h
#pragma once

#include "ArchiveImagesContext.h"
#include "ListViewState.h"
#include "utl/Path.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ReportListControl.h"
#include <vector>


class CAlbumModel;


class CArchiveImagesDialog : public CLayoutDialog
{
public:
	CArchiveImagesDialog( const CAlbumModel* pModel, const std::tstring& srcDocPath, CWnd* pParent = NULL );
	virtual ~CArchiveImagesDialog();

	void StoreSelection( const CListViewState& lvState ) { m_lvState = lvState; }
private:
	// file rename/copy engine
	bool FetchFileContext( void );
	bool SetDefaultDestPath( void );

	// operations
	void SetupFilesView( bool firstTimeInit = false );
	void UpdateDirty( void );
	void SetDirty( bool _isDirty = true );
	bool CheckDestFolder( void );
	bool GenerateDestFiles( void );
	bool CommitFileOperation( void );

	void UpdateTargetFileCountStatic( void );
private:
	const CAlbumModel* m_pModel;
	fs::CPath m_srcDocPath;					// source document file path
public:
	CArchiveImagesContext m_filesContext;
	FileOp m_fileOp;
	DestType m_destType;
	fs::CPath m_destPath;					// either a destination directory or compound file
private:
	CListViewState m_lvState;
	int m_seqCounter;
	bool m_destOnSelection;					// if true destination m_filesContext is built on selection, otherwise on all files
	bool m_inInit;
	bool m_dirty;
	CFont m_largeBoldFont;

	static const TCHAR s_archiveFnameSuffix[];		// default fname.ext for an image compound file
private:
	// enum { IDD = IDD_ARCHIVE_IMAGES_DIALOG };
	enum Column { SrcFolder, SrcFilename, DestFilename };

	CStatic m_targetFileCountStatic;
	CReportListControl m_filesListCtrl;
	CHistoryComboBox m_formatCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	virtual void OnOK( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void OnCBnChange_Format( void );
	afx_msg void OnEnChangeCounter( void );
	afx_msg void OnDestroy( void );
	afx_msg void CmBrowseDestPath( void );
	afx_msg void OnToggleCopyFilesRadio( void );
	afx_msg void OnToggleRenameFilesRadio( void );
	afx_msg void OnEnChangeDestFolder( void );
	afx_msg void OnBnClicked_CreateDestFolder( void );
	afx_msg void OnToggleToFolderRadio( void );
	afx_msg void OnToggleToCompoundFileRadio( void );
	afx_msg void OnToggleTargetSelectedFilesRadio( void );
	afx_msg void OnToggleTargetAllFilesRadio( void );
	afx_msg void LVnItemChangedFilePathsList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void CmResetNumCounter( void );
	afx_msg void OnEditArchivePassword( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ArchiveImagesDialog_h
