
#include "stdafx.h"
#include "Color.h"
#include "StringUtilities.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	bool ParseDecDigit( BYTE* pValue, const std::tstring& decString )
	{
		ASSERT_PTR( pValue );
		std::tistringstream iss( decString );
		int value = -1;
		iss >> value;
		if ( iss.fail() || value < 0 || value > 0xFF )
			return false;
		*pValue = static_cast<BYTE>( value );
		return true;
	}

	bool ParseHexDigit( BYTE* pValue, const std::tstring& hexString, int pos )
	{
		ASSERT_PTR( pValue );
		enum { HexDigitLength = 2 };
		std::tistringstream iss( hexString.substr( pos, HexDigitLength ) );
		int value = -1;
		iss >> std::hex >> value;
		if ( iss.fail() || value < 0 || value > 0xFF )
			return false;
		*pValue = static_cast<BYTE>( value );
		return true;
	}
}


namespace ui
{
	BYTE GetLuminance( UINT red, UINT green, UINT blue )
	{	// perceived luminance (gray level) of a color (Y component) = 0.3*R + 0.59*G + 0.11*B
		UINT luminance;
		if ( red == green && red == blue )
			luminance = red;					// preserve grays unchanged
		else
			luminance = ( red * 30 + green * 59 + blue * 11 ) / 100;

		ASSERT( luminance <= 255 );
		return static_cast<BYTE>( luminance );
	}
}


namespace ui
{
	const std::tstring nullTag = _T("null");
	const TCHAR rgbTag[] = _T("RGB");
	const TCHAR htmlTag[] = _T("#");
	const TCHAR sysTag[] = _T("SYS");


	COLORREF ParseHtmlLiteral( const char* pHtmlLiteral )
	{
		// quick and less safe parsing
		// HTML color format: #RRGGBB - "#FF0000" for bright red
		ASSERT( pHtmlLiteral != nullptr && '#' == pHtmlLiteral[ 0 ] && 7 == strlen( pHtmlLiteral ) );

		const char digits[] =
		{
			*++pHtmlLiteral, *++pHtmlLiteral, '-',
			*++pHtmlLiteral, *++pHtmlLiteral, '-',
			*++pHtmlLiteral, *++pHtmlLiteral, '\0'
		};

		unsigned int red, green, blue;
		if ( sscanf( digits, "%x-%x-%x", &red, &green, &blue ) != 3 )
		{
			ASSERT( false );		// ill formed HTML colour literal
			return CLR_NONE;
		}

		return RGB( red, green, blue );
	}

