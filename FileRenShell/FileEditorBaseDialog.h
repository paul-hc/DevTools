#ifndef FileEditorBaseDialog_h
#define FileEditorBaseDialog_h
#pragma once

#include "IFileEditor.h"
#include "utl/BaseMainDialog.h"
#include "utl/DialogToolBar.h"
#include "utl/ReportListControl.h"


class CFileModel;
class CPathItemBase;


abstract class CFileEditorBaseDialog : public CBaseMainDialog
									 , public IFileEditor
{
protected:
	CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent );
	virtual ~CFileEditorBaseDialog();

	// IFileEditor interface (partial)
	virtual CFileModel* GetFileModel( void ) const;
	virtual CDialog* GetDialog( void );
	virtual bool IsRollMode( void ) const;

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	bool SafeExecuteCmd( utl::ICommand* pCmd );
	int PopStackRunCrossEditor( cmd::StackType stackType );
protected:
	enum Mode					// determines the OK button label
	{
		EditMode,				// edit destinations
		CommitFilesMode,		// ready to apply destinations to target files
		RollBackMode,			// ready to undo the peeked files command
		RollForwardMode			// ready to redo the peeked files command
	};

	bool IsNativeCmd( const utl::ICommand* pCmd ) const;
	bool IsForeignCmd( const utl::ICommand* pCmd ) const;		// must be handled by a different editor?
	utl::ICommand* PeekCmdForDialog( cmd::StackType stackType ) const;
	int EnsureVisibleFirstError( CReportListControl* pFileListCtrl ) const;

	enum Prompt { PromptClose, PromptNoFileChanges };
	bool PromptCloseDialog( Prompt prompt = PromptNoFileChanges );
protected:
	CFileModel* m_pFileModel;
	std::vector< cmd::CommandType > m_nativeCmdTypes;		// the first one always identifies the editor
	Mode m_mode;

	std::vector< CPathItemBase* > m_errorItems;

	// controls
	CDialogToolBar m_toolbar;

	// generated stuff
	public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnUndoRedo( UINT btnId );
	afx_msg void OnOptions( void );
	afx_msg void OnUpdateOptions( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FileEditorBaseDialog_h
