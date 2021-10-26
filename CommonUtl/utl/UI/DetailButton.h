#ifndef DetailButton_h
#define DetailButton_h
#pragma once

#include "Dialog_fwd.h"
#include "Control_fwd.h"
#include "IconButton.h"


// Works in tandem with a host control (e.g. an CEdit).
// Formerly used as details button in CBaseDetailHostCtrl, currently replaced by a toolbar with multiple buttons.

class CDetailButton
	: public CIconButton
	, public ui::ICustomCmdInfo
{
public:
	CDetailButton( ui::IBuddyCommand* pOwnerCallback, UINT iconId = 0 );

	void Create( CWnd* pHostCtrl );
	void LayoutButton( void );

	const ui::CBuddyLayout& GetBuddyLayout( void ) const { return m_buddyLayout; }
	ui::CBuddyLayout& RefBuddyLayout( void ) { return m_buddyLayout; }

	enum Metrics { Spacing = 2 };
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	ui::IBuddyCommand* m_pOwnerCallback;
	CWnd* m_pHostCtrl;
	ui::CBuddyLayout m_buddyLayout;

	enum { ButtonId = -1 };
private:
	// generated message map
	afx_msg BOOL OnReflect_BnClicked( void );

	DECLARE_MESSAGE_MAP()
};


#endif // DetailButton_h
