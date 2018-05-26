
#include "stdafx.h"
#include "ReportListCustomDraw.h"
#include "Color.h"
#include "GpUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CReportListCustomDraw::s_useDefaultDraw = false;
bool CReportListCustomDraw::s_dbgGuides = false;


CReportListCustomDraw::CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList )
	: m_pDraw( safe_ptr( pDraw ) )
	, m_pList( safe_ptr( pList ) )
	, m_index( static_cast< int >( m_pDraw->nmcd.dwItemSpec ) )
	, m_subItem( m_pDraw->iSubItem )
	, m_rowKey( m_pDraw->nmcd.lItemlParam != 0 ? m_pDraw->nmcd.lItemlParam : m_index )
	, m_pObject( CReportListControl::ToSubject( m_pDraw->nmcd.lItemlParam ) )
	, m_pDC( CDC::FromHandle( m_pDraw->nmcd.hdc ) )
{
}

bool CReportListCustomDraw::IsTooltipDraw( const NMLVCUSTOMDRAW* pDraw )
{
	ASSERT_PTR( pDraw );
	static const CRect s_emptyRect( 0, 0, 0, 0 );
	if ( s_emptyRect == pDraw->nmcd.rc )
		return true;						// tooltip custom draw
	return false;
}

bool CReportListCustomDraw::ApplyCellTextEffect( void )
{
	if ( s_useDefaultDraw )
		return false;

	return ApplyEffect( MakeCellEffect() );
}

ui::CTextEffect CReportListCustomDraw::MakeCellEffect( void ) const
{
	ui::CTextEffect textEffect = m_pList->m_listTextEffect;							// start with the global list effect
	textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, EntireRecord ) );	// assign entire record

	if ( m_subItem != EntireRecord )
		textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, m_subItem ) );	// assign individual cell

	if ( m_pList->m_pTextEffectCallback != NULL )
		m_pList->m_pTextEffectCallback->CombineTextEffectAt( textEffect, m_rowKey, m_subItem );		// combine with callback effect (additive operation)

	if ( CLR_NONE == textEffect.m_bkColor )
		if ( m_pList->m_useAlternateRowColoring && HasFlag( m_index, 0x01 ) )
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

bool CReportListCustomDraw::EraseRowBkgndDiffs( void )
{
	if ( s_useDefaultDraw )
		return false;

	ui::CTextEffect rowEffect = MakeCellEffect();

	if ( m_pList->HasItemState( m_index, LVIS_SELECTED ) && !m_pList->UseExplorerTheme() )		// selected classic item (not "Explorer" visual theme background)?
		rowEffect.m_bkColor = GetSysColor( ::GetFocus() == m_pList->m_hWnd ? COLOR_HIGHLIGHT : COLOR_BTNFACE );		// focused or not: realize to real background for classic selection

	ApplyEffect( rowEffect );

	if ( rowEffect.m_bkColor != CLR_NONE )										// custom background, alternate row highlight, etc
		ui::FillRect( *m_pDC, m_pDraw->nmcd.rc, rowEffect.m_bkColor );			// fill background only for specialized colours

	if ( s_dbgGuides )
		gp::FrameRect( m_pDC, m_pDraw->nmcd.rc, color::SkyBlue, 50 );

	return rowEffect.m_bkColor != CLR_NONE;										// erased?
}

bool CReportListCustomDraw::DrawCellTextDiffs( void )
{
	if ( s_useDefaultDraw )
		return false;

	if ( const TDiffColumnPair* pDiffPair = m_pList->FindDiffColumnPair( m_subItem ) )
		if ( const str::TMatchSequence* pCellSeq = pDiffPair->FindRowSequence( m_rowKey ) )
		{
			DiffColumn diffColumn = pDiffPair->m_srcColumn == m_subItem ? SrcColumn : DestColumn;
			if ( HasMismatch( *pCellSeq ) )
			{
				DrawCellTextDiffs( diffColumn, *pCellSeq );
				return true;
			}
			else if ( str::MatchEqual == pCellSeq->m_match && DestColumn == diffColumn )
				ApplyEffect( m_pList->m_matchDest_DiffEffect );			// gray-out text of unmodified dest files, but do default drawing
		}

	return false;
}

