#ifndef TouchFilesDialog_h
#define TouchFilesDialog_h
#pragma once

#include "utl/FileState.h"
#include "utl/ISubject.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/DateTimeControl.h"
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
	virtual void PostMakeDest( bool silent = false );
	virtual void PopStackTop( svc::StackType stackType );

	// utl::IObserver interface (via IFileEditor)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface (via IFileEditor)
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
	virtual void ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const;

	virtual void SwitchMode( Mode mode );
private:
	void Construct( void );

	bool TouchFiles( void );
	void SetupDialog( void );

	// data
	void AccumulateCommonStates( void );
	void AccumulateItemStates( const CTouchItem* pTouchItem );

	// output
	void SetupFileListView( void );

	void UpdateFieldControls( void );
	void UpdateFieldsFromSel( int selIndex );

	// input
	void InputFields( void );
	void ApplyFields( void );
	utl::ICommand* MakeChangeDestFileStatesCmd( void );
	bool VisibleAllSrcColumns( void ) const;

	CTouchItem* FindItemWithKey( const fs::CPath& keyPath ) const;
	void MarkInvalidSrcItems( void );
	void EnsureVisibleFirstError( void );

	static fs::TimeField GetTimeField( UINT dtId );
private:
	const std::vector<CTouchItem*>& m_rTouchItems;
	bool m_anyChanges;

	// multiple states accumulators for edit fields
	std::vector<multi::CDateTimeState> m_dateTimeStates;
	std::vector<multi::CAttribCheckState> m_attribCheckStates;
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
	afx_msg void OnBnClicked_ShowSrcColumns( void );
	afx_msg void OnCopyDateCell( UINT cmdId );
	afx_msg void OnPushToAttributeFields( void );
	afx_msg void OnPushToAllFields( void );
	afx_msg void OnUpdateSelListItem( CCmdUI* pCmdUI );
	afx_msg void OnToggle_Attribute( UINT checkId );
	afx_msg void OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnDtnDateTimeChange( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // TouchFilesDialog_h
