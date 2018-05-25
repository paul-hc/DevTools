#ifndef ReportListCustomDraw_h
#define ReportListCustomDraw_h
#pragma once

#include "ReportListControl.h"


class CReportListCustomDraw
{
	typedef LPARAM TRowKey;								// row keys are invariant to sorting
	typedef int TColumn;
	enum { EntireRecord = CReportListControl::EntireRecord };
public:
	CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList );

	static bool IsTooltipDraw( const NMLVCUSTOMDRAW* pDraw );

	bool ApplyCellTextEffect( void );
	bool ApplyTextEffect( const ui::CTextEffect& textEffect );

	ui::CTextEffect ExtractTextEffects( void ) const;
private:
	CRect MakeItemTextRect( void ) const;
private:
	NMLVCUSTOMDRAW* m_pDraw;
	CReportListControl* m_pList;
	CDC* m_pDC;
public:
	const int m_index;
	const TColumn m_subItem;
	const TRowKey m_rowKey;
	const utl::ISubject* m_pObject;
};


#endif // ReportListCustomDraw_h
