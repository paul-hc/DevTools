#ifndef CmdDashboardDialog_h
#define CmdDashboardDialog_h
#pragma once

#include "utl/UI/DialogToolBar.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "utl/ICommand.h"
#include <deque>


class CFileModel;
class CCommandModel;
class CCmdItem;


class CCmdDashboardDialog : public CLayoutDialog
						  , private ui::ITextEffectCallback
						  , private utl::noncopyable
{
public:
	CCmdDashboardDialog( CFileModel* pFileModel, svc::StackType stackType, CWnd* pParent );
	~CCmdDashboardDialog();
private:
	static CCommandModel* GetCommandModel( void );
	const std::deque< utl::ICommand* >& GetStack( void ) const;
	void BuildCmdItems( void );

	void SetupCommandList( void );
	bool SelectCommandList( int selIndex );
	void UpdateSelCommand( void );

	utl::ICommand* GetSelectedCmd( void ) const;
	void QuerySelectedCmds( std::vector< utl::ICommand* >& rSelCommands ) const;

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
private:
	CFileModel* m_pFileModel;
	svc::ICommandService* m_pCmdSvc;
	svc::StackType m_stackType;

	std::vector< CCmdItem > m_cmdItems;			// proxy items inserted into the list control
	bool m_enableProperties;
private:
	// enum { IDD = IDD_CMD_DASHBOARD_DIALOG };

	CRegularStatic m_actionHistoryStatic;
	CDialogToolBar m_actionToolbar;
	CDialogToolBar m_cmdsToolbar;
	CReportListControl m_commandsList;
	CTextEdit m_cmdHeaderEdit;
	CTextEdit m_cmdDetailsEdit;

	enum CmdColumn { CmdName, FileCount, Timestamp };
public:
	// generated overrides
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL OnInitDialog( void );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK( void );
protected:
	afx_msg void OnLvnItemChanged_CommandsList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnStackType( UINT cmdId );
	afx_msg void OnUpdateStackType( CCmdUI* pCmdUI );
	afx_msg BOOL OnSelCmds_SelectAll( UINT cmdId );
	afx_msg void OnSelCmds_Delete( void );
	afx_msg void OnUpdateSelCmds_Delete( CCmdUI* pCmdUI );
	afx_msg void OnOptions( void );
	afx_msg void OnUpdateOptions( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // CmdDashboardDialog_h
