
#include "stdafx.h"
#include "ThemeCustomDraw.h"
#include "ThemeStore.h"
#include "Options.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/VisualTheme.h"

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
}


// CThemeCustomDraw implementation

const CSize CThemeCustomDraw::s_themePreviewSize( 40, 20 );
const CSize CThemeCustomDraw::s_themePreviewSizeLarge( 50, 32 );

CThemeCustomDraw::CThemeCustomDraw( const COptions* pOptions, const std::tstring& itemCaption /*= _T("text")*/ )
	: m_pOptions( pOptions )
	, m_boundsSize( 0, 0 )
	, m_imageMargin( 0, 1 )
	, m_textMargin( 0 )
	, m_itemCaption( itemCaption )
{
	m_imageSize[ ui::SmallGlyph ] = CSize( 16, 16 );
	m_imageSize[ ui::LargeGlyph ] = CSize( 32, 32 );

	ASSERT_PTR( m_pOptions );
}

CThemeCustomDraw* CThemeCustomDraw::MakeListCustomDraw( const COptions* pOptions )
{
	CThemeCustomDraw* pListCustomDraw = new CThemeCustomDraw( pOptions );

	pListCustomDraw->m_imageSize[ ui::SmallGlyph ] = s_themePreviewSize;
	pListCustomDraw->m_imageSize[ ui::LargeGlyph ] = s_themePreviewSizeLarge;
	pListCustomDraw->m_imageMargin = CSize( 0, 2 );
	pListCustomDraw->m_textMargin = 2;
	return pListCustomDraw;
}

CThemeCustomDraw* CThemeCustomDraw::MakeTreeCustomDraw( const COptions* pOptions )
{
	CThemeCustomDraw* pTreeCustomDraw = new CThemeCustomDraw( pOptions );

	pTreeCustomDraw->m_imageSize[ ui::SmallGlyph ] = s_themePreviewSize;
	pTreeCustomDraw->m_imageMargin = CSize( 0, 2 );
	pTreeCustomDraw->m_textMargin = 2;
	return pTreeCustomDraw;
}

CSize CThemeCustomDraw::GetItemImageSize( ui::GlyphGauge glyphGauge /*= ui::SmallGlyph*/ ) const
{	// called when this drives bounds size (default, more accurate for tweaking margins for list vs tree)
	m_boundsSize = m_imageSize[ glyphGauge ];
	m_boundsSize.cx += m_imageMargin.cx * 2;
	m_boundsSize.cy += m_imageMargin.cy * 2;
	m_boundsSize.cx += m_textMargin;
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
	const CBaseNode* pThemeNode = checked_static_cast< const CBaseNode* >( pSubject );
	ASSERT_PTR( pThemeNode );
	CThemeItemNode themeItem = pThemeNode->MakeThemeItem();
	int nodeFlags = themeItem.m_pDeepNode->GetFlags();

	CRect rect = itemRect;
	rect.DeflateRect( m_imageMargin );
	rect.right -= m_textMargin;

	if ( HasFlag( nodeFlags, PreviewFillBkFlag ) )
		::FillRect( pDC->m_hDC, &rect, GetSysColorBrush( COLOR_BTNFACE ) );

	CRect bkRect = rect;
	if ( HasFlag( nodeFlags, SquareContentFlag | ShrinkFitContentFlag ) )
	{
		CSize partSize( 0, 0 );

		if ( HasFlag( nodeFlags, SquareContentFlag ) )
		{
			partSize = bkRect.Size();
			partSize.cx = partSize.cy = std::min( partSize.cx, partSize.cy );
		}
		else if ( HasFlag( nodeFlags, ShrinkFitContentFlag ) )
			themeItem.GetPartSize( &partSize, pDC->m_hDC, TS_TRUE, &rect );

		if ( partSize.cx * partSize.cy > 2 )						// not a minuscule size?
		{
			partSize = ui::MinSize( partSize, rect.Size() );		// limit bkRect to rect
			bkRect.SetRect( 0, 0, partSize.cx, partSize.cy );
			ui::CenterRect( bkRect, rect );
		}
	}

	if ( !themeItem.DrawBackground( *pDC, bkRect ) )
	{
		hlp::DrawError( pDC, rect );
		return false;
	}

	if ( !m_itemCaption.empty() )
		if ( HasFlag( nodeFlags, TextFlag ) )
		{
			CRect textRect = rect;
			textRect.left += TextMargin;
			themeItem.DrawText( *pDC, textRect, m_itemCaption.c_str(), DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_WORD_ELLIPSIS );
		}

	if ( m_pOptions->m_postBkGuides )
	{
		rect.InflateRect( 1, 1 );
		hlp::DrawFrameGuides( pDC, rect, Color( 96, 0, 0, 255 ) );
	}

	return true;
}
