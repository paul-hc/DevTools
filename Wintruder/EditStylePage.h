#ifndef EditStylePage_h
#define EditStylePage_h
#pragma once

#include "EditFlagsBasePage.h"


class CEditStylePage : public CEditFlagsBasePage
{
public:
	CEditStylePage( void );
	virtual ~CEditStylePage();
protected:
	virtual void StoreFlagStores( HWND hTargetWnd );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // EditStylePage_h
