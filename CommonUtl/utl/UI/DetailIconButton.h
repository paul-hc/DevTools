#ifndef DetailIconButton_h
#define DetailIconButton_h
#pragma once

#include "DetailMateCtrl.h"
#include "IconButton.h"


// Works in tandem with a host control (e.g. a CEdit).
// Formerly used as details button in CBaseHostToolbarCtrl, currently replaced by a toolbar with multiple command buttons.

class CDetailIconButton : public CDetailMateCtrl<CIconButton>
{
public:
	CDetailIconButton( ui::IBuddyCommandHandler* pHostCmdHandler, UINT iconId = 0 );

	void CreateTandem( const ui::CTandemLayout& tandemLayout = ui::CTandemLayout::s_mateOnRight );

	enum { ButtonId = -1 };
};


#endif // DetailIconButton_h
