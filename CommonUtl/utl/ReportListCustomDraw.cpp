
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
	, m_isReportMode( LV_VIEW_DETAILS == m_pList->GetView() )
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

bool CReportListCustomDraw::DrawCellTextDiffs( void )
{
	if ( s_useDefaultDraw )
		return false;

	if ( const TDiffColumnPair* pDiffPair = m_pList->FindDiffColumnPair( m_subItem ) )
		if ( const str::TMatchSequence* pCellSeq = pDiffPair->FindRowSequence( m_rowKey ) )
		{
			DiffColumn diffColumn = pDiffPair->m_srcColumn == m_subItem ? SrcColumn : DestColumn;
			DrawCellTextDiffs( diffColumn, *pCellSeq, MakeCellTextRect() );
			return true;		// drawn diff item text
		}

	return false;
}

void CReportListCustomDraw::DrawCellTextDiffs( DiffColumn diffColumn, const str::TMatchSequence& cellSeq, const CRect& textRect )
{
	enum { TextStyle = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX };	// DT_NOCLIP

	if ( s_dbgGuides )
		gp::FrameRect( m_pDC, textRect, color::Orange, 50 );

	std::vector< ui::CTextEffect > matchEffects;			// indexed by str::Match constants
	BuildTextMatchEffects( matchEffects, diffColumn, cellSeq );

	CFont* pOldFont = m_pDC->GetCurrentFont();
	COLORREF oldTextColor = m_pDC->GetTextColor();
	COLORREF oldBkColor = m_pDC->GetBkColor();
	int oldBkMode = m_pDC->SetBkMode( TRANSPARENT );

	const TCHAR* pText = SrcColumn == diffColumn ? cellSeq.m_textPair.first.c_str() : cellSeq.m_textPair.second.c_str();
	const std::vector< str::Match >& matchSeq = SrcColumn == diffColumn ? cellSeq.m_matchSeqPair.first : cellSeq.m_matchSeqPair.second;
	CRect itemRect = textRect;

	for ( size_t pos = 0; pos != matchSeq.size() && itemRect.left < itemRect.right; )
	{
		unsigned int matchLen = static_cast< unsigned int >( utl::GetMatchingLength( matchSeq.begin() + pos, matchSeq.end() ) );

		SelectTextEffect( matchEffects[ matchSeq[ pos ] ] );
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

void CReportListCustomDraw::BuildTextMatchEffects( std::vector< ui::CTextEffect >& rMatchEffects, DiffColumn diffColumn, const str::TMatchSequence& cellSeq ) const
{
	enum { LastMatch = str::MatchNotEqual, _MatchCount };

	rMatchEffects.resize( _MatchCount );
	rMatchEffects[ str::MatchEqual ] = MakeCellEffect();
	rMatchEffects[ str::MatchEqual ].m_textColor = GetRealizedTextColor( diffColumn, cellSeq );		// realize text colour to reset existing (prev sub-item) text highlighting

	rMatchEffects[ str::MatchNotEqual ] = rMatchEffects[ str::MatchEqual ];
	if ( SrcColumn == diffColumn && cellSeq.m_textPair.second.empty() )								// empty DEST?
	{	// avoid highlighting SRC text
	}
	else
		rMatchEffects[ str::MatchNotEqual ].Combine( SrcColumn == diffColumn ? m_pList->m_deleteSrc_DiffEffect : m_pList->m_mismatchDest_DiffEffect );

	rMatchEffects[ str::MatchEqualDiffCase ] = rMatchEffects[ str::MatchNotEqual ];
	ClearFlag( rMatchEffects[ str::MatchEqualDiffCase ].m_fontEffect, ui::Bold );
	SetFlag( rMatchEffects[ str::MatchEqualDiffCase ].m_fontEffect, ui::Underline );

	if ( m_pList->m_pTextEffectCallback != NULL )
		m_pList->m_pTextEffectCallback->ModifyDiffTextEffectAt( rMatchEffects, m_rowKey, m_subItem );		// resert ui::Bold, etc
}

COLORREF CReportListCustomDraw::GetRealizedTextColor( DiffColumn diffColumn, const str::TMatchSequence& cellSeq ) const
{
	if ( m_pList->HasItemState( m_index, LVIS_SELECTED ) && !m_pList->UseExplorerTheme() && ::GetFocus() == m_pList->m_hWnd )		// classic contrast selected item (not "Explorer" visual theme background)?
		return GetSysColor( COLOR_HIGHLIGHTTEXT );

	if ( str::MatchEqual == cellSeq.m_match && DestColumn == diffColumn )
		return m_pList->m_matchDest_DiffEffect.m_textColor;						// gray-out text of unmodified DEST files, but continue with default drawing

	return ui::GetActualColor( m_pList->m_listTextEffect.m_textColor, GetSysColor( COLOR_BTNTEXT ) );
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


#ifdef _DEBUG
#include "EnumTags.h"
#include "FlagTags.h"
#include "StringUtilities.h"
#endif //_DEBUG

namespace dbg
{
	const TCHAR* FormatDrawStage( DWORD dwDrawStage )
	{
	#ifdef _DEBUG
		static const CEnumTags enumTags( _T("CDDS_PREPAINT|CDDS_POSTPAINT|CDDS_PREERASE|CDDS_POSTERASE"), NULL, -1, CDDS_PREPAINT );		// mask 0x0000000F
		static const CFlagTags::FlagDef flagDefs[] =
		{
			{ CDDS_ITEM, _T("CDDS_ITEM") },
			{ CDDS_SUBITEM, _T("CDDS_SUBITEM") }
		};
		static const CFlagTags flagTags( flagDefs, COUNT_OF( flagDefs ) );
		static const TCHAR sep[] = _T(" | ");

		static std::tstring s_text;
		s_text = enumTags.FormatUi( dwDrawStage & 0x0000000F );
		stream::Tag( s_text, flagTags.FormatKey( dwDrawStage, sep ), sep );

		return s_text.c_str();
	#else
		dwDrawStage;
		return _T("");
	#endif //_DEBUG
	}
}
