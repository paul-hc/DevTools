#ifndef TouchFilesDialog_h
#define TouchFilesDialog_h
#pragma once

#include "utl/BatchTransactions.h"
#include "utl/BaseMainDialog.h"
//#include "utl/DialogToolBar.h"
#include "utl/FileState.h"
#include "utl/ReportListControl.h"
#include "utl/vector_map.h"
#include "Application_fwd.h"
#include "FileWorkingSet_fwd.h"


class CLogger;
class CFileWorkingSet;
class CTouchItem;


class CTouchFilesDialog : public CBaseMainDialog
						, private fs::IBatchTransactionCallback
						, private CReportListControl::ITextEffectCallback
{
public:
	CTouchFilesDialog( CFileWorkingSet* pFileData, CWnd* pParent );
	virtual ~CTouchFilesDialog();

	CFileWorkingSet* GetWorkingSet( void ) const { return m_pFileData; }
	void PostMakeDest( bool silent = false );
private:
	void SetupFileListView( void );
	int FindItemPos( const fs::CPath& keyPath ) const;

	enum Mode { Uninit = -1, TouchMode, UndoRollbackMode };		// reflects the OK button label
	void SwitchMode( Mode mode );

	bool TouchFiles( void );

	enum Column
	{
		Filename,
		DestAttributes, DestModifyTime, DestCreationTime, DestAccessTime,
		SrcAttributes, SrcModifyTime, SrcCreationTime, SrcAccessTime
	};

	// fs::IBatchTransactionCallback interface
	virtual CWnd* GetWnd( void );
	virtual CLogger* GetLogger( void );
	virtual fs::UserFeedback HandleFileError( const fs::CPath& sourcePath, const std::tstring& message );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, utl::ISubject* pSubject, int subItem ) const;
private:
	CFileWorkingSet* m_pFileData;
	const fmt::PathFormat m_pathFormat;
	Mode m_mode;
	std::vector< CTouchItem* > m_displayItems;
	std::auto_ptr< fs::CBatchTouch > m_pBatchTransaction;
private:
	// enum { IDD = IDD_TOUCH_FILES_DIALOG };
	CReportListControl m_fileListCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog( void );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnFieldChanged( void );

	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnBnClicked_PasteDestStates( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnBnClicked_Undo( void );

	DECLARE_MESSAGE_MAP()
};


#endif // TouchFilesDialog_h