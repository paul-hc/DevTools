// Copyleft 2004 Paul Cocoveanu
//
#ifndef CodeUtilities_h
#define CodeUtilities_h
#pragma once

#include "utl/StringCompare.h"
#include "CodeUtils.h"
#include "TokenRange.h"
#include "DocLanguage.h"
#include "StringUtilitiesEx.h"


namespace code
{
	// constants
	extern const TCHAR* g_pBasicLineBreak;

	extern const TCHAR* g_pCppEscapedChars;
	extern const TCHAR* g_pOpenBrackets;
	extern const TCHAR* g_pCloseBrackets;
	extern const TCHAR* g_pQuoteChars;


	// character type

	inline bool isLineEndChar( TCHAR chr )
	{
		return pred::IsLineEnd()( chr );
	}

	bool isLineBreakEscapeChar( TCHAR chr, DocLanguage docLanguage );
	bool isAtLineEnd( const TCHAR* chrPtr, DocLanguage docLanguage );

	inline bool isWhitespaceChar( TCHAR chr )
	{
		return pred::IsBlank()( chr );
	}

	inline bool isWhitespaceOrLineEndChar( TCHAR chr )
	{
		return pred::IsSpace()( chr );
	}

	inline bool isQuoteChar( TCHAR chr )
	{
		return chr == _T('\"') || chr == _T('\'');
	}

	inline bool isQuoteChar( TCHAR chr, const TCHAR* quoteSet )
	{
		ASSERT( quoteSet != nullptr );
		return _tcschr( quoteSet, chr ) != nullptr;
	}

	inline bool isOpenBraceChar( TCHAR chr )
	{
		return chr == _T('(') || chr == _T('[') || chr == _T('<') || chr == _T('{');
	}

	inline bool isCloseBraceChar( TCHAR chr )
	{
		return chr == _T(')') || chr == _T(']') || chr == _T('>') || chr == _T('}');
	}

	inline bool isBraceChar( TCHAR chr )
	{
		return isOpenBraceChar( chr ) || isCloseBraceChar( chr );
	}

	TCHAR getMatchingBrace( TCHAR chBrace );
	bool isValidBraceChar( TCHAR chr, const TCHAR* validOpenBraces );

	// Code helpers
	CString& convertToWindowsLineEnds( CString& targetCodeText );
	CString& convertToUnixLineEnds( CString& targetCodeText );

	namespace cpp
	{
		CString formatEscapeSequence( const TCHAR* pCodeText );
		CString parseEscapeSequences( const TCHAR* pDisplayText );
	}

	// Code parsing (high level)
	int findMatchingQuotePos( const TCHAR* str, int openQuotePos );

	// Code parsing (low level)
	int findNextWhitespacePos( const TCHAR* str, int startPos = 0 );
	int findNextNonWhitespace( const TCHAR* str, int startPos = 0 );

} // namespace code

#endif // CodeUtilities_h
