
#include "stdafx.h"
#include "ReportListCustomDraw.h"
#include "Color.h"
#include "GpUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CReportListCustomDraw::CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList )
	: CListLikeCustomDrawBase( &pDraw->nmcd )
	, m_pDraw( pDraw )
	, m_pList( safe_ptr( pList ) )
	, m_index( static_cast< int >( m_pDraw->nmcd.dwItemSpec ) )
	, m_subItem( m_pDraw->iSubItem )
	, m_rowKey( m_pDraw->nmcd.lItemlParam != 0 ? m_pDraw->nmcd.lItemlParam : m_index )
	, m_pObject( CReportListControl::ToSubject( m_pDraw->nmcd.lItemlParam ) )
	, m_isReportMode( LV_VIEW_DETAILS == m_pList->GetView() )
{
}

bool CReportListCustomDraw::ApplyCellTextEffect( void )
{
	if ( s_useDefaultDraw )
		return false;

	return ApplyEffect( MakeCellEffect() );
}

ui::CTextEffect CReportListCustomDraw::MakeCellEffect( void ) const
{
	ui::CTextEffect textEffect = m_pList->m_ctrlTextEffect;							// start with the global list effect
	textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, EntireRecord ) );	// assign entire record

	if ( m_subItem != EntireRecord )
		textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, m_subItem ) );	// assign individual cell

	m_pList->CombineTextEffectAt( textEffect, m_rowKey, m_subItem, m_pList );				// combine with callback effect (additive operation)

	if ( CLR_NONE == textEffect.m_bkColor )
		if ( m_pList->GetUseAlternateRowColoring() && HasFlag( m_index, 0x01 ) )
			textEffect.m_bkColor = color::GhostWhite;

	return textEffect;
}

bool CReportListCustomDraw::ApplyEffect( const ui::CTextEffect& textEffect )
{
	// handle even if textEffect.IsNull() since it may need to reset previous cell effects
	bool modified = false;

	if ( CFont* pFont = m_pList->GetFontEffectCache()->Lookup( textEffect.m_fontEffect ) )
		if ( m_pDC->SelectObject( pFont ) != pFont )
			modified = true;

	// when assigning CLR_NONE, the list view uses the default colour properly: this->GetTextColor(), this->GetBkColor()
	if ( textEffect.m_textColor != m_pDraw->clrText )
	{
		m_pDraw->clrText = textEffect.m_textColor;
		modified = true;
	}

	if ( textEffect.m_bkColor != m_pDraw->clrTextBk )
	{
		m_pDraw->clrTextBk = textEffect.m_bkColor;
		modified = true;
	}

	return modified;
}

COLORREF CReportListCustomDraw::GetRealizedBkColor( void ) const
{
	if ( IsSelItemContrast() )
		return GetSysColor( COLOR_HIGHLIGHT );

	if ( ui::IsActualColor( m_pDraw->clrTextBk ) )
		return m_pDraw->clrTextBk;

	COLORREF bkColor = bkColor = m_pList->GetTextBkColor();
	if ( ui::IsActualColor( bkColor ) )
		return bkColor;

	return ui::GetActualColorSysdef( m_pList->GetBkColor(), COLOR_WINDOW );
}

COLORREF CReportListCustomDraw::GetRealizedTextColor( DiffSide diffSide, const str::TMatchSequence* pCellSeq /*= NULL*/ ) const
{
	if ( IsSelItemContrast() )
		return GetSysColor( COLOR_HIGHLIGHTTEXT );

	if ( DestDiff == diffSide && pCellSeq != NULL && str::MatchEqual == pCellSeq->m_match )
		return m_pList->m_matchDest_DiffEffect.m_textColor;				// gray-out text of unmodified DEST files, but continue with default drawing

	return ui::GetActualColorSysdef( m_pList->m_ctrlTextEffect.m_textColor, COLOR_WINDOWTEXT );
}

bool CReportListCustomDraw::DrawCellTextDiffs( void )
{
	if ( s_useDefaultDraw )
		return false;

	if ( const CDiffColumnPair* pDiffPair = m_pList->FindDiffColumnPair( m_subItem ) )
		if ( const str::TMatchSequence* pCellSeq = pDiffPair->FindRowSequence( m_rowKey ) )
		{
			DrawCellTextDiffs( pDiffPair->GetDiffSide( m_subItem ), *pCellSeq, MakeCellTextRect() );
			return true;		// drawn diff item text
		}

	return false;
}

void CReportListCustomDraw::DrawCellTextDiffs( DiffSide diffSide, const str::TMatchSequence& cellSeq, const CRect& textRect )
{
	enum { TextStyle = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX };	// DT_NOCLIP

	if ( s_dbgGuides )
		gp::FrameRect( m_pDC, textRect, color::Orange, 50 );

	const TCHAR* pText = SrcDiff == diffSide ? cellSeq.m_textPair.first.c_str() : cellSeq.m_textPair.second.c_str();
	if ( str::IsEmpty( pText ) )
		return;

	std::vector< ui::CTextEffect > matchEffects;			// indexed by str::Match constants
	BuildTextMatchEffects( matchEffects, diffSide, cellSeq );

	CFont* pOldFont = m_pDC->GetCurrentFont();
	COLORREF oldTextColor = m_pDC->GetTextColor();
	COLORREF oldBkColor = m_pDC->GetBkColor();
	int oldBkMode = m_pDC->SetBkMode( TRANSPARENT );

	const std::vector< str::Match >& matchSeq = SrcDiff == diffSide ? cellSeq.m_matchSeqPair.first : cellSeq.m_matchSeqPair.second;
	CRect itemRect = textRect;

	for ( size_t pos = 0; pos != matchSeq.size() && itemRect.left < itemRect.right; )
	{
		unsigned int matchLen = static_cast< unsigned int >( utl::GetMatchingLength( matchSeq.begin() + pos, matchSeq.end() ) );
		const ui::CTextEffect& effect = matchEffects[ matchSeq[ pos ] ];

		SelectTextEffect( effect );

		CSize textSize = m_pDC->GetTextExtent( pText, matchLen );

		if ( !effect.m_frameFillTraits.IsNull() )
		{
			CRect textRect( itemRect.TopLeft(), textSize );
			ui::CenterRect( textRect, itemRect, false, true );
			textRect.InflateRect( 0, 2 );

			DrawTextFrame( textRect, effect.m_frameFillTraits );
		}

		m_pDC->DrawText( pText, matchLen, &itemRect, TextStyle );
		itemRect.left += textSize.cx;

		//::GdiFlush();

		pText += matchLen;
		pos += matchLen;
	}

	m_pDC->SelectObject( pOldFont );
	m_pDC->SetTextColor( oldTextColor );
	m_pDC->SetBkColor( oldBkColor );
	m_pDC->SetBkMode( oldBkMode );
}

