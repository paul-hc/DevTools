#ifndef FileEditorBaseDialog_h
#define FileEditorBaseDialog_h
#pragma once

#include "IFileEditor.h"
#include "utl/UI/BaseMainDialog.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/IconButton.h"


class CFileModel;
class CPathItemBase;
class CReportListControl;


abstract class CFileEditorBaseDialog : public CBaseMainDialog
	, public IFileEditor
{
protected:
	CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent );
	virtual ~CFileEditorBaseDialog();
public:
	bool IsErrorItem( const CPathItemBase* pItem ) const;

	template< typename ItemType >
	ItemType* GetFirstErrorItem( void ) const { return !m_errorItems.empty() ? checked_static_cast<ItemType*>( m_errorItems.front() ) : NULL; }

	bool SafeExecuteCmd( utl::ICommand* pCmd );

	// IFileEditor interface (partial)
	virtual CFileModel* GetFileModel( void ) const override;
	virtual CDialog* GetDialog( void ) override;
	virtual bool IsRollMode( void ) const override;
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	enum Mode					// determines the OK button label
	{
		EditMode,				// edit destinations
		CommitFilesMode,		// ready to apply destinations to target files
		RollBackMode,			// ready to undo the peeked files command
		RollForwardMode			// ready to redo the peeked files command
	};

	virtual void SwitchMode( Mode mode ) = 0;
	void UpdateOkButton( const std::tstring& caption, UINT iconId = 0 );
protected:
	int PopStackRunCrossEditor( svc::StackType stackType );

	bool IsNativeCmd( const utl::ICommand* pCmd ) const;
	bool IsForeignCmd( const utl::ICommand* pCmd ) const;		// must be handled by a different editor?
	utl::ICommand* PeekCmdForDialog( svc::StackType stackType ) const;

	enum Prompt { PromptClose, PromptNoFileChanges };
	bool PromptCloseDialog( Prompt prompt = PromptNoFileChanges );
protected:
	CFileModel* m_pFileModel;
	svc::ICommandService* m_pCmdSvc;
	std::vector< cmd::CommandType > m_nativeCmdTypes;		// the first one always identifies the editor
	Mode m_mode;

	std::vector< CPathItemBase* > m_errorItems;

	// controls
	CIconButton m_okButton;				// overloaded, does various things depending of the mode
	CDialogToolBar m_toolbar;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override;
protected:
	virtual void DoDataExchange( CDataExchange* pDX ) override;
protected:
	afx_msg void OnUndoRedo( UINT btnId );
	afx_msg void OnOptions( void );
	afx_msg void OnUpdateOptions( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FileEditorBaseDialog_h
