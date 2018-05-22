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


	inline BYTE GetLuminance( BYTE red, BYTE green, BYTE blue )
	{	// luminance (gray level) of a color (Y component) = 0.3*R + 0.59*G + 0.11*B
		if ( red == green && red == blue )
			return red;							// preserve grays unchanged

		UINT luminance = ( (UINT)red * 30 + green * 59 + blue * 11 ) / 100;
		return static_cast< BYTE >( luminance );
	}

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
	// accurate colour algorithms

	BYTE GetAverageComponent( UINT component1, UINT component2 );
	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2 );
	inline COLORREF& BlendWithColor( COLORREF& rColor1, COLORREF color2 ) { return rColor1 = GetBlendedColor( rColor1, color2 ); }


	struct CHslColor
	{
	public:
		CHslColor( void ) : m_hue( 0 ), m_luminance( 0 ), m_saturation( 100 ) {}
		CHslColor( WORD hue, WORD luminance, WORD saturation ) : m_hue( hue ), m_luminance( luminance ), m_saturation( saturation ) {}
		CHslColor( COLORREF rgbColor );

		bool IsValid( void ) const { return s_validRange.Contains( m_hue ) && s_validRange.Contains( m_luminance ) && s_validRange.Contains( m_saturation ); }
		COLORREF GetRGB( void ) const;

		CHslColor& ScaleHue( int byPct ) { m_hue = ModifyBy( m_hue, byPct ); return *this; }
		CHslColor& ScaleLuminance( int byPct ) { m_luminance = ModifyBy( m_luminance, byPct ); return *this; }
		CHslColor& ScaleSaturation( int byPct ) { m_saturation = ModifyBy( m_saturation, byPct ); return *this; }

		static WORD ModifyBy( WORD component, int byPercentage );
	public:
		WORD m_hue;
		WORD m_luminance;
		WORD m_saturation;

		static const Range< WORD > s_validRange;
	};
}


#endif // Color_h
