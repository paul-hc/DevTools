#ifndef PathItemListCtrl_h
#define PathItemListCtrl_h
#pragma once

#include "ReportListControl.h"


class CPathItemBase;


class CPathItemListCtrl : public CReportListControl
{
public:
	CPathItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = DefaultStyleEx );
	virtual ~CPathItemListCtrl();
private:
};


#endif // PathItemListCtrl_h
