#ifndef DialogToolBar_h
#define DialogToolBar_h
#pragma once

#include "Control_fwd.h"
#include "ToolbarStrip.h"


class CDialogToolBar : public CToolbarStrip
{
public:
	CDialogToolBar( gdi::DisabledStyle disabledStyle = gdi::DisabledGrayOut ) : CToolbarStrip( disabledStyle ) {}
	virtual ~CDialogToolBar();

	// use a placeholder static (with the same id)
	void DDX_Placeholder( CDataExchange* pDX, int placeholderId,
						  TAlignment alignToPlaceholder = H_AlignLeft | V_AlignBottom, UINT toolbarResId = 0 );

	// use a placeholder static (with the same id)
	void DDX_Tandem( CDataExchange* pDX, CWnd* pHostCtrl, int toolbarId, const ui::CTandemLayout& tandemLayout = ui::CTandemLayout::s_mateOnRight,
					 UINT toolbarResId = 0 );

	void CreateReplacePlaceholder( CWnd* pParent, int placeholderId, TAlignment alignToPlaceholder = H_AlignLeft | V_AlignBottom,
								   UINT toolbarResId = 0 );

	void CreateTandem( CWnd* pHostCtrl, const ui::CTandemLayout& tandemLayout = ui::CTandemLayout::s_mateOnRight, UINT toolbarResId = 0 );

	// creates a toolbar with id AFX_IDW_TOOLBAR
	void CreateToolbar( CWnd* pParent, const CRect* pAlignScreenRect = NULL,
						TAlignment alignment = H_AlignRight | V_AlignCenter, UINT toolbarResId = 0 );
private:
	void CreateToolbar( CWnd* pParent, UINT toolbarResId );

	// generated stuff
protected:
	afx_msg LRESULT OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam );
	afx_msg BOOL OnCustomDrawReflect( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // DialogToolBar_h
