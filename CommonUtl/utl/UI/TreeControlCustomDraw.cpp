
#include "stdafx.h"
#include "TreeControlCustomDraw.h"
#include "TreeControl.h"
#include "Color.h"
#include "GpUtilities.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTreeControlCustomDraw::CTreeControlCustomDraw( NMTVCUSTOMDRAW* pDraw, CTreeControl* pTree )
	: CListLikeCustomDrawBase( &pDraw->nmcd )
	, m_pDraw( pDraw )
	, m_pTree( safe_ptr( pTree ) )
	, m_hItem( reinterpret_cast<HTREEITEM>( m_pDraw->nmcd.dwItemSpec ) )
	, m_pObject( CTreeControl::ToSubject( m_pDraw->nmcd.lItemlParam ) )
{
}

bool CTreeControlCustomDraw::ApplyItemTextEffect( void )
{
	if ( s_useDefaultDraw )
		return false;

	return ApplyEffect( MakeItemEffect() );
}

ui::CTextEffect CTreeControlCustomDraw::MakeItemEffect( void ) const
{
	ui::CTextEffect textEffect = m_pTree->m_ctrlTextEffect;					// start with the global text effect
	if ( m_pTree->HasItemState( m_hItem, TVIS_BOLD ) )
		SetFlag( textEffect.m_fontEffect, ui::Bold );

	textEffect.AssignPtr( m_pTree->FindTextEffect( m_hItem ) );				// assign individual item

	m_pTree->CombineTextEffectAt( textEffect, (LPARAM)m_hItem, 0, m_pTree );	// combine with callback effect (additive operation)
	return textEffect;
}

bool CTreeControlCustomDraw::ApplyEffect( const ui::CTextEffect& textEffect )
{
	// handle even if textEffect.IsNull() since it may need to reset previous cell effects
	bool modified = false;

	if ( CFont* pFont = m_pTree->GetFontEffectCache()->Lookup( textEffect.m_fontEffect ) )
		if ( m_pDC->SelectObject( pFont ) != pFont )
			modified = true;

	// CTreeCtrl: when assigning CLR_NONE, the tree view doesn't use the default colour properly: this->GetTextColor(), this->GetBkColor(). Whereas CListCtrl does.
	// Therefore we don't assign default CLR_NONE colours.
	if ( textEffect.m_textColor != m_pDraw->clrText && ui::IsActualColor( textEffect.m_textColor ) )
	{
		m_pDraw->clrText = textEffect.m_textColor;
		modified = true;
	}

	if ( textEffect.m_bkColor != m_pDraw->clrTextBk && ui::IsActualColor( textEffect.m_bkColor ) )
	{
		m_pDraw->clrTextBk = textEffect.m_bkColor;
		modified = true;
	}

	return modified;
}

COLORREF CTreeControlCustomDraw::GetRealizedBkColor( void ) const
{
	if ( IsSelItemContrast() )
		return GetSysColor( COLOR_HIGHLIGHT );

	if ( ui::IsActualColor( m_pDraw->clrTextBk ) )
		return m_pDraw->clrTextBk;

	return ui::GetActualColorSysdef( m_pTree->GetBkColor(), COLOR_WINDOW );
}

bool CTreeControlCustomDraw::SelectTextEffect( const ui::CTextEffect& textEffect )
{
	bool modified = false;

	if ( CFont* pFont = m_pTree->GetFontEffectCache()->Lookup( textEffect.m_fontEffect ) )
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

bool CTreeControlCustomDraw::IsSelItemContrast( void ) const
{
	// classic contrast selected item: blue backgound with white text (not "Explorer" visual theme background)?
	return m_pTree->HasItemState( m_hItem, LVIS_SELECTED ) && !m_pTree->GetUseExplorerTheme() && ::GetFocus() == m_pTree->m_hWnd;
}
