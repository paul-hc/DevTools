#ifndef DetailIconButton_h
#define DetailIconButton_h
#pragma once

#include "DetailMateCtrl.h"
#include "IconButton.h"


// Works in tandem with a host control (e.g. a CEdit).
// Formerly used as details button in CBaseDetailHostCtrl, currently replaced by a toolbar with multiple command buttons.

class CDetailIconButton : public CDetailMateCtrl<CIconButton>
{
public:
	CDetailIconButton( ui::IBuddyCommandHandler* pHostCmdHandler, UINT iconId = 0 );

	void CreateShrinkBuddy( const ui::CBuddyLayout& buddyLayout = ui::CBuddyLayout::s_tileToRight );

	enum { ButtonId = -1 };
};


#endif // DetailIconButton_h
