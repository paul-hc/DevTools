
#include "pch.h"
#include "Language.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	bool IsBracket( wchar_t bracket )
	{
		static const std::wstring s_allBrackets = L"()[]{}<>";
		return s_allBrackets.find( bracket ) != std::wstring::npos;
	}

	bool IsStartBracket( wchar_t bracket )
	{
		static const std::wstring s_openBrackets = L"([{<";
		return s_openBrackets.find( bracket ) != std::wstring::npos;
	}

	bool IsEndBracket( wchar_t bracket )
	{
		static const std::wstring s_closeBrackets = L")]}>";
		return s_closeBrackets.find( bracket ) != std::wstring::npos;
	}

	wchar_t _GetMatchingBracket( wchar_t bracket )
	{
		static const std::wstring s_brackets = L"()[]{}<>";
		static const std::wstring s_matching = L")(][}{><";

		REQUIRE( IsBracket( bracket ) );

		size_t bracketPos = s_brackets.find( bracket );

		if ( std::wstring::npos == bracketPos )
			return L'\0';

		return s_matching[ bracketPos ];
	}
}


namespace code
{
	// CEscaper implementation

	CEscaper::CEscaper( char escSeqLead, const char* pEscaped, const char* pActual, const char* pEncNewLineContinue, const char* pEncRetain, char hexPrefix, bool parseOctalLiteral )
		: m_escSeqLead( escSeqLead )
		, m_escaped( pEscaped )
		, m_actual( pActual )
		, m_encLineContinue( pEncNewLineContinue )
		, m_encRetain( pEncRetain )
		, m_hexPrefix( hexPrefix )
		, m_parseOctalLiteral( parseOctalLiteral )
	{
		ENSURE( m_escaped.size() == m_actual.size() );
	}

	std::string CEscaper::FormatHexCh( char chr ) const
	{
		char buffer[ 16 ];
		sprintf_s( ARRAY_SPAN( buffer ), "\\x%02X", static_cast<unsigned char>( chr ) );
		return buffer;
	}

	std::wstring CEscaper::FormatHexCh( wchar_t chr ) const
	{
		wchar_t buffer[ 16 ];
		swprintf_s( ARRAY_SPAN( buffer ), L"\\x%04X", static_cast<unsigned short>( chr ) );
		return buffer;
	}
}


namespace code
{
	template<>
	const CLanguage<char>& GetLangCpp<char>( void )
	{	// indexed by cpp::SepMatch
		static const CLanguage<char> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n", &GetEscaperC() );			// quote | double-quote | comment | single-line-comment
		return s_cppLang;
	}

	template<>
	const CLanguage<wchar_t>& GetLangCpp<wchar_t>( void )
	{
		static const CLanguage<wchar_t> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n", &GetEscaperC() );
		return s_cppLang;
	}


	const CEscaper& GetEscaperC( void )
	{
		static const CEscaper s_escaperC( '\\', "abfnrtv\'\"?", "\a\b\f\n\r\t\v\'\"?", "\\\r\n", "\?", 'x', true );
		return s_escaperC;
	}
}
