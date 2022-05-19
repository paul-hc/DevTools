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
	CToolbarStrip( gdi::DisabledStyle disabledStyle = gdi::DisabledGrayOut );
	virtual ~CToolbarStrip();

	// button initialization (before DDX_ creation)
	const CToolStrip& GetStrip( void ) const { return m_strip; }
	CToolStrip& GetStrip( void ) { return m_strip; }

	bool LoadToolStrip( UINT toolStripId, COLORREF transpColor = color::Auto );
	bool InitToolbarButtons( void );						// use already set up strip buttons to initialize bar buttons and imagelist

	gdi::DisabledStyle GetDisabledStyle( void ) const { return m_disabledStyle; }
	void SetDisabledStyle( gdi::DisabledStyle disabledStyle );

	CSize GetIdealBarSize( void ) const;

	void UpdateCmdUI( void );

	void TrackButtonMenu( UINT buttonId, CWnd* pTargetWnd, CMenu* pPopupMenu, ui::PopupAlign popupAlign );

	bool GetEnableUnhandledCmds( void ) const { return m_enableUnhandledCmds; }
	void SetEnableUnhandledCmds( bool enableUnhandledCmds = true ) { m_enableUnhandledCmds = enableUnhandledCmds; }
private:
	void SetCustomDisabledImageList( void );
protected:
	CToolStrip m_strip;

	enum Metrics { BtnEdgeWidth = 7, BtnEdgeHeight = 7 };
private:
	bool m_enableUnhandledCmds;
	gdi::DisabledStyle m_disabledStyle;
	std::auto_ptr<CImageList> m_pDisabledImageList;

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // ToolbarStrip_h
