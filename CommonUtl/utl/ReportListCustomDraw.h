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
	bool EraseRowBkgndDiffs( void );
	bool DrawCellTextDiffs( void );
private:
	// cell text effects
	ui::CTextEffect MakeCellEffect( void ) const;
	bool ApplyEffect( const ui::CTextEffect& textEffect );		// to m_pDraw

	// text diffs
	enum DiffColumn { SrcColumn, DestColumn };

	void DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq );
	void DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq, const CRect& textRect );

	void StrongOverrideEffect( ui::CTextEffect& rTextEffect ) const;			// strong overrides
	bool SelectTextEffect( const ui::CTextEffect& textEffect );		// to m_pDC
	CRect MakeItemTextRect( void ) const;
	bool IsContrastRow( void ) const;		// dark background for selected item?

	static bool HasMismatch( const str::TMatchSequence& cellSeq );
private:
	NMLVCUSTOMDRAW* m_pDraw;
	CReportListControl* m_pList;
public:
	const int m_index;
	const TColumn m_subItem;
	const TRowKey m_rowKey;
	const utl::ISubject* m_pObject;

	static bool s_useDefaultDraw;
	static bool s_dbgGuides;
private:
	CDC* m_pDC;
};


#endif // ReportListCustomDraw_h
