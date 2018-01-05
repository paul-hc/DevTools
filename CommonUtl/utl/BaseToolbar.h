#ifndef BaseToolbar_h
#define BaseToolbar_h
#pragma once

#include <afxext.h>			// CToolBar
#include "ToolStrip.h"
#include "ui_fwd.h"


class CBaseToolbar : public CToolBar
{
public:
	CBaseToolbar( void );
	virtual ~CBaseToolbar();

	// button initialization (before DDX_ creation)
	CToolStrip& GetStrip( void ) { return m_strip; }

	bool LoadToolStrip( UINT toolStripId, COLORREF transpColor = color::Auto );
	bool InitToolbarButtons( void );						// use already set up strip to initialize toolbar buttons
	void SetCustomDisabledImageList( void );

	void UpdateCmdUI( void );

	void TrackButtonMenu( UINT buttonId, CWnd* pTargetWnd, CMenu* pPopupMenu, ui::PopupAlign popupAlign );
protected:
	CToolStrip m_strip;

	enum Metrics { BtnEdgeWidth = 7, BtnEdgeHeight = 7 };
private:
	std::auto_ptr< CImageList > m_pDisabledImageList;
public:
	// generated overrides
protected:
	// generated message map
	DECLARE_MESSAGE_MAP()
};


#endif // BaseToolbar_h
