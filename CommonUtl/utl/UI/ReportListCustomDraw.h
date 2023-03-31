#ifndef ReportListCustomDraw_h
#define ReportListCustomDraw_h
#pragma once

#include "ReportListControl.h"


class CReportListCustomDraw
	: public CListLikeCustomDrawBase
	, public CListTraits
{
public:
	CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList );

	// cell text effects
	bool ApplyCellTextEffect( void );
	COLORREF GetRealizedBkColor( void ) const;
	COLORREF GetRealizedTextColor( DiffSide diffSide, const str::TMatchSequence* pCellSeq = nullptr ) const;

	// text diffs
	bool DrawCellTextDiffs( void );
private:
	// cell text effects
	ui::CTextEffect MakeCellEffect( void ) const;
	bool ApplyEffect( const ui::CTextEffect& textEffect );			// to m_pDraw

	// text diffs
	void DrawCellTextDiffs( DiffSide diffSide, const str::TMatchSequence& cellSeq, const CRect& textRect );

	CRect MakeCellTextRect( void ) const;
	void BuildTextMatchEffects( std::vector<ui::CTextEffect>& rMatchEffects, DiffSide diffSide, const str::TMatchSequence& cellSeq ) const;
	bool SelectTextEffect( const ui::CTextEffect& textEffect );		// to m_pDC
	void DrawTextFrame( const CRect& textRect, const ui::CFrameFillTraits& frameFillTraits );

	bool IsSelItemContrast( void ) const;							// item is blue backgound with white text?
private:
	NMLVCUSTOMDRAW* m_pDraw;
	CReportListControl* m_pList;
public:
	const int m_index;
	const TColumn m_subItem;
	const TRowKey m_rowKey;
	const utl::ISubject* m_pObject;
	const bool m_isReportMode;
};


#endif // ReportListCustomDraw_h
