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
	virtual void OnControlResized( void );

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


#endif // ResizeFrameStatic_h
