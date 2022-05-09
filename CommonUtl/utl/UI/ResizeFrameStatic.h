#ifndef ResizeFrameStatic_h
#define ResizeFrameStatic_h
#pragma once

#include "CtrlInterfaces.h"


class CResizeGripBar;


class CResizeFrameStatic : public CStatic
						 , public ui::ILayoutFrame
{
public:
	CResizeFrameStatic( CWnd* pFirstCtrl, CWnd* pSecondCtrl, CResizeGripBar* pGripper );
	virtual ~CResizeFrameStatic();

	CResizeGripBar* GetGripBar( void ) const { return m_pGripBar.get(); }

	void SetSection( const std::tstring& regSection ) { m_regSection = regSection; }
public:
	// ui::ILayoutFrame interface
	virtual void OnControlResized( UINT ctrlId );

	enum Notification { RF_GRIPPER_TOGGLE = 1, RF_GRIPPER_RESIZING, RF_GRIPPER_RESIZED };

	void NotifyParent( Notification notification );
private:
	std::auto_ptr<CResizeGripBar> m_pGripBar;
	CWnd* m_pFirstCtrl;
	CWnd* m_pSecondCtrl;
	std::tstring m_regSection;		// optional, for persisting split percentage

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnDestroy( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ResizeFrameStatic_h
