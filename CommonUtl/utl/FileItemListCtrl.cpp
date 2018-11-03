
#include "stdafx.h"
#include "FileItemListCtrl.h"
#include "PathItemBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileItemListCtrl::CFileItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= DefaultStyleEx*/ )
	 : CDragListCtrl( columnLayoutId, listStyleEx )
{
}

CFileItemListCtrl::~CFileItemListCtrl()
{
}
