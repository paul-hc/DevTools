#ifndef ResizeFrameStatic_h
#define ResizeFrameStatic_h
#pragma once

#include "CtrlInterfaces.h"
#include "ResizeGripBar.h"


class CResizeFrameStatic : public CStatic
	, public ui::ILayoutFrame
{
public:
	CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl, resize::Orientation orientation = resize::NorthSouth, resize::ToggleStyle toggleStyle = resize::ToggleSecond );
	virtual ~CResizeFrameStatic();

	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }

	CResizeGripBar& GetGripBar( void ) const { return *m_pGripBar; }
public:
	// ui::ILayoutFrame interface
	virtual CWnd* GetControl( void ) const implements(ui::ILayoutFrame);
	virtual void OnControlResized( void ) implements(ui::ILayoutFrame);
	virtual bool ShowFrame( bool show ) implements(ui::ILayoutFrame);

	enum Notification { RF_GRIPPER_TOGGLE = 1, RF_GRIPPER_RESIZING, RF_GRIPPER_RESIZED };

	void NotifyParent( Notification notification );
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


class CLayoutStatic : public CStatic
	, public ui::ILayoutEngine
	, public ui::ILayoutFrame
{
public:
	CLayoutStatic( void );
	virtual ~CLayoutStatic();

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void ) implements(ui::ILayoutEngine);
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) implements(ui::ILayoutEngine);
	virtual bool HasControlLayout( void ) const implements(ui::ILayoutEngine);

	// ui::ILayoutFrame interface
	virtual CWnd* GetControl( void ) const implements(ui::ILayoutFrame);
	virtual void OnControlResized( void ) implements(ui::ILayoutFrame);
	virtual bool ShowFrame( bool show ) implements(ui::ILayoutFrame);
private:
	std::auto_ptr<CLayoutEngine> m_pLayoutEngine;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ResizeFrameStatic_h
