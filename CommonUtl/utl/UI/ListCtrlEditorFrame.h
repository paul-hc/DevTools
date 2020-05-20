#ifndef ListCtrlEditorFrame_h
#define ListCtrlEditorFrame_h
#pragma once

#include "AccelTable.h"
#include "TooltipsHook.h"


class CReportListControl;


class CListCtrlEditorFrame : public CCmdTarget
						   , public CTooltipsHook		// hooks tooltip notification messages of the toolbar
{
public:
	CListCtrlEditorFrame( CReportListControl* pListCtrl, CToolBar* pToolbar );
	virtual ~CListCtrlEditorFrame();

	bool InEditMode( void ) const;
private:
	CReportListControl* m_pListCtrl;
	CToolBar* m_pToolbar;
	CWnd* m_pParentWnd;
	CAccelTable m_listAccel;

	// generated overrides
public:
	virtual bool HandleTranslateMessage( MSG* pMsg );
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
