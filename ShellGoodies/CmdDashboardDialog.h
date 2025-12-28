#ifndef CmdDashboardDialog_h
#define CmdDashboardDialog_h
#pragma once

#include "utl/UI/DialogToolBar.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/ImageEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "utl/ICommand.h"
#include <deque>


class CFileModel;
class CCommandModel;
class CCommandItem;


class CCmdDashboardDialog : public CLayoutDialog
	, private ui::ITextEffectCallback
	, private utl::noncopyable
{
public:
	CCmdDashboardDialog( CFileModel* pFileModel, svc::StackType stackType, CWnd* pParent );
	~CCmdDashboardDialog();
private:
	static CCommandModel* GetCommandModel( void );
	const std::deque<utl::ICommand*>& GetStack( void ) const;
	void BuildCmdItems( void );

	void SetupCommandList( void );
	bool SelectCommandList( int selIndex );
	void UpdateSelCommand( void );

	const CCommandItem* GetSelCaretCmdItem( void ) const;
	void QuerySelectedCmds( std::vector<utl::ICommand*>& rSelCommands ) const;
	static bool IsSelContiguousToTop( const std::vector<int>& selIndexes );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
private:
	CFileModel* m_pFileModel;
	svc::ICommandService* m_pCmdSvc;
	svc::StackType m_stackType;

	std::vector<CCommandItem> m_cmdItems;			// proxy items inserted into the list control
	bool m_enableProperties;
private:
	// enum { IDD = IDD_CMD_DASHBOARD_DIALOG };

	CRegularStatic m_actionHistoryStatic;
	CDialogToolBar m_actionToolbar;
	CDialogToolBar m_cmdsToolbar;
	CReportListControl m_commandsList;
	CImageEdit m_cmdHeaderEdit;
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
	afx_msg void OnOptions( void );
	afx_msg void OnUpdateOptions( CCmdUI* pCmdUI );
	afx_msg BOOL OnCmdList_SelectAll( UINT cmdId );
	afx_msg void OnCmdList_SelectToTop( void );
	afx_msg void OnUpdateCmdList_SelectToTop( CCmdUI* pCmdUI );
	afx_msg void OnCmdList_Delete( void );
	afx_msg void OnUpdateCmdList_Delete( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // CmdDashboardDialog_h
