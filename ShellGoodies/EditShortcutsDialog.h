#ifndef EditShortcutsDialog_h
#define EditShortcutsDialog_h
#pragma once

#include "utl/ISubject.h"
#include "utl/UI/FrameHostCtrl.h"
#include "utl/UI/PathItemEdit.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/SelectionData.h"
#include "utl/UI/TandemControls.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "FileEditorBaseDialog.h"


class CEditLinkItem;
class CEnumTags;


class CEditShortcutsDialog : public CFileEditorBaseDialog
	, private ui::ITextEffectCallback
{
public:
	CEditShortcutsDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CEditShortcutsDialog();
protected:
	// IFileEditor interface
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

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override;
	virtual void ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const override;

	virtual void SwitchMode( Mode mode ) override;
private:
	const std::vector<CEditLinkItem*>* GetCmdSelItems( void ) const;
	const std::vector<CEditLinkItem*>& GetTargetItems( void ) const;

	static const CEnumTags& GetTags_Mode( void );

	bool ModifyShortcuts( void );
	void SetupDialog( void );

	// output
	void UpdateTargetScopeButton( void );
	void UpdateFileListStatus( void );

	void SetupFileListView( void );
	void UpdateFileListViewSelItems( void );

	void UpdateFieldControls( void );
	void UpdateFieldsFromCaretItem();

	// input
	void InputFields( void );
	void ApplyFields( void );
	utl::ICommand* MakeChangeDestShortcutsCmd( void );
	bool VisibleAllSrcColumns( void ) const;

	CEditLinkItem* FindItemWithKey( const fs::CPath& keyPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	//static fs::TimeField GetTimeFieldFromId( UINT dtCtrlId );
private:
	const std::vector<CEditLinkItem*>& m_rEditLinkItems;
	ui::CSelectionData<CEditLinkItem> m_selData;
	bool m_dirty;
private:
	// enum { IDD = IDD_EDIT_SHORTCUTS_DIALOG };
	enum Column
	{
		LinkName,
		D_Target, D_Folder, D_Arguments, D_IconLocation, D_HotKey, D_ShowCmd, D_Description,
		S_Target, S_Folder, S_Arguments, S_IconLocation, S_HotKey, S_ShowCmd, S_Description
	};

	CPathItemListCtrl m_fileListCtrl;

	CLabelDivider m_filesLabelDivider;
	CFrameHostCtrl<CButton> m_targetSelItemsButton;
	CHostToolbarCtrl<CStatusStatic> m_fileStatsStatic;
	CHostToolbarCtrl<CPathItemEdit> m_currFolderEdit;

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
	afx_msg void OnToggle_TargetSelItems( void );
	afx_msg void On_SelItems_ResetDestFile( void );

	afx_msg void OnBnClicked_CopySourceFiles( void );
	afx_msg void OnBnClicked_PasteDestShortcuts( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnBnClicked_ShowSrcColumns( void );

	afx_msg void OnUpdateListCaretItem( CCmdUI* pCmdUI );
	afx_msg void OnUpdateListSelection( CCmdUI* pCmdUI );
	afx_msg void OnLvnItemChanged_LinkList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCopyTableText_LinkList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // EditShortcutsDialog_h
