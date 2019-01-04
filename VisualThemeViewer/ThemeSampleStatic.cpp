
#include "StdAfx.h"
#include "ThemeSampleStatic.h"
#include "Options.h"
#include "ThemeCustomDraw.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CThemeSampleStatic::CThemeSampleStatic( void )
	: CBufferedStatic()
	, m_pOptions( NULL )
	, m_margins( Margin, Margin )
	, m_coreSize( 0, 0 )
	, m_stcInfoId( 0 )
{
}

CThemeSampleStatic::~CThemeSampleStatic()
{
}

void CThemeSampleStatic::SetThemeItem( const CThemeItem& themeItem )
{
	m_themeItem = themeItem;
	RedrawWindow( NULL, NULL );
}

CRect CThemeSampleStatic::GetCoreRect( const CRect& clientRect ) const
{
	CSize coreSize = m_coreSize;
	if ( 0 == coreSize.cx )
		coreSize.cx = clientRect.Width() - 2 * m_margins.cx;
	if ( 0 == coreSize.cy )
		coreSize.cy = clientRect.Height() - 2 * m_margins.cy;

	ASSERT( coreSize.cx > 0 && coreSize.cy > 0 );
	CRect coreRect( 0, 0, coreSize.cx, coreSize.cy );
	ui::CenterRect( coreRect, clientRect, true, true );
	return coreRect;
}

CSize CThemeSampleStatic::ComputeIdealSize( void )
{
	return ComputeIdealTextSize();
}

bool CThemeSampleStatic::HasCustomFacet( void ) const
{
	return true;
}

void CThemeSampleStatic::DrawBackground( CDC* pDC, const CRect& clientRect )
{
	CBrush bkBrush( m_pOptions->GetBkColor() );
	pDC->FillRect( &clientRect, &bkBrush );
}

void CThemeSampleStatic::Draw( CDC* pDC, const CRect& clientRect )
{
	CRect coreRect = GetCoreRect( clientRect );

	if ( m_pOptions->m_useBorder )
	{
		CRect borderRect = coreRect;
		borderRect.InflateRect( 1, 1 );
		::FrameRect( *pDC, &borderRect, GetSysColorBrush( COLOR_WINDOWFRAME ) );
	}

	if ( InSizeToContentMode() )
		SizeToContent( coreRect, pDC );

	BYTE alpha = 96;
	if ( m_pOptions->m_preBkGuides )
		hlp::DrawGuides( pDC, coreRect, Color( alpha, 255, 0, 0 ) );

	if ( !m_themeItem.DrawBackground( *pDC, coreRect ) )
	{
		hlp::DrawError( pDC, coreRect );
		return;
	}

	if ( !m_sampleText.empty() )
	{
		CRect textRect = coreRect;
		textRect.DeflateRect( 5, 5 );
		m_themeItem.DrawText( *pDC, textRect, m_sampleText.c_str(), DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_WORD_ELLIPSIS );
	}

	if ( m_pOptions->m_postBkGuides )
		hlp::DrawGuides( pDC, coreRect, Color( alpha, 0, 0, 255 ) );
}

bool CThemeSampleStatic::SizeToContent( CRect& rCoreRect, CDC* pDC )
{
	CVisualTheme theme( m_themeItem.m_pThemeClass );
	CSize coreSize;
	std::tstring shrunkInfo;

	if ( theme.IsValid() )
		if ( theme.GetThemePartSize( &coreSize, *pDC, m_themeItem.m_partId, m_themeItem.GetStateId(), TS_TRUE, &rCoreRect ) )
			if ( coreSize != CSize( 0, 0 ) && coreSize != CSize( 1, 1 ) )		// ignore minuscule sizes
			{
				CRect coreRect( 0, 0, coreSize.cx, coreSize.cy );
				ui::CenterRect( coreRect, rCoreRect );

				{
					Graphics graphics( *pDC );
					SolidBrush brush( gp::MakeSysColor( COLOR_INACTIVEBORDER, 160 ) );
					graphics.FillRectangle( &brush, gp::ToRect( rCoreRect ) );
				}

				rCoreRect = coreRect;		// shrink core rect to auto size
				shrunkInfo = str::Format( _T("SHRUNK:\ncx=%d\ncy=%d"), rCoreRect.Width(), rCoreRect.Height() );

				DrawBackground( pDC, rCoreRect );

				if ( m_pOptions->m_useBorder )
				{
					coreRect.InflateRect( 1, 1 );
					ui::FrameRect( *pDC, coreRect, color::ScarletRed );
				}
			}

	ui::SetDlgItemText( GetParent(), m_stcInfoId, shrunkInfo );
	return !shrunkInfo.empty();
}


// message handlers

BEGIN_MESSAGE_MAP( CThemeSampleStatic, CBufferedStatic )
END_MESSAGE_MAP()
