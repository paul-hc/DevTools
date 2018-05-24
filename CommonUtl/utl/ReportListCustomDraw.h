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

	bool ApplyCellTextEffect( void );
	bool ApplyTextEffect( const ui::CTextEffect& textEffect );

	ui::CTextEffect ExtractTextEffects( void ) const;
private:
	NMLVCUSTOMDRAW* m_pDraw;
	CReportListControl* m_pList;

	int m_index;
	TColumn m_subItem;
	TRowKey m_rowKey;
	utl::ISubject* m_pObject;

	CDC* m_pDC;
};


#endif // ReportListCustomDraw_h
