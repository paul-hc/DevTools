
#include "stdafx.h"
#include "PathItemListCtrl.h"
#include "PathItemBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
{
	AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );	// default row item comparator
	SetCustomFileGlyphDraw();
}

CPathItemListCtrl::~CPathItemListCtrl()
{
}
