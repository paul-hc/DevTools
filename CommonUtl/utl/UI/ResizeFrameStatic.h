#ifndef ResizeFrameStatic_h
#define ResizeFrameStatic_h
#pragma once

#include "CtrlInterfaces.h"
#include "ResizeGripBar.h"


// Static frame that surrounds the splitter panes (1st and 2nd) + the inner gripper bar.  This frame is usually hidden.
//
class CResizeFrameStatic : public CStatic
	, public ui::ILayoutFrame
{
public:
	CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl, resize::Orientation orientation = resize::NorthSouth, resize::ToggleStyle toggleStyle = resize::ToggleSecond );
	virtual ~CResizeFrameStatic();

	enum Notification { RF_GRIPPER_TOGGLE = 1, RF_GRIPPER_RESIZING, RF_GRIPPER_RESIZED };

	void NotifyParent( Notification notification );

	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }

	CResizeGripBar& GetGripBar( void ) const { return *m_pGripBar; }
public:
	// ui::ILayoutFrame interface
	virtual CWnd* GetControl( void ) const implements(ui::ILayoutFrame);
	virtual CWnd* GetDialog( void ) const implements(ui::ILayoutFrame);
	virtual void OnControlResized( void ) implements(ui::ILayoutFrame);
	virtual bool ShowPane( bool show ) implements(ui::ILayoutFrame);
	virtual CResizeGripBar* GetSplitterGripBar( void ) const implements(ui::ILayoutFrame) { return &GetGripBar(); }
private:
	std::auto_ptr<CResizeGripBar> m_pGripBar;
	std::tstring m_regSection;		// optional, for persisting split percentage

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnDestroy( void );

	DECLARE_MESSAGE_MAP()
};


#include "LayoutMetrics.h"


// Static frame for one of the splitter panes that contains embedded controls with layout support.  This frame is usually hidden.
//
class CLayoutStatic : public CStatic
	, public ui::ILayoutEngine
	, public ui::ILayoutFrame
{
public:
	CLayoutStatic( void );
	virtual ~CLayoutStatic();

	void SetUseSmoothTransparentGroups( bool useSmoothTransparentGroups = true );		// prevents groups clipping issues (in dialogs)

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void ) implements(ui::ILayoutEngine);
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) implements(ui::ILayoutEngine);
	virtual bool HasControlLayout( void ) const implements(ui::ILayoutEngine);

	// ui::ILayoutFrame interface
	virtual CWnd* GetControl( void ) const implements(ui::ILayoutFrame);
	virtual CWnd* GetDialog( void ) const implements(ui::ILayoutFrame);
	virtual void OnControlResized( void ) implements(ui::ILayoutFrame);
	virtual bool ShowPane( bool show ) implements(ui::ILayoutFrame);
	virtual CResizeGripBar* GetSplitterGripBar( void ) const implements(ui::ILayoutFrame);
	virtual void SetSplitterGripBar( CResizeGripBar* pResizeGripBar ) implements(ui::ILayoutFrame);
private:
	std::auto_ptr<CPaneLayoutEngine> m_pPaneLayoutEngine;
	CResizeGripBar* m_pResizeGripBar;		// typically assigned by the sibling CResizeGripBar

	// generated stuff
protected:
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ResizeFrameStatic_h
