
#include "StdAfx.h"
#include "ThemeSampleStatic.h"
#include "utl/Color.h"
#include "utl/HistoryComboBox.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/VisualTheme.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("MainDialog\\ThemeSample");
	static const TCHAR entry_enableThemes[] = _T("EnableThemes");
	static const TCHAR entry_enableThemesFallback[] = _T("EnableThemesFallback");
}


// CThemeSampleOptions implementation

CThemeSampleOptions::CThemeSampleOptions( ISampleOptionsCallback* pCallback )
	: reg::COptionContainer( reg::section )
	, m_pCallback( pCallback )
	, m_useBorder( true )
	, m_preBkGuides( false )
	, m_postBkGuides( false )
	, m_showThemeGlyphs( true )
	, m_enableThemes( *CVisualTheme::GetEnabledPtr() )						// acts directly on target bool (by reference)
	, m_enableThemesFallback( *CVisualTheme::GetFallbackEnabledPtr() )		// acts directly by target bool (by reference)
{
	AddOption( MAKE_OPTION( &m_bkColorText ) );
	AddOption( MAKE_OPTION( &m_useBorder ), IDC_USE_BORDER_CHECK );
	AddOption( MAKE_OPTION( &m_preBkGuides ), IDC_PRE_BK_GUIDES_CHECK );
	AddOption( MAKE_OPTION( &m_postBkGuides ), IDC_POST_BK_GUIDES_CHECK );
	AddOption( MAKE_OPTION( &m_showThemeGlyphs ), ID_SHOW_THEME_GLYPHS_CHECK );
	AddOption( MAKE_OPTION( &m_enableThemes ), IDC_ENABLE_THEMES_CHECK );
	AddOption( MAKE_OPTION( &m_enableThemesFallback ), IDC_ENABLE_THEMES_FALLBACK_CHECK );

	LoadOptions();
}

CThemeSampleOptions::~CThemeSampleOptions()
{
	SaveOptions();
}

COLORREF CThemeSampleOptions::GetBkColor( void ) const
{
	if ( m_bkColorText.empty() )
		return GetSysColor( COLOR_BTNFACE );

	enum { Salmon = RGB( 255, 145, 164 ) };
	COLORREF bkColor;
	return ui::ParseColor( &bkColor, m_bkColorText ) ? bkColor : Salmon;
}

CHistoryComboBox* CThemeSampleOptions::GetBkColorCombo( void ) const
{
	return static_cast< CHistoryComboBox* >( m_pCallback->GetWnd()->GetDlgItem( IDC_BK_COLOR_COMBO ) );
}


BEGIN_MESSAGE_MAP( CThemeSampleOptions, COptionContainer )
	ON_CBN_EDITCHANGE( IDC_BK_COLOR_COMBO, OnChange_BkColor )
	ON_CBN_SELCHANGE( IDC_BK_COLOR_COMBO, OnChange_BkColor )
END_MESSAGE_MAP()

void CThemeSampleOptions::OnToggle_BoolOption( UINT cmdId )
{
	__super::OnToggle_BoolOption( cmdId );

	switch ( cmdId )
	{
		case IDC_ENABLE_THEMES_CHECK:
		case IDC_ENABLE_THEMES_FALLBACK_CHECK:
			m_pCallback->GetWnd()->Invalidate();		// redraw the resize gripper
			break;
	}

	m_pCallback->RedrawSamples();
}

void CThemeSampleOptions::OnChange_BkColor( void )
{
	CHistoryComboBox* pCombo = GetBkColorCombo();
	std::tstring bkColorText = ui::GetComboSelText( *pCombo );

	COLORREF bkColor;
	if ( bkColorText.empty() || ui::ParseColor( &bkColor, bkColorText ) )
	{
		m_bkColorText = bkColorText;
		m_pCallback->RedrawSamples();
		pCombo->SetFrameColor( CLR_NONE );
	}
	else
		pCombo->SetFrameColor( RGB( 255, 0, 0 ) );
}


// CThemeSampleStatic implementation

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
		DrawGuides( pDC, coreRect, Color( alpha, 255, 0, 0 ) );

	if ( !m_themeItem.DrawBackground( *pDC, coreRect ) )
	{
		DrawError( pDC, coreRect );
		return;
	}

	if ( !m_sampleText.empty() )
	{
		CRect textRect = coreRect;
		textRect.DeflateRect( 5, 5 );
		m_themeItem.DrawText( *pDC, textRect, m_sampleText.c_str(), DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_WORD_ELLIPSIS );
	}

	if ( m_pOptions->m_postBkGuides )
		DrawGuides( pDC, coreRect, Color( alpha, 0, 0, 255 ) );
}

void CThemeSampleStatic::DrawGuides( CDC* pDC, CRect coreRect, Color guideColor )
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

void CThemeSampleStatic::DrawError( CDC* pDC, const CRect& coreRect )
{
	enum { Pink = RGB( 254, 204, 204 ) };
	CBrush errorBrush( Pink );
	pDC->FillRect( &coreRect, &errorBrush );		// error colour

	Graphics graphics( *pDC );
    HatchBrush brush( HatchStyleBackwardDiagonal, Color( 128, 200, 0, 0 ), Color( 0, 0, 0, 0 ) );
	graphics.FillRectangle( &brush, gp::ToRect( coreRect ) );
}

bool CThemeSampleStatic::SizeToContent( CRect& rCoreRect, CDC* pDC )
{
	CVisualTheme theme( m_themeItem.m_pThemeClass );
	CSize coreSize;
	std::tstring shrunkLabel;

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
				shrunkLabel = str::Format( _T("SHRUNK:\ncx=%d\ncy=%d"), rCoreRect.Width(), rCoreRect.Height() );

				DrawBackground( pDC, rCoreRect );

				if ( m_pOptions->m_useBorder )
				{
					coreRect.InflateRect( 1, 1 );
					ui::FrameRect( *pDC, coreRect, color::ScarletRed );
				}
			}

	ui::SetDlgItemText( GetParent(), m_stcInfoId, shrunkLabel );
	return !shrunkLabel.empty();
}


// message handlers

BEGIN_MESSAGE_MAP( CThemeSampleStatic, CBufferedStatic )
END_MESSAGE_MAP()
