#ifndef Encoding_h
#define Encoding_h
#pragma once


class CEnumTags;


namespace fs
{
	enum Encoding
	{
		ANSI_UTF8,			// no BOM - could be UTF8 with no-BOM as well (hard to speculate)
		UTF8_bom,			// BOM: {EF BB BF}
		UTF16_LE_bom,		// BOM: {FF FE} - aka UCS2 (little-endian) MS default UTF-16; e.g. first char {2F 00}
		UTF16_be_bom,		// BOM: {FE FF} - aka UCS2 (big-endian); e.g. first char {00 2F}
		UTF32_LE_bom,		// BOM: {FF FE 00 00} - aka UCS4 (little-endian) - MS default UTF-32; e.g. first char {2F 00 00 00}
		UTF32_be_bom,		// BOM: {00 00 FE FF} - aka UCS4 (big-endian); e.g. first char {00 00 00 2F}

			_Encoding_Count,
	};

	const CEnumTags& GetTags_Encoding( void );

	size_t GetCharByteCount( Encoding encoding );		// byte count per charater in encoding
}


#if _MSC_VER <= 1700		// vc11 (VS-2012)

	// introducing char32_t just for illustration - no support yet implemented in STL or Windows
	typedef unsigned long char32_t;				// UTF32, e.g. U'a'

#endif


namespace str
{
	typedef std::basic_string<char32_t> wstring4;
}


#include "Endianness.h"


namespace func
{
	// encoding/decoding with byte swapping for UTF16_be_bom/UTF32_be_bom

	template< fs::Encoding encoding >
	struct CharEncoder
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const
		{
			return chr;
		}
	};

	template<>
	struct CharEncoder< fs::UTF16_be_bom >
	{
		wchar_t operator()( wchar_t chr ) const
		{
			return endian::GetBytesSwapped<endian::Little, endian::Big>()( chr );
		}
	};

	template<>
	struct CharEncoder< fs::UTF32_be_bom >
	{
		char32_t operator()( char32_t chr ) const
		{
			return endian::GetBytesSwapped<endian::Little, endian::Big>()( chr );
		}
	};


	template< fs::Encoding encoding >
	struct CharDecoder
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const
		{
			return chr;
		}
	};

	template<>
	struct CharDecoder< fs::UTF16_be_bom >
	{
		wchar_t operator()( wchar_t beChr ) const
		{
			return endian::GetBytesSwapped<endian::Big, endian::Little>()( beChr );
		}
	};

	template<>
	struct CharDecoder< fs::UTF32_be_bom >
	{
		char32_t operator()( char32_t beChr ) const
		{
			return endian::GetBytesSwapped<endian::Big, endian::Little>()( beChr );
		}
	};
}


#endif // Encoding_h