void CReportListCustomDraw::DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq )
{
	DrawCellTextDiffs( diffColumn, cellSeq, MakeCellTextRect() );

	if (false&& DestColumn == diffColumn )
		if ( HasFlag( m_pDraw->nmcd.uItemState, CDIS_FOCUS ) )
		{
			CRect focusRect;
			m_pList->GetItemRect( m_index, &focusRect, LVIR_BOUNDS );

			if ( m_pList->UseExplorerTheme() )
				focusRect.DeflateRect( 1, 1 );
			else
				focusRect.DeflateRect( 4, 0, 1, 1 );

			m_pDC->DrawFocusRect( &focusRect );
		}
}

void CReportListCustomDraw::DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq, const CRect& textRect )
{
	enum { TextStyle = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX };	// DT_NOCLIP

	if ( s_dbgGuides )
		gp::FrameRect( m_pDC, textRect, color::Orange, 50 );

	ui::CTextEffect normalEffect = MakeCellEffect();
	normalEffect.m_textColor = ui::GetActualColor( m_pList->m_listTextEffect.m_textColor, GetSysColor( COLOR_WINDOWTEXT ) );		// realize to reset existing text highlighting

	ui::CTextEffect highlightEffect = normalEffect | ( SrcColumn == diffColumn ? m_pList->m_deleteSrc_DiffEffect : m_pList->m_mismatchDest_DiffEffect );

	ui::CTextEffect diffCaseEffect = highlightEffect;
	ClearFlag( diffCaseEffect.m_fontEffect, ui::Bold );
	SetFlag( diffCaseEffect.m_fontEffect, ui::Underline );

	CFont* pOldFont = m_pDC->GetCurrentFont();
	COLORREF oldTextColor = m_pDC->GetTextColor();
	COLORREF oldBkColor = m_pDC->GetBkColor();
	int oldBkMode = m_pDC->SetBkMode( TRANSPARENT );
	CRect itemRect = textRect;

	const TCHAR* pText = SrcColumn == diffColumn ? cellSeq.m_textPair.first.c_str() : cellSeq.m_textPair.second.c_str();
	const std::vector< str::Match >& matchSeq = SrcColumn == diffColumn ? cellSeq.m_matchSeqPair.first : cellSeq.m_matchSeqPair.second;

	for ( size_t pos = 0; pos != matchSeq.size() && itemRect.left < itemRect.right; )
	{
		const ui::CTextEffect* pEffect = NULL;
		switch ( matchSeq[ pos ] )
		{
			case str::MatchEqual: pEffect = &normalEffect; break;
			case str::MatchEqualDiffCase: pEffect = &diffCaseEffect; break;
			case str::MatchNotEqual: pEffect = &highlightEffect; break;
		}
		SelectTextEffect( *pEffect );

		unsigned int matchLen = static_cast< unsigned int >( utl::GetMatchingLength( matchSeq.begin() + pos, matchSeq.end() ) );

		m_pDC->DrawText( pText, matchLen, &itemRect, TextStyle );
		itemRect.left += m_pDC->GetTextExtent( pText, matchLen ).cx;

		pText += matchLen;
		pos += matchLen;
	}

	m_pDC->SelectObject( pOldFont );
	m_pDC->SetTextColor( oldTextColor );
	m_pDC->SetBkColor( oldBkColor );
	m_pDC->SetBkMode( oldBkMode );
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

bool CReportListCustomDraw::HasMismatch( const str::TMatchSequence& cellSeq )
{
	return
		cellSeq.m_match != str::MatchEqual &&
		!cellSeq.m_textPair.second.empty();			// means dest not init, so don't hilight changes
}

CRect CReportListCustomDraw::MakeCellTextRect( void ) const
{
	CRect textRect = m_pDraw->nmcd.rc;

	// requires larger spacing for sub-item: 2 for item, 6 for sub-item
	textRect.left += ( 0 == m_subItem ? CReportListControl::ItemSpacingX : CReportListControl::SubItemSpacingX );
	return textRect;
}
