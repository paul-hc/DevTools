#ifndef TouchFilesDialog_h
#define TouchFilesDialog_h
#pragma once

#include "utl/FileState.h"
#include "utl/ISubject.h"
#include "utl/UI/DateTimeControl.h"
#include "utl/UI/FrameHostCtrl.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/SelectionData.h"
#include "utl/UI/TandemControls.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "FileEditorBaseDialog.h"


class CTouchItem;
class CEnumTags;

namespace multi
{
	class CDateTimeState;
	class CAttribCheckState;
}


class CTouchFilesDialog : public CFileEditorBaseDialog
	, private ui::ITextEffectCallback
{
public:
	CTouchFilesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CTouchFilesDialog();
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
	const std::vector<CTouchItem*>* GetCmdSelItems( void ) const;
	const std::vector<CTouchItem*>& GetTargetItems( void ) const;

	static const CEnumTags& GetTags_Mode( void );

	void Construct( void );

	bool TouchFiles( void );
	void SetupDialog( void );

	// data
	void AccumulateCommonStates( void );
	void AccumulateItemStates( const CTouchItem* pTouchItem );

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
	utl::ICommand* MakeChangeDestFileStatesCmd( void );
	bool VisibleAllSrcColumns( void ) const;

	CTouchItem* FindItemWithKey( const fs::CPath& keyPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	static fs::TimeField GetTimeFieldFromId( UINT dtCtrlId );
private:
	const std::vector<CTouchItem*>& m_rTouchItems;
	ui::CSelectionData<CTouchItem> m_selData;
	bool m_dirtyTouch;

	// multiple states accumulators for edit fields
	std::vector<multi::CDateTimeState> m_dateTimeStates;			// fs::_TimeFieldCount
	std::vector<multi::CAttribCheckState> m_attribCheckStates;		// attribute count: {READONLY, HIDDEN, etc}
private:
	// enum { IDD = IDD_TOUCH_FILES_DIALOG };
	enum Column
	{
		PathName,
		DestAttributes, DestModifyTime, DestCreationTime, DestAccessTime,
		SrcAttributes, SrcModifyTime, SrcCreationTime, SrcAccessTime
	};

	CReportListControl m_fileListCtrl;
	CDateTimeControl m_modifiedDateCtrl, m_createdDateCtrl, m_accessedDateCtrl;

	CLabelDivider m_filesLabelDivider;
	CFrameHostCtrl<CButton> m_targetSelItemsButton;
	CHostToolbarCtrl<CStatusStatic> m_fileStatsStatic;
	CHostToolbarCtrl<CTextEdit> m_currFolderEdit;

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
	afx_msg void OnBnClicked_PasteDestStates( void );
	afx_msg void OnBnClicked_ResetDestFiles( void );
	afx_msg void OnBnClicked_ShowSrcColumns( void );
	afx_msg void OnCopyDateField( UINT cmdId );

	afx_msg void OnPushDateField( UINT cmdId );
	afx_msg void OnPushAttributeFields( void );
	afx_msg void OnPushAllFields( void );
	afx_msg void OnUpdateListCaretItem( CCmdUI* pCmdUI );
	afx_msg void OnUpdateListSelection( CCmdUI* pCmdUI );
	afx_msg void OnToggle_Attribute( UINT checkId );
	afx_msg void OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCopyTableText_TouchList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnDtnDateTimeChange( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // TouchFilesDialog_h
