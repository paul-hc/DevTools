
#include "pch.h"
#include "CodeParsing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	bool IsBrace( wchar_t brace )
	{
		static const std::wstring s_allBraces = L"()[]{}<>";
		return s_allBraces.find( brace ) != std::wstring::npos;
	}

	bool IsStartBrace( wchar_t brace )
	{
		static const std::wstring s_openBraces = L"([{<";
		return s_openBraces.find( brace ) != std::wstring::npos;
	}

	bool IsEndBrace( wchar_t brace )
	{
		static const std::wstring s_closeBraces = L")]}>";
		return s_closeBraces.find( brace ) != std::wstring::npos;
	}

	wchar_t _GetMatchingBrace( wchar_t brace )
	{
		static const std::wstring s_braces =   L"()[]{}<>";
		static const std::wstring s_matching = L")(][}{><";

		REQUIRE( IsBrace( brace ) );

		size_t bracePos = s_braces.find( brace );

		if ( std::wstring::npos == bracePos )
			return L'\0';

		return s_matching[ bracePos ];
	}


	template<>
	const CLanguage<char>& GetCppLanguage<char>( void )
	{
		static const CLanguage<char> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n" );			// quote | double-quote | comment | single-line-comment
		return s_cppLang;
	}

	template<>
	const CLanguage<wchar_t>& GetCppLanguage<wchar_t>( void )
	{
		static const CLanguage<wchar_t> s_cppLang( "'|\"|/*|//", "'|\"|*/|\n" );
		return s_cppLang;
	}
}
