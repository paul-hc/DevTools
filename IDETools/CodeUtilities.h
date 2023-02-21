// Copyleft 2004 Paul Cocoveanu
//
#ifndef CodeUtilities_h
#define CodeUtilities_h
#pragma once

#include "TokenRange.h"


namespace code
{
	int FindPosMatchingBracket( int bracketPos, const std::tstring& codeText );
	bool SkipPosPastMatchingBracket( int* pBracketPos /*in-out*/, const std::tstring& codeText );
}


#include "DocLanguage.h"
#include "StringUtilitiesEx.h"


namespace code
{
	// Constants
	extern const TCHAR* lineEnd;
	extern const TCHAR* lineEndUnix;
	extern const TCHAR* basicMidLineEnd;

	extern const TCHAR* cppEscapedChars;
	extern const TCHAR* openBraces;
	extern const TCHAR* closeBraces;
	extern const TCHAR* quoteChars;

	// Character type
	inline bool isLineEndChar( TCHAR chr )
	{
		return chr == _T('\r') || chr == _T('\n');
	}

	bool isLineBreakEscapeChar( TCHAR chr, DocLanguage docLanguage );
	bool isAtLineEnd( const TCHAR* chrPtr, DocLanguage docLanguage );

	inline bool isWhitespaceChar( TCHAR chr )
	{
		return chr == _T(' ') || chr == _T('\t');
	}

	inline bool isWhitespaceOrLineEndChar( TCHAR chr )
	{
		return isWhitespaceChar( chr ) || isLineEndChar( chr );
	}

	inline bool isQuoteChar( TCHAR chr )
	{
		return chr == _T('\"') || chr == _T('\'');
	}

	inline bool isQuoteChar( TCHAR chr, const TCHAR* quoteSet )
	{
		ASSERT( quoteSet != NULL );
		return _tcschr( quoteSet, chr ) != NULL;
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
	CString convertTabsToSpaces( const CString& codeText, int tabSize = 4 );

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
