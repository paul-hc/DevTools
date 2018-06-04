#ifndef TouchFilesDialog_h
#define TouchFilesDialog_h
#pragma once

#include "utl/BaseMainDialog.h"
#include "utl/FileState.h"
#include "utl/ISubject.h"
#include "utl/ReportListControl.h"
#include "utl/DateTimeControl.h"
#include "FileCommands_fwd.h"


class CFileModel;
class CTouchItem;
class CEnumTags;

namespace app { enum DateTimeField; }

namespace multi
{
	class CDateTimeState;
	class CAttribCheckState;
}


class CTouchFilesDialog : public CBaseMainDialog
						, private utl::IObserver
						, private cmd::IErrorObserver
						, private CReportListControl::ITextEffectCallback
{
public:
	CTouchFilesDialog( CFileModel* pFileModel, CWnd* pParent );
	virtual ~CTouchFilesDialog();
private:
	void Construct( void );

	enum Mode { Uninit = -1, StoreMode, TouchMode, UndoRollbackMode };		// reflects the OK button label
	void SwitchMode( Mode mode );

	bool TouchFiles( void );
	void PostMakeDest( bool silent = false );
	void SetupDialog( void );

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

	// utl::IObserver interface
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// cmd::IErrorObserver interface
	virtual void ClearFileErrors( void );
	virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;
	virtual void ModifyDiffTextEffectAt( std::vector< ui::CTextEffect >& rMatchEffects, LPARAM rowKey, int subItem ) const;

	size_t FindItemPos( const fs::CPath& keyPath ) const;
	void MarkInvalidSrcItems( void );

	static const CEnumTags& GetTags_DateTimeField( void );
	static app::DateTimeField GetDateTimeField( UINT dtId );
private:
	CFileModel* m_pFileModel;
	const std::vector< CTouchItem* >& m_rTouchItems;
	std::vector< CTouchItem* > m_errorItems;
	Mode m_mode;
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
	CDateTimeControl m_modifiedDateCtrl, m_createdDateCtrl, m_accessedDateCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog( void );
protected:
	virtual void OnOK( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnFieldChanged( void );
	afx_msg void OnBnClicked_Undo( void );
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
