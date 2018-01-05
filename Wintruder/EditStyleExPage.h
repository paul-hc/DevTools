#ifndef EditStyleExPage_h
#define EditStyleExPage_h
#pragma once

#include "EditFlagsBasePage.h"


class CEditStyleExPage : public CEditFlagsBasePage
{
public:
	CEditStyleExPage( void );
	virtual ~CEditStyleExPage();
protected:
	// base overrides
	virtual void StoreFlagStores( HWND hTargetWnd );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // EditStyleExPage_h
