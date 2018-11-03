#ifndef FileItemListCtrl_h
#define FileItemListCtrl_h
#pragma once

#include "DragListCtrl.h"


class CPathItemBase;


class CFileItemListCtrl : public CDragListCtrl
{
public:
	CFileItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = DefaultStyleEx );
	virtual ~CFileItemListCtrl();
private:
};


#endif // FileItemListCtrl_h
