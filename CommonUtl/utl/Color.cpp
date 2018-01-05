
#include "stdafx.h"
#include "Color.h"
#include "StringUtilities.h"

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
		*pValue = static_cast< BYTE >( value );
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
		*pValue = static_cast< BYTE >( value );
		return true;
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
		ASSERT( pHtmlLiteral != NULL && '#' == pHtmlLiteral[ 0 ] && 7 == strlen( pHtmlLiteral ) );

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
			int pos = static_cast< int >( posTag + 1 );
			BYTE r, g, b;

			if ( str::length( &text[ pos ] ) >= MinLength &&
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
