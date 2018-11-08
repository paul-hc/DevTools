#ifndef FileItemListCtrl_h
#define FileItemListCtrl_h
#pragma once

#include "ReportListControl.h"


class CPathItemBase;


class CFileItemListCtrl : public CReportListControl
{
public:
	CFileItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = DefaultStyleEx );
	virtual ~CFileItemListCtrl();
private:
};


#endif // FileItemListCtrl_h
