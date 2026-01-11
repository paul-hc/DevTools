
#include "pch.h"
#include "CodeUtilities.h"
#include "CppParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	const TCHAR* g_pBasicLineBreak = _T("_\r\n");

	const TCHAR* g_pCppEscapedChars = _T("rntvabf\"\'\\?");
	const TCHAR* g_pOpenBrackets =  _T("(<[{");
	const TCHAR* g_pCloseBrackets = _T(")>]}");
	const TCHAR* g_pQuoteChars = _T("\"'");


	CString& convertToWindowsLineEnds( CString& targetCodeText )
	{
		targetCodeText.Replace( g_pLineEnd, g_pLineEndUnix );
		targetCodeText.Replace( g_pLineEndUnix, g_pLineEnd );

		return targetCodeText;
	}

	CString& convertToUnixLineEnds( CString& targetCodeText )
	{
		targetCodeText.Replace( g_pLineEnd, g_pLineEndUnix );
		return targetCodeText;
	}

	bool isLineBreakEscapeChar( TCHAR chr, DocLanguage docLanguage )
	{
		switch ( docLanguage )
		{
			case DocLang_Cpp:
			case DocLang_IDL:
				return chr == _T('\\');
			case DocLang_Basic:
				return chr == _T('_');
			default:
				return false;
		}
	}

	bool isAtLineEnd( const TCHAR* chrPtr, DocLanguage docLanguage )
	{
		if ( isLineEndChar( *chrPtr ) )
			return true;

		return isLineBreakEscapeChar( *chrPtr, docLanguage ) &&
			   ( chrPtr[ 1 ] == '\0' || isLineEndChar( chrPtr[ 1 ] ) );
	}

	TCHAR getMatchingBrace( TCHAR chBrace )
	{
		switch ( chBrace )
		{
			default:
				ASSERT( false );

			// Open braces
			case _T('('):
				return _T(')');
			case _T('['):
				return _T(']');
			case _T('<'):
				return _T('>');
			case _T('{'):
				return _T('}');

			// Close braces
			case _T(')'):
				return _T('(');
			case _T(']'):
				return _T('[');
			case _T('>'):
				return _T('<');
			case _T('}'):
				return _T('{');
		}
	}

	bool isValidBraceChar( TCHAR chr, const TCHAR* validOpenBraces )
	{
		ASSERT( validOpenBraces != nullptr );

		if ( code::isOpenBraceChar( chr ) )
			return _tcschr( validOpenBraces, chr ) != nullptr;
		else if ( code::isCloseBraceChar( chr ) )
			return _tcschr( validOpenBraces, code::getMatchingBrace( chr ) ) != nullptr;
		else
			return false;
	}

	int findMatchingQuotePos( const TCHAR* str, int openQuotePos )
	{
		TCHAR matchingQuote = str[ openQuotePos ];

		ASSERT( str != nullptr && openQuotePos >= 0 && openQuotePos < str::Length( str ) );
		ASSERT( isQuoteChar( matchingQuote ) );

		const TCHAR* cursor = str + openQuotePos + 1;

		while ( *cursor != '\0' )
			if ( *cursor == matchingQuote )
				return int( cursor - str );		// found the matching quote -> return the position
			else if ( *cursor == _T('\\') )
			{
				++cursor;

				if ( _tcschr( g_pCppEscapedChars, *cursor ) != nullptr )
					++cursor;						// known C++ escape sequence, skip it
				else if ( *cursor == _T('x') )
				{
					++cursor;
					while ( _istxdigit( *cursor ) )
						++cursor;
				}
				else
				{
					while ( _istdigit( *cursor ) )
						++cursor;
				}
			}
			else
				++cursor;

		TRACE_FL( _T("ERROR: Quoted string not closed at pos %d: [%s]\n"), openQuotePos, str + openQuotePos );
		return -1; // reached end of string, closing quote not found
	}

	/**
		Finds next whitespace character, starting from startPos
	*/
	int findNextWhitespacePos( const TCHAR* str, int startPos /*= 0*/ )
	{
		ASSERT( str != nullptr && startPos <= (int)_tcslen( str ) );

		const TCHAR* cursorNonWS = str + startPos;

		while ( *cursorNonWS != '\0' && !_istspace( *cursorNonWS ) )
			++cursorNonWS;

		return int( cursorNonWS - str );
	}

	/**
		Finds first non-whitespace character, starting from startPos
	*/
	int findNextNonWhitespace( const TCHAR* str, int startPos /*= 0*/ )
	{
		ASSERT( str != nullptr && startPos <= (int)_tcslen( str ) );

		const TCHAR* cursorNonWS = str + startPos;

		while ( *cursorNonWS != '\0' && _istspace( *cursorNonWS ) )
			++cursorNonWS;

		return int( cursorNonWS - str );
	}

} // namespace code
