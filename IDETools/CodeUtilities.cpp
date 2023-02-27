
#include "pch.h"
#include "CodeUtilities.h"
#include "CppParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	const TCHAR* lineEnd = _T("\r\n");
	const TCHAR* lineEndUnix = _T("\n");
	const TCHAR* basicMidLineEnd = _T("_\r\n");

	const TCHAR* cppEscapedChars = _T("rntvabf\"\'\\?");
	const TCHAR* openBraces =  _T("(<[{");
	const TCHAR* closeBraces = _T(")>]}");
	const TCHAR* quoteChars = _T("\"'");


	CString& convertToWindowsLineEnds( CString& targetCodeText )
	{
		targetCodeText.Replace( lineEnd, lineEndUnix );
		targetCodeText.Replace( lineEndUnix, lineEnd );

		return targetCodeText;
	}

	CString& convertToUnixLineEnds( CString& targetCodeText )
	{
		targetCodeText.Replace( lineEnd, lineEndUnix );
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
			   ( chrPtr[ 1 ] == _T('\0') || isLineEndChar( chrPtr[ 1 ] ) );
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
		ASSERT( validOpenBraces != NULL );

		if ( code::isOpenBraceChar( chr ) )
			return _tcschr( validOpenBraces, chr ) != NULL;
		else if ( code::isCloseBraceChar( chr ) )
			return _tcschr( validOpenBraces, code::getMatchingBrace( chr ) ) != NULL;
		else
			return false;
	}

	int findMatchingQuotePos( const TCHAR* str, int openQuotePos )
	{
		TCHAR matchingQuote = str[ openQuotePos ];

		ASSERT( str != NULL && openQuotePos >= 0 && openQuotePos < str::Length( str ) );
		ASSERT( isQuoteChar( matchingQuote ) );

		const TCHAR* cursor = str + openQuotePos + 1;

		while ( *cursor != _T('\0') )
			if ( *cursor == matchingQuote )
				return int( cursor - str );		// found the matching quote -> return the position
			else if ( *cursor == _T('\\') )
			{
				++cursor;

				if ( _tcschr( cppEscapedChars, *cursor ) != NULL )
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

		TRACE( _T("ERROR: Quoted string not closed at pos %d: [%s]\n"), openQuotePos, str + openQuotePos );
		return -1; // reached end of string, closing quote not found
	}

	/**
		Finds next whitespace character, starting from startPos
	*/
	int findNextWhitespacePos( const TCHAR* str, int startPos /*= 0*/ )
	{
		ASSERT( str != NULL && startPos <= (int)_tcslen( str ) );

		const TCHAR* cursorNonWS = str + startPos;

		while ( *cursorNonWS != _T('\0') && !_istspace( *cursorNonWS ) )
			++cursorNonWS;

		return int( cursorNonWS - str );
	}

	/**
		Finds first non-whitespace character, starting from startPos
	*/
	int findNextNonWhitespace( const TCHAR* str, int startPos /*= 0*/ )
	{
		ASSERT( str != NULL && startPos <= (int)_tcslen( str ) );

		const TCHAR* cursorNonWS = str + startPos;

		while ( *cursorNonWS != _T('\0') && _istspace( *cursorNonWS ) )
			++cursorNonWS;

		return int( cursorNonWS - str );
	}

	namespace cpp
	{
		CString formatEscapeSequence( const TCHAR* pCodeText )
		{
			ASSERT( pCodeText != NULL );

			CString displayText;
			TCHAR* pOut = displayText.GetBuffer( 2 * str::Length( pCodeText ) );

			for ( const TCHAR* pSource = pCodeText; *pSource != _T('\0'); ++pSource )
				switch ( *pSource )
				{
					case _T('\r'): *pOut++ = _T('\\'); *pOut++ = _T('r'); break;
					case _T('\n'): *pOut++ = _T('\\'); *pOut++ = _T('n'); break;
					case _T('\t'): *pOut++ = _T('\\'); *pOut++ = _T('t'); break;
					case _T('\v'): *pOut++ = _T('\\'); *pOut++ = _T('v'); break;
					case _T('\a'): *pOut++ = _T('\\'); *pOut++ = _T('a'); break;
					case _T('\b'): *pOut++ = _T('\\'); *pOut++ = _T('b'); break;
					case _T('\f'): *pOut++ = _T('\\'); *pOut++ = _T('f'); break;
					case _T('\"'): *pOut++ = _T('\\'); *pOut++ = _T('\"'); break;
					case _T('\''): *pOut++ = _T('\\'); *pOut++ = _T('\''); break;
					case _T('\\'): *pOut++ = _T('\\'); *pOut++ = _T('\\'); break;
					case _T('\?'): *pOut++ = _T('\\'); *pOut++ = _T('?'); break;
					default:
						*pOut++ = *pSource;
						break;
				}

			*pOut = _T('\0');
			displayText.ReleaseBuffer();

			return displayText;
		}

		CString parseEscapeSequences( const TCHAR* pDisplayText )
		{
			ASSERT( pDisplayText != NULL );

			CString codeText;
			TCHAR* pOut = codeText.GetBuffer( 2 * str::Length( pDisplayText ) );

			for ( const TCHAR* pSource = pDisplayText; *pSource != _T('\0'); ++pSource )
				if ( *pSource == _T('\\') )
					switch ( *++pSource )
					{
						case _T('r'):  *pOut++ = _T('\r'); break;
						case _T('n'):  *pOut++ = _T('\n'); break;
						case _T('t'):  *pOut++ = _T('\t'); break;
						case _T('v'):  *pOut++ = _T('\v'); break;
						case _T('a'):  *pOut++ = _T('\a'); break;
						case _T('b'):  *pOut++ = _T('\b'); break;
						case _T('f'):  *pOut++ = _T('\f'); break;
						case _T('\"'): *pOut++ = _T('\"'); break;
						case _T('\''): *pOut++ = _T('\''); break;
						case _T('\\'): *pOut++ = _T('\\'); break;
						case _T('?'):  *pOut++ = _T('\?'); break;
						default:
							// bad escape -> consider it an '\'
							*pOut++ = *--pSource;
							MessageBeep( MB_ICONERROR );
							break;
					}
				else
					*pOut++ = *pSource;

			*pOut = _T('\0');
			codeText.ReleaseBuffer();

			return codeText;
		}
	}

} // namespace code
