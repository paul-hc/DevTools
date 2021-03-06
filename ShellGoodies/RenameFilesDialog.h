#ifndef RenameFilesDialog_h
#define RenameFilesDialog_h
#pragma once

#include "utl/Path.h"
#include "utl/PathFormatter.h"
#include "utl/Subject.h"
#include "utl/UI/LayoutChildPropertySheet.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/EnumSplitButton.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/SplitPushButton.h"
#include "utl/UI/SpinEdit.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "Application_fwd.h"
#include "FileEditorBaseDialog.h"


class CRenameItem;
class CDisplayFilenameAdapter;
class CRenameService;
class CPickDataset;


class CRenameFilesDialog : public CFileEditorBaseDialog
{
public:
	CRenameFilesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CRenameFilesDialog();

	const std::vector< CRenameItem* >& GetRenameItems( void ) const { return m_rRenameItems; }

	bool IsInitialized( void ) const { return m_isInitialized; }
	bool HasDestPaths( void ) const;

	CDisplayFilenameAdapter* GetDisplayFilenameAdapter( void ) { return m_pDisplayFilenameAdapter.get(); }
protected:
	// IFileEditor interface (partial)
	virtual void PostMakeDest( bool silent = false );
	virtual void PopStackTop( svc::StackType stackType );

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	virtual void SwitchMode( Mode mode );
private:
	void CommitLocalEdits( void );

	void AutoGenerateFiles( void );
	bool RenameFiles( void );
	bool ChangeSeqCount( UINT seqCount );

	CRenameItem* FindItemWithKey( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	std::tstring JoinErrorDestPaths( void ) const;
	std::tstring GetSelFindWhat( void ) const;

	CPathFormatter InputRenameFormatter( bool checkConsistent ) const;

	bool GenerateDestPaths( const CPathFormatter& pathFormatter, UINT* pSeqCount );
	void ReplaceFormatEditText( const std::tstring& text );
private:
	const std::vector< CRenameItem* >& m_rRenameItems;
	std::auto_ptr< CRenameService > m_pRenSvc;
	bool m_isInitialized;

	bool m_autoGenerate;
	bool m_seqCountAutoAdvance;
	bool m_ignoreExtension;				// "Show Extension" checkbox has inverted logic

	std::auto_ptr< CDisplayFilenameAdapter > m_pDisplayFilenameAdapter;
	std::auto_ptr< CPickDataset > m_pPickDataset;
private:
	// enum { IDD = IDD_RENAME_FILES_DIALOG };
	enum Page { ListPage, EditPage };

	CLayoutChildPropertySheet m_filesSheet;

	CHistoryComboBox m_formatCombo;
	CDialogToolBar m_formatToolbar;
	CSpinEdit m_seqCountEdit;
	CDialogToolBar m_seqCountToolbar;
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
	afx_msg void OnUpdateUndoRedo( CCmdUI* pCmdUI );
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
	afx_msg void OnToggle_ShowExtension( void );
	afx_msg void OnBnClicked_PasteDestFiles( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnBnClicked_CapitalizeDestFiles( void );
	afx_msg void OnSbnRightClicked_CapitalizeOptions( void );
	afx_msg void OnBnClicked_ChangeCase( void );
	afx_msg void OnBnClicked_ReplaceDestFiles( void );
	afx_msg void OnBnClicked_ReplaceAllDelimitersDestFiles( void );
	afx_msg void OnPickFormatToken( void );
	afx_msg void OnPickDirPath( void );
	afx_msg void OnDirPathPicked( UINT cmdId );
	afx_msg void OnPickTextTools( void );
	afx_msg void OnFormatTextToolPicked( UINT menuId );
	afx_msg void OnToggleAutoGenerate( void );
	afx_msg void OnUpdateAutoGenerate( CCmdUI* pCmdUI );
	afx_msg void OnNumericSequence( UINT cmdId );
	afx_msg void OnBnClicked_PickRenameActions( void );
	afx_msg void OnChangeDestPathsTool( UINT menuId );
	afx_msg void OnEnsureUniformNumPadding( void );
	afx_msg void OnGenerateNumericSequence( void );

	DECLARE_MESSAGE_MAP()
};


#endif // RenameFilesDialog_h
