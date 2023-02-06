
#include "stdafx.h"
#include "CodeParsingBase.h"

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

	bool IsOpenBrace( wchar_t brace )
	{
		static const std::wstring s_openBraces = L"([{<";
		return s_openBraces.find( brace ) != std::wstring::npos;
	}

	bool IsCloseBrace( wchar_t brace )
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
}
