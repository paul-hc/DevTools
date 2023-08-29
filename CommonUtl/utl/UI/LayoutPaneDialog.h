#ifndef LayoutPaneDialog_h
#define LayoutPaneDialog_h
#pragma once

#include "Dialog_fwd.h"
#include "LayoutMetrics.h"
#include <afxpanedialog.h>


class CLayoutPaneDialog : public CPaneDialog
	, public ui::ILayoutEngine
	, public ui::ICustomCmdInfo
{
	DECLARE_SERIAL( CLayoutPaneDialog )
public:
	CLayoutPaneDialog( void );
	virtual ~CLayoutPaneDialog();

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void ) implements(ui::ILayoutEngine);
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) implements(ui::ILayoutEngine);
	virtual bool HasControlLayout( void ) const implements(ui::ILayoutEngine);

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const implements(ui::ICustomCmdInfo);

	// base overrides
	virtual BOOL OnShowControlBarMenu( CPoint point ) overrides(CPane);
private:
	std::auto_ptr<CLayoutEngine> m_pLayoutEngine;
protected:
	// transient data members (managed by concrete subclass)
	bool m_fillToolBarBkgnd;			// if false use dialog background, true for themed toolbar background fill
	bool m_showControlBarMenu;			// if false prevents displaying the pane standard context menu (Floating|Docked|etc)

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // LayoutPaneDialog_h
