
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

	void BraceParityStatus::clear( void )
	{
		m_braceCounters.clear();
		m_errorMessages.clear();
	}

	int BraceParityStatus::findMatchingBracePos( const TCHAR* pCode, int openBracePos, DocLanguage docLanguage )
	{
		clear();

		ASSERT( pCode != NULL && openBracePos >= 0 && openBracePos <= str::Length( pCode ) );
		ASSERT( isOpenBraceChar( pCode[ openBracePos ] ) );

		storeBrace( pCode[ openBracePos ] );

		code::LanguageSearchEngine languageEngine( docLanguage );
		TCHAR matchingCloseBrace = getMatchingBrace( pCode[ openBracePos ] );
		const TCHAR* pCursor = pCode + openBracePos + 1;

		while ( *pCursor != _T('\0') )
		{
			int commentEnd;

			if ( isBraceChar( *pCursor ) )
			{
				BraceCounter* braceCounter = storeBrace( *pCursor );

				if ( braceCounter != NULL && -1 == braceCounter->m_parityCounter )
					m_errorMessages.push_back( str::formatString( _T("Closing brace '%c' encountered before the opening brace; at pos %d: [%s]"),
																braceCounter->m_closeBrace, int( pCursor - pCode ), pCursor ) );

				if ( *pCursor == matchingCloseBrace )
					if ( isBraceEven( *pCursor ) )
						break;
			}
			else if ( isQuoteChar( *pCursor ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pCode, int( pCursor - pCode ) );

				if ( matchingQuotePos != -1 )
					pCursor = pCode + matchingQuotePos;
				else
				{
					m_errorMessages.push_back( str::formatString( _T("Quoted string not closed at pos %d: [%s]"),
																int( pCursor - pCode ), pCursor ) );
					pCursor += str::Length( pCursor ); // fatal error -> go to end
				}
			}
			else if ( languageEngine.isCommentStatement( commentEnd, pCode, int( pCursor - pCode ) ) )
			{
				pCursor = pCode + str::safePos( commentEnd, pCode );
				continue; // skip incrementing
			}

			++pCursor;
		}

		checkParityErrors();

		if ( *pCursor == _T('\0') )
		{
			m_errorMessages.push_back( str::formatString( _T("Closing brace not found for opening brace '%c' at pos %d"),
														pCode[ openBracePos ], openBracePos ) );
			return -1;						// reached end of string, closing brace not found
		}

		return int( pCursor - pCode );		// found the matching brace -> return the position
	}

	struct BraceReverser : public std::unary_function<void, TCHAR>
	{
		void operator()( TCHAR& chr )
		{
			if ( code::isBraceChar( chr ) )
				chr = code::getMatchingBrace( chr );
		}
	};

	int BraceParityStatus::reverseFindMatchingBracePos( const TCHAR* pCode, int openBracePos, DocLanguage docLanguage )
	{
		// build just the leading  part of the original string, but reversed
		std::tstring revCode( pCode, openBracePos + 1 );

		std::reverse( revCode.begin(), revCode.end() );
		std::for_each( revCode.begin(), revCode.end(), BraceReverser() );

		int revMatchingBracePos = findMatchingBracePos( revCode.c_str(), 0, docLanguage );

		return revMatchingBracePos != -1 ? ( openBracePos - revMatchingBracePos /*- 1*/ ) : -1;
	}

	TokenRange BraceParityStatus::findArgList( const TCHAR* pCode, int pos, const TCHAR* argListOpenBraces,
											   DocLanguage docLanguage, bool allowUnclosedArgList /*= false*/ )
	{
		ASSERT( pCode != NULL && pos >= 0 && pos <= str::Length( pCode ) );
		ASSERT( argListOpenBraces != NULL && argListOpenBraces[ 0 ] != _T('\0') );

		code::LanguageSearchEngine languageEngine( docLanguage );
		int openBracePos = languageEngine.findOneOf( pCode, argListOpenBraces, pos );

		ASSERT( openBracePos != -1 );
		if ( pCode[ openBracePos ] != _T('\0') )
		{
			int closeBracePos = findMatchingBracePos( pCode, openBracePos, docLanguage );

			if ( closeBracePos != -1 )
				return TokenRange( openBracePos, closeBracePos + 1 );
			else if ( allowUnclosedArgList )
				return TokenRange( openBracePos, str::Length( pCode ) );
		}

		return TokenRange::endOfString( pCode );
	}

	/**
		Analyzes the syntax for parity of the braces/quotes, returning true if syntax is correct.
		Errors (if any) are stored in 'm_errorMessages' data member.
	*/
	bool BraceParityStatus::analyzeBraceParity( const TCHAR* pCode, DocLanguage docLanguage )
	{
		ASSERT_PTR( pCode );

		code::LanguageSearchEngine languageEngine( docLanguage );

		int firstOpenBracePos = languageEngine.findOneOf( pCode, code::openBraces, 0 );

		ASSERT( firstOpenBracePos != -1 );

		return pCode[ firstOpenBracePos ] != _T('\0') &&
			   -1 != findMatchingBracePos( pCode, firstOpenBracePos, docLanguage );
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
			m_braceCounters.push_back( BraceCounter( brace ) ); // ctor auto-counts the brace
			return &m_braceCounters.back();
		}
	}

	bool BraceParityStatus::checkParityErrors( void )
	{
		bool succeeded = true;

		for ( std::vector<BraceCounter>::const_iterator itBraceCounter = m_braceCounters.begin();
			  itBraceCounter != m_braceCounters.end(); ++itBraceCounter )
			if ( !itBraceCounter->IsEven() )
			{
				m_errorMessages.push_back( str::formatString( _T("Un-even brace pair '%c%c'; missing %d '%c' braces"),
															itBraceCounter->m_openBrace, itBraceCounter->m_closeBrace,
															abs( itBraceCounter->m_parityCounter ),
															itBraceCounter->m_parityCounter > 0 ? itBraceCounter->m_closeBrace : itBraceCounter->m_openBrace ) );
				succeeded = false;
			}

		return succeeded;
	}

	bool BraceParityStatus::isEntirelyEven( void ) const
	{
		for ( std::vector<BraceCounter>::const_iterator itBraceCounter = m_braceCounters.begin();
			  itBraceCounter != m_braceCounters.end(); ++itBraceCounter )
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

		for ( std::vector<BraceCounter>::const_iterator itBraceCounter = m_braceCounters.begin();
			  itBraceCounter != m_braceCounters.end(); ++itBraceCounter )
			if ( !itBraceCounter->IsEven() )
				oddOpenBraces += itBraceCounter->m_openBrace;

		return oddOpenBraces;
	}

	CString BraceParityStatus::getErrorMessage( const TCHAR* separator /*= _T("\r\n")*/ ) const
	{
		return str::unsplit( m_errorMessages, separator );
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
