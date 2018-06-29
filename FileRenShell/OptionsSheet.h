#ifndef OptionsSheet_h
#define OptionsSheet_h
#pragma once

#include "utl/LayoutPropertySheet.h"


class COptionsSheet : public CLayoutPropertySheet
{
public:
	COptionsSheet( CWnd* pParent, UINT initialPageIndex = UINT_MAX );

	enum PageIndex { GeneralPage, CapitalizePage };
};


#endif // OptionsSheet_h
