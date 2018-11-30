
#include "stdafx.h"
#include "ThemeCustomDraw.h"
#include "ThemeStore.h"
#include "ThemeSampleStatic.h"
#include "utl/VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	void DrawError( CDC* pDC, const CRect& coreRect )
	{
		enum { Pink = RGB( 254, 204, 204 ) };
		CBrush errorBrush( Pink );
		pDC->FillRect( &coreRect, &errorBrush );		// error colour

		Graphics graphics( *pDC );
		HatchBrush brush( HatchStyleBackwardDiagonal, Color( 128, 200, 0, 0 ), Color( 0, 0, 0, 0 ) );
		graphics.FillRectangle( &brush, gp::ToRect( coreRect ) );
	}

	void DrawFrameGuides( CDC* pDC, CRect coreRect, Color guideColor )
	{
		Graphics graphics( *pDC );

		Rect rect = gp::ToRect( coreRect );
		Pen pen( guideColor );
		gp::FrameRectangle( graphics, rect, &pen );
	}

	void DrawGuides( CDC* pDC, CRect coreRect, Color guideColor )
	{
		Graphics graphics( *pDC );

		Rect rect = gp::ToRect( coreRect );
		Pen pen( guideColor );
		gp::FrameRectangle( graphics, rect, &pen );

		coreRect.DeflateRect( 1, 1, 2, 2 );
		Point center = gp::ToPoint( coreRect.CenterPoint() );

		Point vertPoints[] = { Point( center.X, coreRect.top ), Point( center.X, coreRect.bottom ) };
		graphics.DrawLines( &pen, vertPoints, COUNT_OF( vertPoints ) );

		Point horizPoints[] = { Point( coreRect.left, center.Y ), Point( coreRect.right, center.Y ) };
		graphics.DrawLines( &pen, horizPoints, COUNT_OF( horizPoints ) );
	}

	bool HasBitmap( const CThemeItem& themeItem )
	{
		CVisualTheme theme( themeItem.m_pThemeClass );
		if ( theme.IsValid() )
		{
			HBITMAP hBitmap = NULL;
			if ( HR_OK( ::GetThemeBitmap( theme.GetTheme(), themeItem.m_partId, themeItem.GetStateId(), TMT_GLYPHDIBDATA, GBF_DIRECT, &hBitmap ) ) )
				if ( hBitmap != NULL )
					return true;
		}
		return false;
	}

	bool HasGlyph( const CThemeItem& themeItem )
	{
		CVisualTheme theme( themeItem.m_pThemeClass );
		if ( theme.IsValid() )
		{
			int value;
			if ( HR_OK( ::GetThemeEnumValue( theme.GetTheme(), themeItem.m_partId, themeItem.GetStateId(), TMT_GLYPHTYPE, &value ) ) )
				switch ( value )
				{
					case GT_IMAGEGLYPH:
					case GT_FONTGLYPH:
						return true;
				}

			if ( HR_OK( ::GetThemeEnumValue( theme.GetTheme(), themeItem.m_partId, themeItem.GetStateId(), TMT_SIZINGTYPE, &value ) ) )
				switch ( value )
				{
					case ST_TRUESIZE:
						return true;
				}

			if ( HR_OK( ::GetThemeBool( theme.GetTheme(), themeItem.m_partId, themeItem.GetStateId(), TMT_GLYPHTRANSPARENT, &value ) ) )
				if ( value != FALSE )
					return true;

			if ( HR_OK( ::GetThemeBool( theme.GetTheme(), themeItem.m_partId, themeItem.GetStateId(), TMT_GLYPHONLY, &value ) ) )
				if ( value != FALSE )
					return true;
		}

		return HasBitmap( themeItem );
	}

	bool UseText( const CThemeItem& themeItem )
	{
		return !HasGlyph( themeItem );
	}
}


CSize CThemeCustomDraw::GetItemImageSize( ui::GlyphGauge glyphGauge /*= ui::SmallGlyph*/ ) const
{
	glyphGauge;
	ASSERT( false );		// shouldn't be called since the ctrl drives bounds size
	return m_boundsSize;
}

bool CThemeCustomDraw::SetItemImageSize( const CSize& boundsSize )
{	// call when UI control drives image bounds size
	if ( m_boundsSize == boundsSize )
		return false;

	ASSERT( boundsSize.cx >= 16 && boundsSize.cy >= 16 );
	m_boundsSize = boundsSize;
	return true;
}

bool CThemeCustomDraw::DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemRect )
{
	const IThemeNode* pThemeNode = checked_static_cast< const IThemeNode* >( pSubject );
	ASSERT_PTR( pThemeNode );
	CThemeItem themeItem = pThemeNode->MakeThemeItem();

	CRect rect = itemRect;
	rect.DeflateRect( 0, 1 );
	if ( !themeItem.DrawBackground( *pDC, rect ) )
	{
		hlp::DrawError( pDC, rect );
		return false;
	}

	if ( !m_itemCaption.empty() )
		if ( hlp::UseText( themeItem ) )
		{
			CRect textRect = rect;
			textRect.left += 5;
			themeItem.DrawText( *pDC, textRect, m_itemCaption.c_str(), DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_WORD_ELLIPSIS );
		}

	if ( m_pOptions->m_postBkGuides )
		hlp::DrawFrameGuides( pDC, itemRect, Color( 96, 0, 0, 255 ) );

	return true;
}