void CReportListCustomDraw::DrawTextFrame( const CRect& textRect, const ui::CFrameFillTraits& frameFillTraits )
{
	Graphics graphics( m_pDC->GetSafeHdc() );
	Rect rect = gp::ToRect( textRect );
	rect.Inflate( 0, -1 );

	Pen pen( gp::MakeColor( frameFillTraits.m_color, frameFillTraits.m_frameAlpha ) );
	gp::FrameRectangle( graphics, rect, &pen );

	SolidBrush brush( gp::MakeColor( frameFillTraits.m_color, frameFillTraits.m_fillAlpha ) );
	gp::FillRectangle( graphics, rect, &brush );
}

void CReportListCustomDraw::BuildTextMatchEffects( std::vector< ui::CTextEffect >& rMatchEffects, DiffSide diffSide, const str::TMatchSequence& cellSeq ) const
{
	enum { LastMatch = str::MatchNotEqual, _MatchCount };

	rMatchEffects.resize( _MatchCount );

	lv::CMatchEffects effects( rMatchEffects );

	effects.m_rEqual = MakeCellEffect();
	effects.m_rEqual.m_textColor = GetRealizedTextColor( diffSide, &cellSeq );		// realize text colour to reset existing (prev sub-item) text highlighting

	effects.m_rNotEqual = effects.m_rEqual;
	if ( SrcDiff == diffSide && cellSeq.m_textPair.second.empty() )					// empty DEST?
	{	// avoid highlighting SRC text
	}
	else
		effects.m_rNotEqual.Combine( SrcDiff == diffSide ? m_pList->m_deleteSrc_DiffEffect : m_pList->m_mismatchDest_DiffEffect );

	effects.m_rEqualDiffCase = effects.m_rNotEqual;
	ClearFlag( effects.m_rEqualDiffCase.m_fontEffect, ui::Bold );

	if ( m_pList->GetHighlightTextDiffsFrame() )
	{
		if ( effects.m_rEqualDiffCase.m_textColor != effects.m_rEqual.m_textColor )		// diff?
			effects.m_rEqualDiffCase.SetFrameFromTextColor( 12, 3 );

		if ( effects.m_rNotEqual.m_textColor != effects.m_rEqual.m_textColor )			// diff?
			effects.m_rNotEqual.SetFrameFromTextColor( 15, 3 );
	}
	else
		SetFlag( effects.m_rEqualDiffCase.m_fontEffect, ui::Underline );

	m_pList->ModifyDiffTextEffectAt( effects, m_rowKey, m_subItem, m_pList );					// resert ui::Bold, etc
}

bool CReportListCustomDraw::SelectTextEffect( const ui::CTextEffect& textEffect )
{
	bool modified = false;

	if ( CFont* pFont = m_pList->GetFontEffectCache()->Lookup( textEffect.m_fontEffect ) )
		if ( m_pDC->SelectObject( pFont ) != pFont )
			modified = true;

	ASSERT( textEffect.m_textColor != CLR_NONE && textEffect.m_textColor != CLR_DEFAULT );

	if ( m_pDC->SetTextColor( textEffect.m_textColor ) != textEffect.m_textColor )
		modified = true;

	if ( textEffect.m_bkColor != CLR_NONE && textEffect.m_bkColor != CLR_DEFAULT )
		if ( m_pDC->SetBkColor( textEffect.m_bkColor ) != textEffect.m_bkColor )
			modified = true;

	return modified;
}

CRect CReportListCustomDraw::MakeCellTextRect( void ) const
{
	CRect textRect = m_pDraw->nmcd.rc;

	if ( m_isReportMode )
	{
		CRect imageRect;
		if ( m_pList->GetSubItemRect( m_index, m_subItem, LVIR_ICON, imageRect ) )		// item visible?
			if ( imageRect.Width() != 0 )												// sub-item has image?
				textRect.left = imageRect.right;										// shift text to the right of image
	}
	else
		m_pList->GetItemRect( m_index, &textRect, LVIR_LABEL );

	// requires larger spacing for sub-item: 2 for item, 6 for sub-item
	textRect.left += ( 0 == m_subItem ? CReportListControl::ItemSpacingX : CReportListControl::SubItemSpacingX );		// DON'T TOUCH IT: lines up perfectly with default text draw

	return textRect;
}

bool CReportListCustomDraw::IsSelItemContrast( void ) const
{
	// classic contrast selected item: blue backgound with white text (not "Explorer" visual theme background)?
	return m_pList->HasItemState( m_index, LVIS_SELECTED ) && !m_pList->GetUseExplorerTheme() && ::GetFocus() == m_pList->m_hWnd;
}
