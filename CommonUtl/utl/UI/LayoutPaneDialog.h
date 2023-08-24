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
	CLayoutPaneDialog( bool fillToolBarBkgnd = true );
	virtual ~CLayoutPaneDialog();

	bool GetFillToolBarBkgnd( void ) const { return m_fillToolBarBkgnd; }
	void SetFillToolBarBkgnd( bool fillToolBarBkgnd = true ) { m_fillToolBarBkgnd = fillToolBarBkgnd; }

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void );
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );
	virtual bool HasControlLayout( void ) const;

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	// base overrides
	virtual void Serialize( CArchive& archive ) overrides(CDockablePane);
	virtual void CopyState( CDockablePane* pSrc ) overrides(CDockablePane);
private:
	std::auto_ptr<CLayoutEngine> m_pLayoutEngine;
	bool m_fillToolBarBkgnd;			// if false use dialog background, true for themed toolbar background fill

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
