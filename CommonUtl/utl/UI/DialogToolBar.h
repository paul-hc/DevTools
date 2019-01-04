#ifndef DialogToolBar_h
#define DialogToolBar_h
#pragma once

#include "ui_fwd.h"
#include "ToolbarStrip.h"


class CDialogToolBar : public CToolbarStrip
{
public:
	CDialogToolBar( gdi::DisabledStyle disabledStyle = gdi::DisabledGrayOut ) : m_disabledStyle( disabledStyle ) {}
	virtual ~CDialogToolBar();

	// use a placeholder static (with the same id)
	void DDX_Placeholder( CDataExchange* pDX, int placeholderId,
						  int alignToPlaceholder = H_AlignLeft | V_AlignBottom, UINT toolbarResId = 0 );

	// creates a toolbar with id AFX_IDW_TOOLBAR
	void CreateToolbar( CWnd* pParent, const CRect* pAlignScreenRect = NULL,
						int alignment = H_AlignRight | V_AlignCenter, UINT toolbarResId = 0 );
private:
	void CreateToolbar( CWnd* pParent, UINT toolbarResId );
private:
	gdi::DisabledStyle m_disabledStyle;
public:
	// generated overrides
protected:
	// generated message map
	afx_msg LRESULT OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam );
	afx_msg BOOL OnCustomDrawReflect( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // DialogToolBar_h
