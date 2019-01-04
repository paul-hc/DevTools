// Copyleft 2004 Paul Cocoveanu
//
#include "stdafx.h"
#include "LanguageSearchEngine.h"
#include "BraceParityStatus.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	// CommentTokens implementation

	const TCHAR* CommentTokens::getOpenToken( CommentState commentState ) const
	{
		if ( commentState == SingleLineComment )
			return m_singleLineComment;
		else if ( commentState == MultiLineComment )
			return m_openComment;

		return NULL;
	}

	bool CommentTokens::hasSingleLineComment( void ) const
	{
		return m_singleLineComment != NULL;
	}

	bool CommentTokens::hasMultiLineComment( void ) const
	{
		return m_openComment != NULL && m_closeComment != NULL;
	}

	const CommentTokens& CommentTokens::getLanguageSpecific( DocLanguage m_docLanguage )
	{
		static CommentTokens commentTokens[] =
		{
			{ _T("#"), NULL, NULL },			// DocLang_None
			{ _T("//"), _T("/*"), _T("*/") },	// DocLang_Cpp
			{ _T("'"), NULL, NULL },			// DocLang_Basic
			{ _T("--"), _T("/*"), _T("*/") },	// DocLang_SQL
			{ NULL, _T("<!--"), _T("-->") },	// DocLang_HtmlXml
			{ _T("//"), _T("/*"), _T("*/") }	// DocLang_IDL
		};

		if ( m_docLanguage == DocLang_None )
			commentTokens[ DocLang_None ].m_singleLineComment = app::GetModuleSession().m_singleLineCommentToken.c_str();

		return commentTokens[ m_docLanguage ];
	}


	// LanguageSearchEngine implementation

	LanguageSearchEngine::LanguageSearchEngine( DocLanguage _docLanguage )
		: m_docLanguage( _docLanguage )
	{
	}

	LanguageSearchEngine::~LanguageSearchEngine()
	{
	}

	void LanguageSearchEngine::setDocLanguage( DocLanguage _docLanguage )
	{
		m_docLanguage = _docLanguage;
	}

	bool LanguageSearchEngine::isTokenMatch( const TCHAR* pString, int pos, const TCHAR* token,
											 bool skipFwdWhiteSpace /*= true*/ ) const
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );
		ASSERT( token != NULL );

		if ( skipFwdWhiteSpace )
			while ( code::isWhitespaceChar( pString[ pos ] ) )
				++pos;

		return str::isTokenMatch( pString, token, pos, ::getLanguageCase( m_docLanguage ) );
	}

	bool LanguageSearchEngine::isTokenMatchBefore( const TCHAR* pString, int pos, const TCHAR* token,
												   bool skipBkwdWhiteSpace /*= true*/ ) const
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );
		ASSERT( token != NULL );

		if ( skipBkwdWhiteSpace )
			while ( pos > 0 && code::isWhitespaceChar( pString[ pos - 1 ] ) )
				--pos;

		pos -= str::Length( token );
		if ( pos < 0 )
			return false;

		return str::isTokenMatch( pString, token, pos, ::getLanguageCase( m_docLanguage ) );
	}

	/**
		Notes:
			Caller is responsible to handle the case 'outStatementEndPos' is -1.
			For single-line comments, the line-end is excluded from the comment.
	*/
	bool LanguageSearchEngine::isCommentStatement( int& outStatementEndPos, const TCHAR* pString, int pos ) const
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );

		outStatementEndPos = -1;

		switch ( m_docLanguage )
		{
			case DocLang_None:
			{
				const CString& singleLineCommentToken = app::GetModuleSession().m_singleLineCommentToken.c_str();

				if ( 0 == _tcsncmp( pString + pos, singleLineCommentToken, singleLineCommentToken.GetLength() ) )
				{	// "#"
					outStatementEndPos = str::findStringPos( pString, code::lineEnd, pos += singleLineCommentToken.GetLength() ).m_start;
					return true;
				}
				break;
			}
			case DocLang_Cpp:
			case DocLang_IDL:
				if ( pString[ pos ] == _T('/') )
				{
					++pos;
					if ( pString[ pos ] == _T('*') )
					{	// "/*...*/"
						outStatementEndPos = str::findStringPos( pString, _T("*/"), ++pos ).m_end;
						return true;
					}
					else if ( pString[ pos ] == _T('/') )
					{	// "//..."
						// NB: for single-line comment, the line-end is excluded from the comment
						outStatementEndPos = str::findStringPos( pString, code::lineEnd, ++pos ).m_start;
						return true;
					}
				}
				break;
			case DocLang_Basic:
				if ( pString[ pos ] == _T('\'') )
				{	// "'..."
					outStatementEndPos = str::findStringPos( pString, code::lineEnd, ++pos ).m_start;
					return true;
				}
				else if ( 0 == _tcsnicmp( pString + pos, _T("REM"), 3 ) )
					if ( code::isWhitespaceChar( pString[ pos += 3 ] ) )
					{	// "REM ..."
						outStatementEndPos = str::findStringPos( pString, code::lineEnd, ++pos ).m_start;
						return true;
					}
				break;
			case DocLang_SQL:
				if ( 0 == _tcsncmp( pString + pos, _T("/*"), 2 ) )
				{	// "/*...*/"
					outStatementEndPos = str::findStringPos( pString, _T("*/"), pos += 2 ).m_end;
					return true;
				}
				else if ( 0 == _tcsncmp( pString + pos, _T("--"), 2 ) )
				{	// "--..."
					outStatementEndPos = str::findStringPos( pString, code::lineEnd, pos += 2 ).m_start;
					return true;
				}
				break;
			case DocLang_HtmlXml:
				if ( 0 == _tcsncmp( pString + pos, _T("<!--"), 4 ) )
				{	// "<!--...-->"
					outStatementEndPos = str::findStringPos( pString, _T("-->"), pos += 4 ).m_end;
					return true;
				}
				break;
		}

		return false;
	}

	bool LanguageSearchEngine::isSingleLineCommentStatement( const TCHAR* pString, int pos ) const
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );

		switch ( m_docLanguage )
		{
			case DocLang_None:
			{
				const CString& singleLineCommentToken = app::GetModuleSession().m_singleLineCommentToken.c_str();

				return 0 == _tcsncmp( pString + pos, singleLineCommentToken, singleLineCommentToken.GetLength() );
			}
			case DocLang_Cpp:
			case DocLang_IDL:
				return 0 == _tcsncmp( pString + pos, _T("//"), 2 );
			case DocLang_Basic:
				return pString[ pos ] == _T('\'') || 0 == _tcsnicmp( pString + pos, _T("REM "), 4 );
			case DocLang_SQL:
				return 0 == _tcsncmp( pString + pos, _T("--"), 2 );
			case DocLang_HtmlXml:
				return false; // multi-line comments only
		}

		return false;
	}

	/**
		Return true and positions outStatementEndPos at the end of cast if string at pos points
		to a C cast statement.
		Example: "(const char*)blahblah" or "(int)(char*)(const char*)blahblah"
	*/
	bool LanguageSearchEngine::isCCastStatement( int& outStatementEndPos, const TCHAR* pString, int pos ) const
	{
		ASSERT( m_docLanguage == DocLang_Cpp );
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );

		outStatementEndPos = -1;

		TCHAR prevChar = _T('\0');

		if ( pos > 0 )
		{
			int prevCharPos = pos - 1;

			prevChar = pString[ prevCharPos ];
			if ( !code::isWhitespaceChar( prevChar ) )
				if ( _istalnum( prevChar ) )
					return false; // not a cast for sure, e.g. func(...)
		}

		const TCHAR* pCursor = pString + pos;

		if ( *pCursor == _T('(') )
			if ( *( pCursor + 1 ) != _T(')') )
			{
				code::BraceParityStatus braceStatus;
				int closeBracePos = braceStatus.findMatchingBracePos( pString, pos, m_docLanguage );

				if ( closeBracePos != -1 )
					if ( braceStatus.isEntirelyEven() )
					{
						pCursor = pString + closeBracePos + 1;

						if ( *pCursor != _T('\0') )
							if ( code::isQuoteChar( *pCursor ) ||
								 ( _istalnum( *pCursor ) && !_istalnum( prevChar ) ) ||
								 0 == _tcsncmp( pCursor, _T("::"), 2 ) )
							{
								outStatementEndPos = closeBracePos + 1;
								return true;
							}
							else
							{
								if ( *pCursor == _T('(') )
									// Could be a cascaded cast like "(char*)(const char*)blahblah"
									return isCCastStatement( outStatementEndPos, pString, closeBracePos + 1 );
							}
					}
			}

		return false;
	}

	bool LanguageSearchEngine::isUnicodePortableStringConstant( int& outStatementEndPos, const TCHAR* pString, int pos ) const
	{
		ASSERT( m_docLanguage == DocLang_Cpp );
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );

		outStatementEndPos = -1;

		const TCHAR* pCursor = pString + pos;

		if ( str::isTokenMatch( pString, _T("_T("), pos ) )
		{
			pCursor = _tcschr( pCursor, _T('(') );
			const TCHAR* quoteStart = pCursor + 1;

			while ( code::isWhitespaceChar( *quoteStart ) )
				++quoteStart;

			if ( code::isQuoteChar( *quoteStart ) )
			{
				code::BraceParityStatus braceStatus;
				int closeBracePos = braceStatus.findMatchingBracePos( pString, int( pCursor - pString ), m_docLanguage );

				if ( closeBracePos != -1 )
					if ( braceStatus.isEntirelyEven() )
					{
						outStatementEndPos = closeBracePos + 1;
						return true;
					}
			}
		}

		return false;
	}

	/**
		Returns true if at the current position there is: [whitespaces] [comment] [whitespaces] [line-end]
	*/
	bool LanguageSearchEngine::isProtectedLineTermination( int& outStatementEndPos, const TCHAR* pString, int pos ) const
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= str::Length( pString ) );

		outStatementEndPos = -1;

		const TCHAR* pCursor = pString + pos;

		while ( code::isWhitespaceChar( *pCursor ) )
			++pCursor;

		int commentEnd;

		if ( isCommentStatement( commentEnd, pString, int( pCursor - pString ) ) )
			pCursor = pString + str::safePos( commentEnd, pString );

		while ( code::isWhitespaceChar( *pCursor ) || isAtLineEnd( pCursor, m_docLanguage ) )
			++pCursor;

		if ( *pCursor != _T('\0') )
			return false; // not end of string -> match was found

		outStatementEndPos = int( pCursor - pString );
		return true; // true if a match was found
	}

	TokenRange LanguageSearchEngine::findQuotedString( const TCHAR* pString, int startPos /*= 0*/,
													   str::CaseType caseType /*= str::Case*/,
													   const TCHAR* quoteSet /*= code::quoteChars*/ ) const
	{
		caseType;
		ASSERT( pString != NULL && startPos >= 0 && startPos <= str::Length( pString ) );

		const TCHAR* pCursor = pString + startPos;

		while ( *pCursor != _T('\0') )
		{
			int commentEnd;

			if ( code::isQuoteChar( *pCursor, quoteSet ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pString, int( pCursor - pString ) );

				if ( matchingQuotePos != -1 )
					return TokenRange( int( pCursor - pString ), matchingQuotePos + 1 );
				else
					pCursor += _tcslen( pCursor ); // fatal syntax error -> abort searching
			}
			else if ( isCommentStatement( commentEnd, pString, int( pCursor - pString ) ) )
			{
				pCursor = pString + str::safePos( commentEnd, pString );
				continue; // skip incrementing
			}

			++pCursor;
		}

		return TokenRange( int( pCursor - pString ) ); // no match was found
	}

	TokenRange LanguageSearchEngine::findComment( const TCHAR* pString, int startPos /*= 0*/,
												  str::CaseType caseType /*= str::Case*/ ) const
	{
		caseType;
		ASSERT( pString != NULL && startPos >= 0 && startPos <= str::Length( pString ) );

		const TCHAR* pCursor = pString + startPos;

		while ( *pCursor != _T('\0') )
		{
			int commentEnd;

			if ( isCommentStatement( commentEnd, pString, int( pCursor - pString ) ) )
				return TokenRange( int( pCursor - pString ), str::safePos( commentEnd, pString ) );
			else if ( code::isQuoteChar( *pCursor ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pString, int( pCursor - pString ) );

				if ( matchingQuotePos != -1 )
					pCursor = pString + matchingQuotePos; // next step: increment past closing quote
				else
				{
					pCursor += _tcslen( pCursor ); // fatal syntax error -> abort searching
					break;
				}
			}

			++pCursor;
		}

		return TokenRange( int( pCursor - pString ) ); // no match was found
	}

	TokenRange LanguageSearchEngine::findNextNumber( const TCHAR* text, int startPos /*= 0*/ )
	{
		ASSERT( text != NULL && startPos >= 0 && startPos <= str::Length( text ) );

		int pos = startPos;

		while ( text[ pos ] != _T('\0') && !_istdigit( text[ pos ] ) )
			++pos;

		while ( pos > 0 && text[ pos ] != _T('\0') && !_istspace( text[ pos - 1 ] ) )
		{
			str::skipDigit( pos, text );

			while ( text[ pos ] != _T('\0') && !_istdigit( text[ pos ] ) )
				++pos;
		}

		if ( text[ pos ] == _T('\0') )
			return TokenRange( -1 );

		ASSERT( str::isNumberChar( text[ pos ] ) );

		TokenRange numberRange( pos );

		str::skipDigit( numberRange.m_end, text );
		return numberRange;
	}

	FormattedNumber< unsigned int > LanguageSearchEngine::extractNumber( const TCHAR* text, int startPos /*= 0*/ ) throws_( CRuntimeException )
	{
		TokenRange numberRange = findNextNumber( text, startPos );
		if ( numberRange.IsEmpty() )
			throw CRuntimeException( str::Format( _T("No number detected in '%s'"), text ) );

		ASSERT( numberRange.IsValid() );

		FormattedNumber< unsigned int > parsedNumber( str::parseUnsignedInteger( text + numberRange.m_start ), _T("%u") );

		if ( 0 == _tcsnicmp( text + numberRange.m_start, _T("0x"), 2 ) )
		{
			int hexDigitCount = 0;

			for ( int hexPos = numberRange.m_start + 2; _istxdigit( text[ hexPos ] ); ++hexPos )
				++hexDigitCount;

			if ( 0 == hexDigitCount )
				throw CRuntimeException( str::Format( _T("Invalid hex number in '%s'"), text ) );

			parsedNumber.m_format.Format( _T("0x%%0%dX"), hexDigitCount );
		}

		return parsedNumber;
	}

	struct IsSubString : public LanguageSearchEngine::IsMatchAtCursor
	{
		IsSubString( const TCHAR* string, const TCHAR* pSubString, str::CaseType caseType )
			: m_pString( string ), m_pSubString( pSubString ), m_caseType( caseType ), m_outFoundSubString( -1 )
		{
		}

		virtual bool operator()( const TCHAR* pCursor )
		{
			if ( !str::isTokenMatch( pCursor, m_pSubString, 0, m_caseType ) )
				return false;

			// found the pSubString match -> return the pSubString token
			m_outFoundSubString.setEmpty( int( pCursor - m_pString ) );
			m_outFoundSubString.setLength( str::Length( m_pSubString ) );
			return true;
		}
	private:
		const TCHAR* m_pString;
		const TCHAR* m_pSubString;
		str::CaseType m_caseType;
	public:
		TokenRange m_outFoundSubString;
	};

	TokenRange LanguageSearchEngine::findString( const TCHAR* pString, const TCHAR* pSubString, int startPos /*= 0*/,
												 str::CaseType caseType /*= str::Case*/ ) const
	{
		ASSERT( pSubString != NULL );

		IsSubString predIsSubString( pString, pSubString, caseType );

		if ( !findNextMatch( pString, startPos, predIsSubString ) )
			predIsSubString.m_outFoundSubString.gotoEnd( pString );

		return predIsSubString.m_outFoundSubString;
	}

	struct IsOneOfCharSet : public LanguageSearchEngine::IsMatchAtCursor
	{
		IsOneOfCharSet( const TCHAR* string, const TCHAR* pCharSet, str::CaseType caseType )
			: m_pString( string ), m_pCharSet( pCharSet ), m_caseType( caseType ), m_outFoundPos( -1 )
		{
		}

		virtual bool operator()( const TCHAR* pCursor )
		{
			if ( -1 == str::findCharPos( m_pCharSet, *pCursor, 0, m_caseType ) )
				return false;

			// found a match -> return the position
			m_outFoundPos = int( pCursor - m_pString );
			return true;
		}
	private:
		const TCHAR* m_pString;
		const TCHAR* m_pCharSet;
		str::CaseType m_caseType;
	public:
		int m_outFoundPos;
	};

	int LanguageSearchEngine::findOneOf( const TCHAR* pString, const TCHAR* pCharSet, int startPos,
										 str::CaseType caseType /*= str::Case*/ ) const
	{
		ASSERT( pString != NULL && pCharSet != NULL );

		IsOneOfCharSet predIsOneOfCharSet( pString, pCharSet, caseType );

		if ( !findNextMatch( pString, startPos, predIsOneOfCharSet ) )
			return str::Length( pString );

		return predIsOneOfCharSet.m_outFoundPos;
	}

	/**
		Convenient code-reuse helper for code searching (skipping comments, quoted strings, etc).
		NOTE: Return position(s) are stored in the functor!
	*/
	bool LanguageSearchEngine::findNextMatch( const TCHAR* pString, int startPos, IsMatchAtCursor& isMatchAtCursor ) const
	{
		ASSERT( pString != NULL && startPos >= 0 && startPos <= str::Length( pString ) );

		const TCHAR* pCursor = pString + startPos;

		while ( *pCursor != _T('\0') )
		{
			int commentEnd;

			if ( isMatchAtCursor( pCursor ) )
				break; // a match was found
			else if ( code::isQuoteChar( *pCursor ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pString, int( pCursor - pString ) );

				if ( matchingQuotePos != -1 )
					pCursor = pString + matchingQuotePos; // next step: increment past closing quote
				else
					return false; // fatal syntax error -> abort searching
			}
			else if ( isCommentStatement( commentEnd, pString, int( pCursor - pString ) ) )
			{
				pCursor = pString + str::safePos( commentEnd, pString );
				continue; // skip incrementing
			}

			++pCursor;
		}

		return *pCursor != _T('\0'); // true if a match was found
	}

} // namespace code
