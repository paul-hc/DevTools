#ifndef TouchFilesDialog_h
#define TouchFilesDialog_h
#pragma once

#include "utl/BatchTransactions.h"
#include "utl/BaseMainDialog.h"
//#include "utl/DialogToolBar.h"
#include "utl/FileState.h"
#include "utl/ReportListControl.h"
#include "utl/DateTimeControl.h"
#include "Application_fwd.h"
#include "FileWorkingSet_fwd.h"


class CLogger;
class CFileWorkingSet;
class CTouchItem;

namespace multi
{
	class CDateTimeState;
	class CAttribCheckState;
}


class CTouchFilesDialog : public CBaseMainDialog
						, private fs::IBatchTransactionCallback
						, private CReportListControl::ITextEffectCallback
{
public:
	CTouchFilesDialog( CFileWorkingSet* pFileData, CWnd* pParent );
	virtual ~CTouchFilesDialog();
private:
	void Construct( void );

	CFileWorkingSet* GetWorkingSet( void ) const { return m_pFileData; }
	void PostMakeDest( bool silent = false );

	enum Mode { Uninit = -1, ApplyFieldsMode, TouchMode, UndoRollbackMode };		// reflects the OK button label
	void SwitchMode( Mode mode );

	bool TouchFiles( void );

	// data
	void AccumulateCommonStates( void );
	void AccumulateItemStates( const CTouchItem* pTouchItem );

	// output
	void SetupFileListView( void );
	void UpdateFileListViewDest( void );

	void UpdateFieldControls( void );
	void UpdateFieldsFromSel( int selIndex );

	// input
	void InputFields( void );
	void ApplyFields( void );
	bool VisibleAllSrcColumns( void ) const;

	int FindItemPos( const fs::CPath& keyPath ) const;

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
	bool m_anyChanges;

	// multiple states accumulators for edit fields
	std::vector< multi::CDateTimeState > m_dateTimeStates;
	std::vector< multi::CAttribCheckState > m_attribCheckStates;
private:
	// enum { IDD = IDD_TOUCH_FILES_DIALOG };
	enum Column
	{
		PathName,
		DestAttributes, DestModifyTime, DestCreationTime, DestAccessTime,
		SrcAttributes, SrcModifyTime, SrcCreationTime, SrcAccessTime
	};

	CReportListControl m_fileListCtrl;
	CDateTimeControl m_dateTimeCtrls[ app::_DateTimeFieldCount ];

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog( void );
protected:
	virtual void OnOK( void );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnBnClicked_Undo( void );
	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnBnClicked_PasteDestStates( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnBnClicked_ShowSrcColumns( void );
	afx_msg void OnToggle_Attribute( UINT checkId );
	afx_msg void OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnDtnDateTimeChange_DateTimeCtrl( UINT dtId, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // TouchFilesDialog_h
