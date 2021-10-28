#ifndef ToolbarStrip_h
#define ToolbarStrip_h
#pragma once

#include <afxext.h>			// CToolBar
#include "ToolStrip.h"
#include "DibDraw_fwd.h"
#include "ui_fwd.h"


class CToolbarStrip : public CToolBar
{
public:
	CToolbarStrip( void );
	virtual ~CToolbarStrip();

	// button initialization (before DDX_ creation)
	CToolStrip& GetStrip( void ) { return m_strip; }

	bool LoadToolStrip( UINT toolStripId, COLORREF transpColor = color::Auto );
	bool InitToolbarButtons( void );						// use already set up strip to initialize toolbar buttons
	void SetCustomDisabledImageList( gdi::DisabledStyle style = gdi::DisabledGrayOut );

	CSize GetIdealBarSize( void ) const;

	void UpdateCmdUI( void );

	void TrackButtonMenu( UINT buttonId, CWnd* pTargetWnd, CMenu* pPopupMenu, ui::PopupAlign popupAlign );

	bool GetEnableUnhandledCmds( void ) const { return m_enableUnhandledCmds; }
	void SetEnableUnhandledCmds( bool enableUnhandledCmds = true ) { m_enableUnhandledCmds = enableUnhandledCmds; }
protected:
	CToolStrip m_strip;

	enum Metrics { BtnEdgeWidth = 7, BtnEdgeHeight = 7 };
private:
	bool m_enableUnhandledCmds;
	std::auto_ptr< CImageList > m_pDisabledImageList;

	// generated overrides
protected:
	// generated message map
	DECLARE_MESSAGE_MAP()
};


#endif // ToolbarStrip_h