	std::tstring FormatRgb( COLORREF color )
	{
		return str::Format( _T("%s(%u, %u, %u)"), rgbTag, GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}

	std::tstring FormatHtml( COLORREF color )
	{
		return str::Format( _T("%s%02X%02X%02X"), htmlTag, GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}


	bool ParseRgb( COLORREF* pColor, const std::tstring& text )
	{
		ASSERT_PTR( pColor );
		size_t pos = text.find( rgbTag );

		if ( pos != std::tstring::npos )
		{	// RGB( 255, 0, 0 ) color format for bright red
			std::vector< std::tstring > tokens;
			str::Tokenize( tokens, &text[ pos + 3 ], _T("(,) \t") );

			BYTE r, g, b;
			if ( tokens.size() >= 3 )
				if ( hlp::ParseDecDigit( &r, tokens[ 0 ] ) && hlp::ParseDecDigit( &g, tokens[ 1 ] ) && hlp::ParseDecDigit( &b, tokens[ 2 ] ) )
				{
					*pColor = RGB( r, g, b );
					return true;
				}
		}
		return false;
	}

	bool ParseHtml( COLORREF* pColor, const std::tstring& text )
	{
		ASSERT_PTR( pColor );
		size_t posTag = text.find( htmlTag );
		if ( posTag != std::tstring::npos )
		{	// HTML color format: "#FF0000" for bright red
			enum { PosR = 0, PosG = 2, PosB = 4, MinLength = 6 };
			int pos = static_cast<int>( posTag + 1 );
			BYTE r, g, b;

			if ( str::GetLength( &text[ pos ] ) >= MinLength &&
				 hlp::ParseHexDigit( &r, text, pos + PosR ) &&
				 hlp::ParseHexDigit( &g, text, pos + PosG ) &&
				 hlp::ParseHexDigit( &b, text, pos + PosB ) )
			{
				*pColor = RGB( r, g, b );
				return true;
			}
		}
		return false;
	}

	std::tstring FormatColor( COLORREF color, const TCHAR* pSep /*= _T("  ")*/ )
	{
		if ( CLR_NONE == color )
			return nullTag;

		std::tostringstream oss;
		oss << FormatHtml( color ) << pSep << FormatRgb( color );
		return oss.str();
	}

	bool ParseColor( COLORREF* pColor, const std::tstring& text )
	{
		ASSERT_PTR( pColor );
		if ( text.find( htmlTag ) != std::tstring::npos && ParseHtml( pColor, text ) )
			return true;

		if ( text.find( rgbTag ) != std::tstring::npos && ParseRgb( pColor, text ) )
			return true;

		if ( text.find( nullTag ) != std::tstring::npos )
			*pColor = CLR_NONE;

		return false;
	}
}


namespace ui
{
	inline double Square( UINT comp ) { ASSERT( comp >= 0 && comp <= 255 ); return static_cast<double>( comp * comp ); }
	inline double PercentToFactor( int pct ) { ASSERT( pct >= 0 && pct <= 100 ); return static_cast<double>( pct ) / 100.0; }
	inline BYTE AsComponent( double component ) { ASSERT( component >= 0.0 && component < 256.0 ); return static_cast<BYTE>( component ); }


	// accurate colour algorithms

	BYTE GetAverageComponent( UINT comp1, UINT comp2 )		// cast to UINT to allow squaring
	{
		// Average by extracting square root of sum of square components (same for G, B): NewColorR = sqrt( (R1^2 + R2^2) / 2 )
		// Details: check arntjw's entry at:
		//	https://stackoverflow.com/questions/649454/what-is-the-best-way-to-average-two-colors-that-define-a-linear-gradient?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

		return AsComponent( sqrt( ( Square(comp1) + Square(comp2) ) / 2 ) );
	}

	BYTE GetAverageComponent( UINT comp1, UINT comp2, UINT comp3 )
	{
		// Average of 3 colours (same for G, B): NewColorR = sqrt( (R1^2 + R2^2 + R3^2) / 3 )

		return AsComponent( sqrt( ( Square(comp1) + Square(comp2) + Square(comp3) ) / 3 ) );
	}

	BYTE GetWeightedMixComponent( UINT comp1, UINT comp2, double weight1 )
	{
		// For weighted mixes (i.e. 75% colour A, and 25% colour B) (same for G, B): NewColorR = sqrt( R1^2 * w + R2^2 * (1 - w) )
		//   weight1 from 0.0 to 1.0

		ASSERT( weight1 >= 0.0 && weight1 <= 1.0 );
		return AsComponent( sqrt( ( Square(comp1) * ( 1.0 - weight1 ) + Square(comp2) * weight1 ) ) );
	}


	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2 )
	{
		if ( CLR_NONE == color1 )
			return color2;
		else if ( CLR_NONE == color2 )
			return color1;

		ASSERT( IsActualColor( color1 ) && IsActualColor( color2 ) );

		return RGB(
			GetAverageComponent( GetRValue( color1 ), GetRValue( color2 ) ),
			GetAverageComponent( GetGValue( color1 ), GetGValue( color2 ) ),
			GetAverageComponent( GetBValue( color1 ), GetBValue( color2 ) )
		);
	}

	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2, COLORREF color3 )
	{
		ASSERT( IsActualColor( color1 ) && IsActualColor( color2 ) && IsActualColor( color3 ) );

		return RGB(
			GetAverageComponent( GetRValue( color1 ), GetRValue( color2 ), GetRValue( color3 ) ),
			GetAverageComponent( GetGValue( color1 ), GetGValue( color2 ), GetGValue( color3 ) ),
			GetAverageComponent( GetBValue( color1 ), GetBValue( color2 ), GetBValue( color3 ) )
		);
	}

	COLORREF GetWeightedMixColor( COLORREF color1, COLORREF color2, int pct1 )
	{
		ASSERT( IsActualColor( color1 ) && IsActualColor( color2 ) );

		double weight1 = PercentToFactor( pct1 );

		return RGB(
			GetWeightedMixComponent( GetRValue( color1 ), GetRValue( color2 ), weight1 ),
			GetWeightedMixComponent( GetGValue( color1 ), GetGValue( color2 ), weight1 ),
			GetWeightedMixComponent( GetBValue( color1 ), GetBValue( color2 ), weight1 )
		);
	}


	COLORREF GetAdjustLuminance( COLORREF color, int byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleLuminance( byPct ).GetRGB();
	}

	COLORREF GetAdjustSaturation( COLORREF color, int byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleSaturation( byPct ).GetRGB();
	}

	COLORREF GetAdjustHue( COLORREF color, int byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleHue( byPct ).GetRGB();
	}


	// CHslColor implementation

	const Range<WORD> CHslColor::s_validRange( 0, 240 );

	CHslColor::CHslColor( COLORREF rgbColor )
	{
		REQUIRE( ui::IsActualColor( rgbColor ) );

		::ColorRGBToHLS( rgbColor, &m_hue, &m_luminance, &m_saturation );	// note the difference: Hsl vs HLS - L and S position is swapped in shell API
	}

	COLORREF CHslColor::GetRGB( void ) const
	{
		ASSERT( IsValid() );
		return ::ColorHLSToRGB( m_hue, m_luminance, m_saturation );			// note the difference: Hsl vs HLS - L and S position is swapped in shell API
	}

	WORD CHslColor::ModifyBy( WORD component, int byPercentage )
	{
		ASSERT( byPercentage >= -100 && byPercentage <= 100 );

		const double byFactor = (double)byPercentage / 100.0;
		double newComponent = component;

		if ( byFactor > 0.0 )
			newComponent += ( ( 255.0 - newComponent ) * byFactor );
		else if ( byFactor < 0.0 )
			newComponent *= ( 1.0 + byFactor );

		static const Range<double> s_dValidRange( s_validRange );
		s_dValidRange.Constrain( newComponent );

		return static_cast<WORD>( newComponent );
	}
}
