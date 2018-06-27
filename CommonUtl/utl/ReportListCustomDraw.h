#ifndef ReportListCustomDraw_h
#define ReportListCustomDraw_h
#pragma once

#include "ReportListControl.h"


class CReportListCustomDraw
{
	typedef LPARAM TRowKey;								// row keys are invariant to sorting
	typedef int TColumn;
	typedef CReportListControl::CDiffColumnPair TDiffColumnPair;

	enum { EntireRecord = CReportListControl::EntireRecord };
public:
	CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList );

	static bool IsTooltipDraw( const NMLVCUSTOMDRAW* pDraw );

	// cell text effects
	bool ApplyCellTextEffect( void );

	// text diffs
	bool DrawCellTextDiffs( void );
private:
	// cell text effects
	ui::CTextEffect MakeCellEffect( void ) const;
	bool ApplyEffect( const ui::CTextEffect& textEffect );		// to m_pDraw

	// text diffs
	enum DiffColumn { SrcColumn, DestColumn };

	void DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq, const CRect& textRect );

	CRect MakeCellTextRect( void ) const;
	void BuildTextMatchEffects( std::vector< ui::CTextEffect >& rMatchEffects, DiffColumn diffColumn, const str::TMatchSequence& cellSeq ) const;
	COLORREF GetRealizedTextColor( DiffColumn diffColumn, const str::TMatchSequence& cellSeq ) const;
	bool SelectTextEffect( const ui::CTextEffect& textEffect );		// to m_pDC
private:
	NMLVCUSTOMDRAW* m_pDraw;
	CReportListControl* m_pList;
public:
	const int m_index;
	const TColumn m_subItem;
	const TRowKey m_rowKey;
	const utl::ISubject* m_pObject;
	const bool m_isReportMode;

	static bool s_useDefaultDraw;
	static bool s_dbgGuides;
private:
	CDC* m_pDC;
};


namespace dbg
{
	const TCHAR* FormatDrawStage( DWORD dwDrawStage );
}


#endif // ReportListCustomDraw_h
