#ifndef MainRenameDialog_h
#define MainRenameDialog_h
#pragma once

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
#include "Application_fwd.h"
#include "FileEditorBaseDialog.h"


class CRenameItem;
class CRenameService;
class CPickDataset;


class CMainRenameDialog : public CFileEditorBaseDialog
						, private CReportListControl::ITextEffectCallback
{
public:
	CMainRenameDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CMainRenameDialog();
private:
	// IFileEditor interface (partial)
	virtual void PostMakeDest( bool silent = false );
	virtual void PopStackTop( cmd::StackType stackType );

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	void SetupFileListView( void );

	enum Mode { MakeMode, RenameMode, RollBackMode, RollForwardMode };		// same as OK button label
	void SwitchMode( Mode mode );

	void AutoGenerateFiles( void );
	bool RenameFiles( void );
	bool ChangeSeqCount( UINT seqCount );

	enum Column { Source, Destination };

	size_t FindItemPos( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	std::tstring JoinErrorDestPaths( void ) const;

	bool GenerateDestPaths( const std::tstring& format, UINT* pSeqCount );
	void EnsureUniformNumPadding( void );
	void ReplaceFormatEditText( const std::tstring& text );
private:
	const std::vector< CRenameItem* >& m_rRenameItems;
	std::auto_ptr< CRenameService > m_pRenSvc;

	Mode m_mode;
	bool m_autoGenerate;
	bool m_seqCountAutoAdvance;

	std::auto_ptr< CPickDataset > m_pPickDataset;
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
