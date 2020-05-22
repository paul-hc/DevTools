#ifndef ListCtrlEditorFrame_h
#define ListCtrlEditorFrame_h
#pragma once

#include "AccelTable.h"
#include "CtrlInterfaces.h"
#include "TooltipsHook.h"


class CReportListControl;


class CListCtrlEditorFrame : public CCmdTarget
						   , public CTooltipsHook			// provide tooltip messages for the toolbar
						   , public ui::ICommandFrame
{
public:
	CListCtrlEditorFrame( CReportListControl* pListCtrl, CToolBar* pToolbar );
	virtual ~CListCtrlEditorFrame();

	bool InInlineEditingMode( void ) const;

	// ui::ICommandFrame interface
	virtual CCmdTarget* GetCmdTarget( void );
	virtual bool HandleTranslateMessage( MSG* pMsg );
	virtual bool HandleCtrlCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
private:
	CReportListControl* m_pListCtrl;
	CToolBar* m_pToolbar;
	CWnd* m_pParentWnd;
	CAccelTable m_listAccel;

	// generated overrides
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnUpdate_NotEditing( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_AnySelected( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_SingleSelected( CCmdUI* pCmdUI );

	afx_msg void OnRemoveItem( void );
	afx_msg void OnRemoveAll( void );
	afx_msg void OnUpdate_RemoveAll( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // ListCtrlEditorFrame_h
