
#include "stdafx.h"
#include "ReportListCustomDraw.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CReportListCustomDraw::CReportListCustomDraw( NMLVCUSTOMDRAW* pDraw, CReportListControl* pList )
	: m_pDraw( safe_ptr( pDraw ) )
	, m_pList( safe_ptr( pList ) )
	, m_pDC( NULL )
	, m_index( static_cast< int >( m_pDraw->nmcd.dwItemSpec ) )
	, m_subItem( m_pDraw->iSubItem )
	, m_rowKey( m_pDraw->nmcd.lItemlParam != 0 ? m_pDraw->nmcd.lItemlParam : m_index )
	, m_pObject( CReportListControl::ToSubject( m_pDraw->nmcd.lItemlParam ) )
{
}

bool CReportListCustomDraw::IsTooltipDraw( const NMLVCUSTOMDRAW* pDraw )
{
	ASSERT_PTR( pDraw );
	static const CRect s_emptyRect( 0, 0, 0, 0 );
	return ( s_emptyRect == pDraw->nmcd.rc ) != FALSE;
}

CDC* CReportListCustomDraw::EnsureDC( void )
{
	if ( NULL == m_pDC )
		m_pDC = CDC::FromHandle( m_pDraw->nmcd.hdc );
	return m_pDC;
}

bool CReportListCustomDraw::ApplyCellTextEffect( void )
{
	ui::CTextEffect textEffect = m_pList->m_listTextEffect;							// start with the global list effect
	textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, EntireRecord ) );	// assign entire record

	if ( m_subItem != EntireRecord )
		textEffect.AssignPtr( m_pList->FindTextEffectAt( m_rowKey, m_subItem ) );	// assign individual cell

	if ( m_pList->m_pTextEffectCallback != NULL )
		m_pList->m_pTextEffectCallback->CombineTextEffectAt( textEffect, m_rowKey, m_subItem );		// combine with callback effect (additive operation)

	if ( m_pList->m_useAlternateRowColoring )
		if ( HasFlag( m_index, 0x01 ) )
			if ( CLR_NONE == textEffect.m_bkColor )
				textEffect.m_bkColor = color::GhostWhite;

	return ApplyTextEffect( textEffect );
}

bool CReportListCustomDraw::ApplyTextEffect( const ui::CTextEffect& textEffect )
{
	// handle even if textEffect.IsNull() since it must reset previous cell effects

	bool modified = false;

	if ( HGDIOBJ hFont = m_pList->GetFontEffectCache()->Lookup( textEffect.m_fontEffect )->GetSafeHandle() )
		if ( ::SelectObject( *m_pDC, hFont ) != hFont )
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

ui::CTextEffect CReportListCustomDraw::ExtractTextEffects( void ) const
{
	ui::CTextEffect textEffect;

	if ( CFont* pCurrentFont = m_pDC->GetCurrentFont() )
	{
		LOGFONT logFont;
		memset( &logFont, 0, sizeof( LOGFONT ) );
		if ( pCurrentFont->GetLogFont( &logFont ) != 0 )
		{
			SetFlag( textEffect.m_fontEffect, ui::Bold, FW_BOLD == logFont.lfWeight );
			SetFlag( textEffect.m_fontEffect, ui::Italic, TRUE == logFont.lfItalic );
			SetFlag( textEffect.m_fontEffect, ui::Underline, TRUE == logFont.lfUnderline );
		}
	}

	textEffect.m_textColor = m_pDraw->clrText;
	textEffect.m_bkColor = m_pDraw->clrTextBk;
	return textEffect;
}

CRect CReportListCustomDraw::MakeItemTextRect( void ) const
{
	CRect textRect = m_pDraw->nmcd.rc;

	// requires larger spacing for sub-item: 2 for item, 6 for sub-item
	textRect.left += ( 0 == m_subItem ? CReportListControl::ItemSpacingX : CReportListControl::SubItemSpacingX );
	return textRect;
}
