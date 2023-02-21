
#include "pch.h"
#include "CodeParsing.h"

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
		static const std::wstring s_brackets =   L"()[]{}<>";
		static const std::wstring s_matching = L")(][}{><";

		REQUIRE( IsBracket( bracket ) );

		size_t bracketPos = s_brackets.find( bracket );

		if ( std::wstring::npos == bracketPos )
			return L'\0';

		return s_matching[ bracketPos ];
	}


	template<>
	const CLanguage<char>& GetCppLang<char>( void )
	{
		static const CLanguage<char> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n" );			// quote | double-quote | comment | single-line-comment
		return s_cppLang;
	}

	template<>
	const CLanguage<wchar_t>& GetCppLang<wchar_t>( void )
	{
		static const CLanguage<wchar_t> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n" );
		return s_cppLang;
	}
}
