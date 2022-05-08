#ifndef Color_h
#define Color_h
#pragma once

#include "StdColors.h"
#include "Range.h"


#define HTML_COLOR( htmlLiteral ) ( ui::ParseHtmlLiteral( #htmlLiteral ) )


namespace ui
{
	enum StdTranspColor { Transp_LowColor = color::LightGrey, Transp_TrueColor = color::ToolStripPink };

	inline COLORREF GetStdTranspColor( WORD bitsPerPixel ) { return bitsPerPixel >= 24 ? Transp_TrueColor : Transp_LowColor; }

	COLORREF ParseHtmlLiteral( const char* pHtmlLiteral );		// quick and less safe parsing

	std::tstring FormatRgb( COLORREF color );
	std::tstring FormatHtml( COLORREF color );

	bool ParseRgb( COLORREF* pColor, const std::tstring& text );
	bool ParseHtml( COLORREF* pColor, const std::tstring& text );
	bool ParseSys( COLORREF* pColor, const std::tstring& text );

	std::tstring FormatColor( COLORREF color, const TCHAR* pSep = _T("  ") );
	bool ParseColor( COLORREF* pColor, const std::tstring& text );


	inline BYTE GetColorFlags( COLORREF color ) { return LOBYTE( color >> 24 ); }				// highest BYTE

	inline bool IsActualColor( COLORREF color ) { return 0 == GetColorFlags( color ); }			// not CLR_NONE, CLR_DEFAULT, other
	inline COLORREF GetActualColor( COLORREF color, COLORREF defaultColor ) { return IsActualColor( color ) ? color : defaultColor; }
	inline COLORREF GetActualColorSysdef( COLORREF color, int defaultSysIndex ) { return IsActualColor( color ) ? color : ::GetSysColor( defaultSysIndex ); }


	BYTE GetLuminance( UINT red, UINT green, UINT blue );

	inline BYTE GetLuminance( COLORREF color )
	{
		return GetLuminance( GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}

	inline COLORREF GetContrastColor( COLORREF bkColor, COLORREF darkColor = color::Black, COLORREF lightColor = color::White )
	{
		return GetLuminance( bkColor ) < 128 ? lightColor : darkColor;
	}

	inline COLORREF GetMappedColor( COLORREF color, CDC* pDC )
	{
		// CDC::GetNearestColor() doesn't produce the expected result for a DIB selected in it.
		// we need to call SetPixel to get the right colour mapping, which is equivalent to BitBlt() colour mapping.
		COLORREF originColor = pDC->GetPixel( 0, 0 );			// save pixel at origin
		COLORREF mappedColor = pDC->SetPixel( 0, 0, color );
		pDC->SetPixel( 0, 0, originColor );						// restore pixel at origin
		return mappedColor;
	}
}


namespace ui
{
	// accurate colour algorithms (using geometric averaging)

	BYTE GetAverageComponent( UINT comp1, UINT comp2 );
	BYTE GetAverageComponent( UINT comp1, UINT comp2, UINT comp3 );
	BYTE GetWeightedMixComponent( UINT comp1, UINT comp2, double weight1 );		// weight1 from 0.0 to 1.0

	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2 );
	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2, COLORREF color3 );
	COLORREF GetWeightedMixColor( COLORREF color1, COLORREF color2, int pct1 );

	inline COLORREF& BlendWithColor( COLORREF& rColor1, COLORREF color2 ) { return rColor1 = GetBlendedColor( rColor1, color2 ); }
	inline COLORREF& WeightedMixWithColor( COLORREF& rColor1, COLORREF color2, int pct1 ) { return rColor1 = GetWeightedMixColor( rColor1, color2, pct1 ); }


	// HSL adjustments

	COLORREF GetAdjustLuminance( COLORREF color, int byPct );
	COLORREF GetAdjustSaturation( COLORREF color, int byPct );
	COLORREF GetAdjustHue( COLORREF color, int byPct );

	inline COLORREF& AdjustLuminance( COLORREF& rColor, int byPct ) { return rColor = GetAdjustLuminance( rColor, byPct ); }
	inline COLORREF& AdjustSaturation( COLORREF& rColor, int byPct ) { return rColor = GetAdjustSaturation( rColor, byPct ); }
	inline COLORREF& AdjustHue( COLORREF& rColor, int byPct ) { return rColor = GetAdjustHue( rColor, byPct ); }


	struct CHslColor
	{
	public:
		CHslColor( void ) : m_hue( 0 ), m_saturation( 100 ), m_luminance( 0 ) {}
		CHslColor( WORD hue, WORD saturation, WORD luminance ) : m_hue( hue ), m_saturation( saturation ), m_luminance( luminance ) {}
		CHslColor( COLORREF rgbColor );

		bool IsValid( void ) const { return s_validRange.Contains( m_hue ) && s_validRange.Contains( m_saturation ) && s_validRange.Contains( m_luminance ); }
		COLORREF GetRGB( void ) const;

		CHslColor& ScaleHue( int byPct ) { m_hue = ModifyBy( m_hue, byPct ); return *this; }
		CHslColor& ScaleSaturation( int byPct ) { m_saturation = ModifyBy( m_saturation, byPct ); return *this; }
		CHslColor& ScaleLuminance( int byPct ) { m_luminance = ModifyBy( m_luminance, byPct ); return *this; }

		CHslColor GetScaledHue( int byPct ) { CHslColor newColor = *this; return newColor.ScaleHue( byPct ); }
		CHslColor GetScaledSaturation( int byPct ) { CHslColor newColor = *this; return newColor.ScaleSaturation( byPct ); }
		CHslColor GetScaledLuminance( int byPct ) { CHslColor newColor = *this; return newColor.ScaleLuminance( byPct ); }

		static WORD ModifyBy( WORD component, int byPercentage );
	public:
		WORD m_hue;
		WORD m_saturation;
		WORD m_luminance;

		static const Range<WORD> s_validRange;		// [0, 240]
	};
}


namespace ui
{
	struct CColorAlpha
	{
		CColorAlpha( void ) : m_color( CLR_NONE ), m_alpha( 0xFF ) {}
		CColorAlpha( COLORREF color, BYTE alpha ) : m_color( color ), m_alpha( alpha ) {}
		CColorAlpha( BYTE red, BYTE green, BYTE blue, BYTE alpha ) : m_color( RGB( red, green, blue ) ), m_alpha( alpha ) {}

		bool IsNull( void ) const { return CLR_NONE == m_color || IsDefault(); }
		bool IsDefault( void ) const { return CLR_DEFAULT == m_color; }

		static CColorAlpha MakeSysColor( int sysColorIndex, BYTE alpha = 0xFF ) { return CColorAlpha( ::GetSysColor( sysColorIndex ), alpha ); }
		static CColorAlpha MakeTransparent( COLORREF color, UINT transpPct ) { return CColorAlpha( color, MakeAlpha( transpPct ) ); }
		static CColorAlpha MakeOpaqueColor( COLORREF color, UINT opacityPct ) { return CColorAlpha( color, MakeAlpha( 100 - opacityPct ) ); }

		static BYTE FromPercentage( UINT percentage ) { ASSERT( percentage <= 100 ); return static_cast<BYTE>( (double)percentage * 255 / 100 ); }
		static BYTE MakeAlpha( UINT transpPct ) { return FromPercentage( 100 - transpPct ); }
	public:
		COLORREF m_color;
		BYTE m_alpha;
	};
}


#endif // Color_h
