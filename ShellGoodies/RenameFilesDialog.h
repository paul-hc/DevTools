#ifndef RenameFilesDialog_h
#define RenameFilesDialog_h
#pragma once

#include "utl/Path.h"
#include "utl/PathFormatter.h"
#include "utl/Subject.h"
#include "utl/UI/LayoutChildPropertySheet.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/EnumSplitButton.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/SplitPushButton.h"
#include "utl/UI/SpinEdit.h"
#include "utl/UI/SelectionData.h"
#include "utl/UI/TandemControls.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "Application_fwd.h"
#include "FileEditorBaseDialog.h"


class CRenameItem;
class CDisplayFilenameAdapter;
class CRenameService;
class CPickDataset;
class CEnumTags;


class CRenameFilesDialog : public CFileEditorBaseDialog
{
public:
	CRenameFilesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CRenameFilesDialog();

	bool IsInitialized( void ) const { return m_isInitialized; }
	bool HasDestPaths( void ) const;

	CDisplayFilenameAdapter* GetDisplayFilenameAdapter( void ) { return m_pDisplayFilenameAdapter.get(); }
	const ui::CSelectionData<CRenameItem>& GetSelData( void ) const { return m_selData; }
	ui::CSelectionData<CRenameItem>& RefSelData( void ) { return m_selData; }

	const std::vector<CRenameItem*>& GetRenameItems( void ) const { return m_rRenameItems; }
protected:
	const std::vector<CRenameItem*>* GetCmdSelItems( void ) const;
	const std::vector<CRenameItem*>& GetTargetItems( void ) const;

	// IFileEditor interface (partial)
	virtual void PostMakeDest( bool silent = false ) override;
	virtual void PopStackTop( svc::StackType stackType ) override;
	virtual void OnExecuteCmd( utl::ICommand* pCmd ) override;

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void ) override;
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) override;

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	virtual void SwitchMode( Mode mode ) override;
private:
	static const CEnumTags& GetTags_Mode( void );

	void UpdateFormatLabel( void );
	void UpdateSortOrderCombo( const ren::TSortingPair& sorting );
	void UpdateTargetScopeButton( void );
	void UpdateFileListStatus( void );
	bool PromptRenameCustomSortOrder( void ) const;

	void CommitLocalEdits( void );

	void AutoGenerateFiles( void );
	bool RenameFiles( void );
	bool ChangeSeqCount( UINT seqCount );

	CRenameItem* FindItemWithKey( const fs::CPath& srcPath ) const;
	void MarkInvalidSrcItems( void );
	std::tstring JoinErrorDestPaths( void ) const;
	std::tstring GetSelFindWhat( void ) const;
	bool SetCaretOnItem( const CRenameItem* pRenameItem );

	CPathFormatter InputRenameFormatter( bool checkConsistent ) const;
	bool IsFormatExtConsistent( void ) const;

	bool GenerateDestPaths( const CPathFormatter& pathFormatter, IN OUT UINT* pSeqCount );
	void ReplaceFormatEditText( const std::tstring& text );
private:
	const std::vector<CRenameItem*>& m_rRenameItems;
	std::auto_ptr<CRenameService> m_pRenSvc;
	bool m_isInitialized;

	persist bool m_autoGenerate;
	persist bool m_seqCountAutoAdvance;
	persist bool m_ignoreExtension;				// "Show Extension" checkbox has inverted logic
	persist UINT m_prevGenSeqCount;				// used in certain cases to roll-back sequence advance on generation

	std::auto_ptr<CDisplayFilenameAdapter> m_pDisplayFilenameAdapter;
	std::auto_ptr<CPickDataset> m_pPickDataset;

	ui::CSelectionData<CRenameItem> m_selData;
private:
	// enum { IDD = IDD_RENAME_FILES_DIALOG };
	enum Page { ListSimplePage, ListDetailsPage, EditPage };

	CLayoutChildPropertySheet m_filesSheet;

	persist CHostToolbarCtrl<CHistoryComboBox> m_formatCombo;
	CSpinEdit m_seqCountEdit;
	CDialogToolBar m_seqCountToolbar;
	CLabelDivider m_filesLabelDivider;
	CHostToolbarCtrl<CStatusStatic> m_fileStatsStatic;
	CHostToolbarCtrl<CTextEdit> m_currFolderEdit;
	CFrameHostCtrl<CButton> m_showExtButton;
	CFrameHostCtrl<CEnumComboBox> m_sortOrderCombo;
	CFrameHostCtrl<CButton> m_targetSelItemsButton;
	CSplitPushButton m_capitalizeButton;
	persist CEnumSplitButton m_changeCaseButton;
	persist CHistoryComboBox m_delimiterSetCombo;
	persist CTextEdit m_newDelimiterEdit;
	CThemeStatic m_delimStatic;
	CPickMenuStatic m_pickRenameActionsStatic;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX ) override;
	virtual BOOL OnInitDialog( void ) override;
protected:
	virtual void OnOK( void ) override;
	afx_msg void OnDestroy( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType );
	afx_msg void OnUpdateUndoRedo( CCmdUI* pCmdUI );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnChanged_Format( void );
	afx_msg void OnEnChange_SeqCounter( void );
	afx_msg void OnSeqCountReset( void );
	afx_msg void OnUpdateSeqCountReset( CCmdUI* pCmdUI );
	afx_msg void OnSeqCountFindNext( void );
	afx_msg void OnUpdateSeqCountFindNext( CCmdUI* pCmdUI );
	afx_msg void OnToggle_SeqCountAutoAdvance( void );
	afx_msg void OnUpdate_SeqCountAutoAdvance( CCmdUI* pCmdUI );
	afx_msg void OnToggle_ShowExtension( void );
	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnCbnSelChange_SortOrder( void );
	afx_msg void OnToggle_TargetSelItems( void );
	afx_msg void On_SelItems_ResetDestFile( void );
	afx_msg void OnUpdateListSelection( CCmdUI* pCmdUI );

	afx_msg void OnBnClicked_PasteDestFiles( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnUpdate_ResetDestFiles( CCmdUI* pCmdUI );
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
	afx_msg void OnGenerateNow( void );
	afx_msg void OnUpdateGenerateNow( CCmdUI* pCmdUI );
	afx_msg void OnToggle_AutoGenerate( void );
	afx_msg void OnUpdate_AutoGenerate( CCmdUI* pCmdUI );
	afx_msg void OnNumericSequence( UINT cmdId );
	afx_msg void OnBnClicked_PickRenameActions( void );
	afx_msg void OnChangeDestPathsTool( UINT menuId );
	afx_msg void OnEnsureUniformNumPadding( void );
	afx_msg void OnGenerateNumericSequence( void );

	afx_msg void On_BrowseCurrFolder( void );
	afx_msg void OnUpdate_BrowseCurrFolder( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // RenameFilesDialog_h
