#ifndef MainRenameDialog_h
#define MainRenameDialog_h
#pragma once

#include "utl/BatchTransactions.h"
#include "utl/BaseMainDialog.h"
#include "utl/DialogToolBar.h"
#include "utl/EnumSplitButton.h"
#include "utl/HistoryComboBox.h"
#include "utl/Subject.h"
#include "utl/Path.h"
#include "utl/ReportListControl.h"
#include "utl/SplitPushButton.h"
#include "utl/SpinEdit.h"
#include "utl/TextEdit.h"
#include "utl/ThemeStatic.h"
#include "FileWorkingSet_fwd.h"
#include "Application_fwd.h"


class CLogger;
namespace str { enum Match; }


class CMainRenameDialog : public CBaseMainDialog
						, private fs::IBatchTransactionCallback
						, private CReportListControl::ITextEffectCallback
{
public:
	CMainRenameDialog( app::MenuCommand menuCmd, CFileWorkingSet* pFileData, CWnd* pParent );
	virtual ~CMainRenameDialog();

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	CFileWorkingSet* GetWorkingSet( void ) const { return m_pFileData; }
	void PostMakeDest( bool silent = false );
private:
	void InitDisplayItems( void );

	void SetupFileListView( void );
	int FindItemPos( const fs::CPath& sourcePath ) const;

	enum Mode { Uninit = -1, MakeMode, RenameMode, UndoRollbackMode };		// same as OK button label
	void SwitchMode( Mode mode );

	void AutoGenerateFiles( void );
	bool RenameFiles( void );
	bool ChangeSeqCount( UINT seqCount );

	enum Column { Source, Destination };

	// fs::IBatchTransactionCallback interface
	virtual CWnd* GetWnd( void );
	virtual CLogger* GetLogger( void );
	virtual fs::UserFeedback HandleFileError( const fs::CPath& sourcePath, const std::tstring& message );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;
private:
	struct CDisplayItem : public CSubject
	{
		CDisplayItem( const TPathPair* pPathPair ) : m_pPathPair( pPathPair ) {}

		const fs::CPath& GetSrcPath( void ) const { return m_pPathPair->first; }		// AKA key path
		const fs::CPath& GetDestPath( void ) const { return m_pPathPair->second; }

		// utl::ISubject interface
		virtual std::tstring GetCode( void ) const;
		virtual std::tstring GetDisplayCode( void ) const;
	private:
		const TPathPair* m_pPathPair;
		const std::tstring m_srcFnameExt;
	};
private:
	CFileWorkingSet* m_pFileData;
	app::MenuCommand m_menuCmd;
	std::vector< CDisplayItem* > m_displayItems;
	bool m_autoGenerate;
	bool m_seqCountAutoAdvance;
	Mode m_mode;
	std::auto_ptr< fs::CBatchRename > m_pBatchTransaction;
private:
	// enum { IDD = IDD_RENAME_FILES_DIALOG };
	CHistoryComboBox m_formatCombo;
	CDialogToolBar m_formatToolbar;
	CSpinEdit m_seqCountEdit;
	CDialogToolBar m_seqCountToolbar;
	CReportListControl m_fileListCtrl;
	CSplitPushButton m_capitalizeButton;
	CEnumSplitButton m_changeCaseButton;
	CHistoryComboBox m_delimiterSetCombo;
	CTextEdit m_newDelimiterEdit;
	CThemeStatic m_delimStatic;
	CPickMenuStatic m_pickRenameActionsStatic;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog( void );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnChanged_Format( void );
	afx_msg void OnEnChange_SeqCounter( void );
	afx_msg void OnSeqCountReset( void );
	afx_msg void OnUpdateSeqCountReset( CCmdUI* pCmdUI );
	afx_msg void OnSeqCountFindNext( void );
	afx_msg void OnUpdateSeqCountFindNext( CCmdUI* pCmdUI );
	afx_msg void OnSeqCountAutoAdvance( void );
	afx_msg void OnUpdateSeqCountAutoAdvance( CCmdUI* pCmdUI );
	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnBnClicked_PasteDestFiles( void );
	afx_msg void OnBnClicked_ClearDestFiles( void );
	afx_msg void OnBnClicked_CapitalizeDestFiles( void );
	afx_msg void OnBnClicked_CapitalizeOptions( void );
	afx_msg void OnBnClicked_ChangeCase( void );
	afx_msg void OnBnClicked_ReplaceDestFiles( void );
	afx_msg void OnBnClicked_ReplaceAllDelimitersDestFiles( void );
	afx_msg void OnPickFormatToken( void );
	afx_msg void OnPickDirPath( void );
	afx_msg void OnDirPathPicked( UINT cmdId );
	afx_msg void OnPickTextTools( void );
	afx_msg void OnFormatTextToolPicked( UINT cmdId );
	afx_msg void OnToggleAutoGenerate( void );
	afx_msg void OnUpdateAutoGenerate( CCmdUI* pCmdUI );
	afx_msg void OnNumericSequence( UINT cmdId );
	afx_msg void OnBnClicked_Undo( void );
	afx_msg void OnBnClicked_PickRenameActions( void );
	afx_msg void OnSingleWhitespace( void );
	afx_msg void OnRemoveWhitespace( void );
	afx_msg void OnDashToSpace( void );
	afx_msg void OnSpaceToDash( void );
	afx_msg void OnUnderbarToSpace( void );
	afx_msg void OnSpaceToUnderbar( void );
	afx_msg void OnEnsureUniformNumPadding( void );

	DECLARE_MESSAGE_MAP()
};


#endif // MainRenameDialog_h
