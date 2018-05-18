#ifndef MainRenameDialog_h
#define MainRenameDialog_h
#pragma once

#include "utl/BatchTransactions.h"
#include "utl/BaseMainDialog.h"
#include "utl/DialogToolBar.h"
#include "utl/EnumSplitButton.h"
#include "utl/HistoryComboBox.h"
#include "utl/Logger.h"
#include "utl/Path.h"
#include "utl/ReportListControl.h"
#include "utl/SplitPushButton.h"
#include "utl/SpinEdit.h"
#include "utl/TextEdit.h"
#include "utl/ThemeStatic.h"
#include "Application_fwd.h"


class CFileWorkingSet;
namespace str { enum Match; }

namespace popup
{
	enum PopupBar { FormatPicker, MoreRenameActions, TextTools };
}


class CMainRenameDialog : public CBaseMainDialog
						, private fs::IBatchTransactionCallback
{
public:
	CMainRenameDialog( app::MenuCommand menuCmd, CFileWorkingSet* pFileData, CWnd* pParent );
	virtual ~CMainRenameDialog();

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	CFileWorkingSet* GetWorkingSet( void ) const { return m_pFileData; }
	void PostMakeDest( bool silent = false );
private:
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

	// custom draw list
	void ListItem_DrawTextDiffs( const NMLVCUSTOMDRAW* pCustomDraw );
	void ListItem_DrawTextDiffs( CDC* pDC, const CRect& textRect, int itemIndex, Column column );
	bool ListItem_FillBkgnd( NMLVCUSTOMDRAW* pCustomDraw ) const;

	bool ListItem_IsSelectedInvert( int itemIndex ) const;

	static CRect MakeItemTextRect( const NMLVCUSTOMDRAW* pCustomDraw );
	static CFont* SelectBoldFont( CDC* pDC );
private:
	struct CDisplayItem
	{
		CDisplayItem( const fs::CPath& srcPath, const fs::CPath& destPath )
			: m_srcPath( srcPath )
			, m_srcFnameExt( m_srcPath.GetNameExt() )
			, m_destFnameExt( destPath.GetNameExt() )
			, m_match( path::GetMatch()( m_srcFnameExt.c_str(), m_destFnameExt.c_str() ) )
		{
			ComputeMatchSeq();
		}

		bool HasMisMatch( void ) const { return !m_destFnameExt.empty() && m_match != str::MatchEqual; }
	private:
		void ComputeMatchSeq( void );
	public:
		const fs::CPath m_srcPath;
		std::tstring m_srcFnameExt;
		std::tstring m_destFnameExt;
		str::Match m_match;
		std::vector< str::Match > m_srcMatchSeq;		// same size with m_srcFnameExt
		std::vector< str::Match > m_destMatchSeq;		// same size with m_destFnameExt
	};
private:
	app::MenuCommand m_menuCmd;
	CFileWorkingSet* m_pFileData;
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
	CReportListControl m_fileListView;
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
	afx_msg void OnCustomDrawFileRenameList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // MainRenameDialog_h
