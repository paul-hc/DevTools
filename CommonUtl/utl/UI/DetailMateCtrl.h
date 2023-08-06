#ifndef DetailMateCtrl_h
#define DetailMateCtrl_h
#pragma once

#include "Control_fwd.h"
#include "Dialog_fwd.h"


// A mate control that works in tandem with a host buddy control that handles a detail command via ui::IBuddyCommandHandler interface,
// and provides tooltip text for internal command.
// The BaseCtrl class is tipically a button (single command).
// Host control manages the layout of this pair of controls.

template< typename BaseCtrl >
class CDetailMateCtrl
	: public BaseCtrl
	, public ui::ICustomCmdInfo
{
public:
	CDetailMateCtrl( ui::IBuddyCommandHandler* pHostCmdHandler );
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
protected:
	ui::IBuddyCommandHandler* m_pHostCmdHandler;
	CWnd* m_pHostCtrl;

	// generated stuff
private:
	afx_msg BOOL OnReflect_Command( void );					// for button control

	DECLARE_MESSAGE_MAP()
};


#endif // DetailMateCtrl_h
