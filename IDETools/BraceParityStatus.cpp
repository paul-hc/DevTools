
#include "stdafx.h"
#include "BraceParityStatus.h"
#include "StringUtilitiesEx.h"
#include "CodeUtilities.h"
#include "LanguageSearchEngine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	// BraceParityStatus implementation

	BraceParityStatus::BraceParityStatus( void )
	{
	}

	BraceParityStatus::~BraceParityStatus()
	{
	}

	void BraceParityStatus::clear( void )
	{
		braceCounters.clear();
		errorMessages.clear();
	}

	int BraceParityStatus::findMatchingBracePos( const TCHAR* pStr, int openBracePos, DocLanguage docLanguage )
	{
		clear();

		ASSERT( pStr != NULL && openBracePos >= 0 && openBracePos <= _tcslen( pStr ) );
		ASSERT( isOpenBraceChar( pStr[ openBracePos ] ) );

		storeBrace( pStr[ openBracePos ] );

		code::LanguageSearchEngine languageEngine( docLanguage );
		TCHAR matchingCloseBrace = getMatchingBrace( pStr[ openBracePos ] );
		const TCHAR* cursor = pStr + openBracePos + 1;

		while ( *cursor != _T('\0') )
		{
			int commentEnd;

			if ( isBraceChar( *cursor ) )
			{
				BraceCounter* braceCounter = storeBrace( *cursor );

				if ( braceCounter != NULL && braceCounter->m_parityCounter == -1 )
					errorMessages.push_back( str::formatString( _T("Closing brace '%c' encountered before the opening brace; at pos %d: [%s]"),
																braceCounter->m_closeBrace, int( cursor - pStr ), cursor ) );

				if ( *cursor == matchingCloseBrace )
					if ( isBraceEven( *cursor ) )
						break;
			}
			else if ( isQuoteChar( *cursor ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pStr, int( cursor - pStr ) );

				if ( matchingQuotePos != -1 )
					cursor = pStr + matchingQuotePos;
				else
				{
					errorMessages.push_back( str::formatString( _T("Quoted string not closed at pos %d: [%s]"),
																int( cursor - pStr ), cursor ) );
					cursor += _tcslen( cursor ); // fatal error -> go to end
				}
			}
			else if ( languageEngine.isCommentStatement( commentEnd, pStr, int( cursor - pStr ) ) )
			{
				cursor = pStr + str::safePos( commentEnd, pStr );
				continue; // skip incrementing
			}

			++cursor;
		}

		checkParityErrors();

		if ( *cursor == _T('\0') )
		{
			errorMessages.push_back( str::formatString( _T("Closing brace not found for opening brace '%c' at pos %d"),
														pStr[ openBracePos ], openBracePos ) );
			return -1;						// reached end of string, closing brace not found
		}

		return int( cursor - pStr );		// found the matching brace -> return the position
	}

	struct BraceReverser : public std::unary_function< void, TCHAR >
	{
		void operator()( TCHAR& chr )
		{
			if ( code::isBraceChar( chr ) )
				chr = code::getMatchingBrace( chr );
		}
	};

	int BraceParityStatus::reverseFindMatchingBracePos( const TCHAR* pStr, int openBracePos, DocLanguage docLanguage )
	{
		// build just the leading  part of the original string, but reversed
		int length = openBracePos + 1;
		TCHAR* revString = _tcsncpy( new TCHAR[ length + 1 ], pStr, length );

		revString[ length ] = _T('\0');

		_tcsrev( revString );
		std::for_each( revString, revString + length, BraceReverser() );

		int revMatchingBracePos = findMatchingBracePos( revString, 0, docLanguage );

		delete revString;
		return revMatchingBracePos != -1 ? ( openBracePos - revMatchingBracePos - 1 ) : -1;
	}

	TokenRange BraceParityStatus::findArgList( const TCHAR* codeText, int pos, const TCHAR* argListOpenBraces,
											   DocLanguage docLanguage, bool allowUnclosedArgList /*= false*/ )
	{
		ASSERT( codeText != NULL && pos >= 0 && pos <= _tcslen( codeText ) );
		ASSERT( argListOpenBraces != NULL && argListOpenBraces[ 0 ] != _T('\0') );

		code::LanguageSearchEngine languageEngine( docLanguage );
		int openBracePos = languageEngine.findOneOf( codeText, argListOpenBraces, pos );

		ASSERT( openBracePos != -1 );
		if ( codeText[ openBracePos ] != _T('\0') )
		{
			int closeBracePos = findMatchingBracePos( codeText, openBracePos, docLanguage );

			if ( closeBracePos != -1 )
				return TokenRange( openBracePos, closeBracePos + 1 );
			else if ( allowUnclosedArgList )
				return TokenRange( openBracePos, str::Length( codeText ) );
		}

		return TokenRange::endOfString( codeText );
	}

	/**
		Analyzes the syntax for parity of the braces/quotes, returning true if syntax is correct.
		Errors (if any) are stored in 'errorMessages' data member.
	*/
	bool BraceParityStatus::analyzeBraceParity( const TCHAR* pStr, DocLanguage docLanguage )
	{
		ASSERT( pStr != NULL );

		code::LanguageSearchEngine languageEngine( docLanguage );

		int firstOpenBracePos = languageEngine.findOneOf( pStr, code::openBraces, 0 );

		ASSERT( firstOpenBracePos != -1 );

		return pStr[ firstOpenBracePos ] != _T('\0') &&
			   -1 != findMatchingBracePos( pStr, firstOpenBracePos, docLanguage );
	}

	bool BraceParityStatus::filterOutOddBraces( CString& inOutOpenBraces ) const
	{
		int orgLength = inOutOpenBraces.GetLength();
		CString oddBraces = getOddBracesAsString();

		if ( !oddBraces.IsEmpty() )
			for ( int i = 0; i != inOutOpenBraces.GetLength(); )
				if ( oddBraces.Find( inOutOpenBraces[ i ] ) != -1 )
					inOutOpenBraces.Delete( i );
				else
					++i;

		return inOutOpenBraces.GetLength() != orgLength;
	}

	/**
		Return a pointer to the NEWLY added brace counter, and NULL if already found.
	*/
	BraceParityStatus::BraceCounter* BraceParityStatus::storeBrace( TCHAR brace )
	{
		BraceCounter* foundBraceCounter = findBrace( brace );

		if ( foundBraceCounter != NULL )
		{
			foundBraceCounter->countBrace( brace );
			return NULL;
		}
		else
		{
			braceCounters.push_back( BraceCounter( brace ) ); // ctor auto-counts the brace
			return &braceCounters.back();
		}
	}

	bool BraceParityStatus::checkParityErrors( void )
	{
		bool succeeded = true;

		for ( std::vector< BraceCounter >::const_iterator itBraceCounter = braceCounters.begin();
			  itBraceCounter != braceCounters.end(); ++itBraceCounter )
			if ( !itBraceCounter->IsEven() )
			{
				errorMessages.push_back( str::formatString( _T("Un-even brace pair '%c%c'; missing %d '%c' braces"),
															itBraceCounter->m_openBrace, itBraceCounter->m_closeBrace,
															abs( itBraceCounter->m_parityCounter ),
															itBraceCounter->m_parityCounter > 0 ? itBraceCounter->m_closeBrace : itBraceCounter->m_openBrace ) );
				succeeded = false;
			}

		return succeeded;
	}

	bool BraceParityStatus::isEntirelyEven( void ) const
	{
		for ( std::vector< BraceCounter >::const_iterator itBraceCounter = braceCounters.begin();
			  itBraceCounter != braceCounters.end(); ++itBraceCounter )
			if ( !itBraceCounter->IsEven() )
				return false;

		return true;
	}

	bool BraceParityStatus::isBraceEven( TCHAR brace ) const
	{
		BraceCounter* foundBraceCounter = findBrace( brace );

		return foundBraceCounter != NULL && foundBraceCounter->IsEven();
	}

	CString BraceParityStatus::getOddBracesAsString( void ) const
	{
		CString oddOpenBraces;

		for ( std::vector< BraceCounter >::const_iterator itBraceCounter = braceCounters.begin();
			  itBraceCounter != braceCounters.end(); ++itBraceCounter )
			if ( !itBraceCounter->IsEven() )
				oddOpenBraces += itBraceCounter->m_openBrace;

		return oddOpenBraces;
	}

	CString BraceParityStatus::getErrorMessage( const TCHAR* separator /*= _T("\r\n")*/ ) const
	{
		return str::unsplit( errorMessages, separator );
	}

	//------------------------------------------------------------------------------
	// BraceParityStatus::BraceCounter implementation
	//------------------------------------------------------------------------------

	BraceParityStatus::BraceCounter::BraceCounter( void )
		: m_openBrace( '\0' )
		, m_closeBrace( '\0' )
		, m_parityCounter( 0 )
	{
	}

	BraceParityStatus::BraceCounter::BraceCounter( TCHAR brace )
		: m_openBrace( '\0' )
		, m_closeBrace( '\0' )
		, m_parityCounter( 0 )
	{
		if ( code::isOpenBraceChar( brace ) )
		{
			m_openBrace = brace;
			m_closeBrace = code::getMatchingBrace( m_openBrace );
			m_parityCounter = 1; // counter goes to 1 when opening brace first added
		}
		else
		{
			ASSERT( code::isCloseBraceChar( brace ) );

			m_closeBrace = brace;
			m_openBrace = code::getMatchingBrace( m_closeBrace );
			m_parityCounter = -1; // counter goes to -1 when closing brace first added
		}
	}

	void BraceParityStatus::BraceCounter::countBrace( TCHAR brace )
	{
		if ( brace == m_openBrace )
			++m_parityCounter;
		else
			--m_parityCounter;
	}

} // namespace code