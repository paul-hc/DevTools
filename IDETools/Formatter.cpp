// Copyleft 2004 Paul Cocoveanu
//
#include "pch.h"
#include "Application.h"
#include "Formatter.h"
#include "StringUtilitiesEx.h"
#include "CodeUtilities.h"
#include "LanguageSearchEngine.h"
#include "BraceParityStatus.h"
#include "FormatterOptions.h"
#include "ModuleSession.h"
#include "utl/RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	const std::tstring CFormatter::s_cancelTag = _T("<cancel>");


	CFormatter::CFormatter( const CFormatterOptions& options )
		: m_options( options )
		, m_multiWhitespacePolicy( UseOptionsPolicy ) // allows overriding for splitting/C++ implementation
		, m_docLanguage( DocLang_None )
		, m_tabSize( 4 )
		, m_useTabs( true )
		, m_languageEngine( m_docLanguage )
		, m_disableBracketSpacingCounter( 0 )
	{
	}

	CFormatter::~CFormatter()
	{
	}

	void CFormatter::resetInternalState( void )
	{
		m_validArgListOpenBraces = m_options.GetArgListOpenBraces().c_str();
	}

	void CFormatter::setDocLanguage( const TCHAR* tagDocLanguage )
	{
		static struct { const TCHAR* tag; DocLanguage m_docLanguage; } vsDocLanguages[] =
		{
			{ _T(""), DocLang_None },
			{ _T("None"), DocLang_None },
			{ _T("C/C++"), DocLang_Cpp },
			{ _T("VBS Macro"), DocLang_Basic },
			{ _T("ODBC SQL"), DocLang_SQL },
			{ _T("Oracle PL/SQL"), DocLang_SQL },
			{ _T("Microsoft SQL"), DocLang_SQL },
			{ _T("HTML - IE 3.0"), DocLang_HtmlXml },
			{ _T("HTML 2.0 (RFC 1866)"), DocLang_HtmlXml },
			{ _T("HTML Server-Side Script"), DocLang_HtmlXml },
			{ _T("IDL/ODL"), DocLang_IDL }
		};

		m_docLanguage = DocLang_None;

		for ( int i = 0; i != COUNT_OF( vsDocLanguages ); ++i )
			if ( 0 == _tcsicmp( tagDocLanguage, vsDocLanguages[ i ].tag ) )
			{
				setDocLanguage( vsDocLanguages[ i ].m_docLanguage );
				break;
			}
	}

	void CFormatter::setDocLanguage( DocLanguage docLanguage )
	{
		m_docLanguage = docLanguage;
		m_languageEngine.setDocLanguage( m_docLanguage );
	}

	CString CFormatter::formatCode( const TCHAR* pCodeText, bool protectLeadingWhiteSpace /*= true*/, bool justAdjustWhiteSpace /*= false*/ )
	{
		resetInternalState();

		std::vector< CString > linesOfCode, lineEnds;

		// break the code into lines
		splitMultipleLines( linesOfCode, lineEnds, pCodeText );

		// format each line
		for ( std::vector< CString >::iterator itLineOfCode = linesOfCode.begin(); itLineOfCode != linesOfCode.end(); ++itLineOfCode )
			*itLineOfCode = formatLineOfCode( *itLineOfCode, protectLeadingWhiteSpace, justAdjustWhiteSpace );

		return unsplitMultipleLines( linesOfCode, lineEnds );
	}

	// formats a line of code (no line-ends inside, please)
	//
	CString CFormatter::formatLineOfCode( const TCHAR* lineOfCode, bool protectLeadingWhiteSpace /*= true*/,
										 bool justAdjustWhiteSpace /*= false*/ )
	{
		ASSERT_PTR( lineOfCode );

		TokenRange formatTargetRange( lineOfCode );

		if ( m_options.m_deleteTrailingWhiteSpace )
			while ( formatTargetRange.m_end > formatTargetRange.m_start && code::isWhitespaceChar( lineOfCode[ formatTargetRange.m_end - 1 ] ) )
				--formatTargetRange.m_end;

		if ( protectLeadingWhiteSpace )
			while ( formatTargetRange.m_start < formatTargetRange.m_end && code::isWhitespaceChar( lineOfCode[ formatTargetRange.m_start ] ) )
				++formatTargetRange.m_start;

		// Tabify leading whitespaces according to 'm_useTabs'
		CString leadingWhiteSpace = makeLineIndentWhiteSpace( computeVisualEditorColumn( lineOfCode,
																						   formatTargetRange.m_start ) - 1 );
		CString targetCode = formatTargetRange.getString( lineOfCode );
		CString coreFormatedCode = justAdjustWhiteSpace ? doAdjustWhitespaceLineOfCode( targetCode ) : doFormatLineOfCode( targetCode );

		return leadingWhiteSpace + coreFormatedCode;
	}

	CString CFormatter::tabifyLineOfCode( const TCHAR* lineOfCode, bool doTabify /*= true*/ )
	{
		ASSERT_PTR( lineOfCode );

		TokenRange leadingWhiteSpaceRange = getWhiteSpaceRange( lineOfCode, 0, false );

		TEditorColumn editorColumn = computeVisualEditorColumn( lineOfCode, leadingWhiteSpaceRange.m_end );
		CString leadingWhiteSpace = makeLineIndentWhiteSpace( editorColumn - 1, doTabify );

		return leadingWhiteSpace + leadingWhiteSpaceRange.getSuffixString( lineOfCode );
	}

	CString CFormatter::splitArgumentList( const TCHAR* pCodeText, TEditorColumn maxColumn /*= UINT_MAX*/,
										   int targetBracketLevel /*= -1*/ )
	{
		UNUSED_ALWAYS( targetBracketLevel );

		resetInternalState();

		// zero-based index
		int maxEditorColIndex = std::max( maxColumn != UINT_MAX ? (int)maxColumn : (int)app::GetModuleSession().m_splitMaxColumn, 1 ) - 1;
		std::vector< CString > brokenLines;

		brokenLines.push_back( makeNormalizedFormattedPrototype( pCodeText ) );

		for ( TokenRange breakToken( 0 ); str::charAt( brokenLines.back(), breakToken.m_end ) != _T('\0'); )
			if ( findLineBreakToken( &breakToken, brokenLines.back(), breakToken.m_end ) == LBT_OpenBrace )
				breakToken.m_end = doSplitArgumentList( brokenLines, breakToken, maxEditorColIndex );

		if ( m_useTabs )
			for ( std::vector< CString >::iterator itLine = brokenLines.begin(); itLine != brokenLines.end(); ++itLine )
				*itLine = tabifyLineOfCode( *itLine );

		brokenLines.push_back( CString() );
		return getArgListCodeText( brokenLines );
	}

	/**
		Sorry, not finished and buggy for now! The VBScript macro seems to be superior...
	*/
	CString CFormatter::toggleComment( const TCHAR* pCodeText )
	{
		resetInternalState();

		CString newCodeText = pCodeText;

		code::convertToWindowsLineEnds( newCodeText );

		CommentState commentState = NoComment;
		int commentEndPos;

		if ( m_languageEngine.isCommentStatement( commentEndPos, newCodeText, 0 ) )
			commentState = m_languageEngine.isSingleLineCommentStatement( newCodeText, 0 ) ? SingleLineComment : MultiLineComment;

		const CommentTokens& langCommentTokens = CommentTokens::getLanguageSpecific( m_docLanguage );
		std::vector< CString > linesOfCode, lineEnds;

		splitMultipleLines( linesOfCode, lineEnds, newCodeText );
		if ( !linesOfCode.empty() )
			if ( 1 == linesOfCode.size() )
			{	// Selection has no line-ends -> toggle multi-line comment on/off
				if ( !langCommentTokens.hasMultiLineComment() )
					return newCodeText;

				if ( commentState == NoComment )
					return comment( newCodeText, false, MultiLineComment );
				else
					return uncomment( newCodeText, false );
			}
			if ( 2 == linesOfCode.size() )
			{	// Single-line selection -> toggle single-line comment on/off
				if ( commentState == NoComment )
					linesOfCode.front() = comment( linesOfCode.front(), true, SingleLineComment );
				else
					linesOfCode.front() = uncomment( linesOfCode.front(), true );
				return unsplitMultipleLines( linesOfCode, lineEnds );
			}
			else
			{	// Multi-line selection -> cicle through NoComment/SingleLineComment/MultiLineComment,
				switch ( commentState )
				{
					default:
						ASSERT( false );
					case NoComment:
						if ( langCommentTokens.hasSingleLineComment() )
						{
							for ( std::vector< CString >::iterator itLineOfCode = linesOfCode.begin(), itLast = linesOfCode.end() - 1;
								  itLineOfCode != itLast; ++itLineOfCode )
								*itLineOfCode = comment( *itLineOfCode, true, SingleLineComment );
							return unsplitMultipleLines( linesOfCode, lineEnds );
						}
						else
							return comment( newCodeText, true, MultiLineComment );
					case SingleLineComment:
						if ( langCommentTokens.hasMultiLineComment() )
						{
							for ( std::vector< CString >::iterator itLineOfCode = linesOfCode.begin(), itLast = linesOfCode.end() - 1;
								  itLineOfCode != itLast; ++itLineOfCode )
								*itLineOfCode = uncomment( *itLineOfCode, true );

							CString uncommentedLines = unsplitMultipleLines( linesOfCode, lineEnds );
							return comment( uncommentedLines, true, MultiLineComment );
						}
						else
							return uncomment( newCodeText, true );
					case MultiLineComment:
						return uncomment( newCodeText, true );
				}
			}
	}

	CString CFormatter::generateConsecutiveNumbers( const TCHAR* pCodeText, unsigned int startingNumber /*= UINT_MAX*/ ) throws_( CRuntimeException )
	{
		std::vector< CString > linesOfCode, lineEnds;

		// break the code into lines
		splitMultipleLines( linesOfCode, lineEnds, pCodeText );

		size_t lineCount = linesOfCode.size();

		if ( lineCount > 0 && linesOfCode.back().IsEmpty() )
			--lineCount;

		if ( lineCount < 2 )
			throw CRuntimeException( _T("You must select at least 2 lines of code!") );

		FormattedNumber< unsigned int > number( startingNumber, _T("%u") );

		if ( UINT_MAX == startingNumber )
			number = m_languageEngine.extractNumber( linesOfCode.front() );

		// format each line
		for ( std::vector< CString >::iterator itLine = linesOfCode.begin() + 1; itLine != linesOfCode.end(); ++itLine )
		{
			itLine->TrimLeft();
			itLine->TrimRight();

			if ( !itLine->IsEmpty() )
			{
				++number.m_number;

				TokenRange numberRange = m_languageEngine.findNextNumber( *itLine );

				ASSERT( numberRange.IsValid() && !numberRange.IsEmpty() );

				numberRange.replaceWithToken( &*itLine, number.formatString() );
			}
		}

		return unsplitMultipleLines( linesOfCode, lineEnds );
	}

	CString CFormatter::sortLines( const TCHAR* pCodeText, bool ascending ) throws_( CRuntimeException )
	{
		std::vector< CString > linesOfCode, lineEnds;

		// break the code into lines
		splitMultipleLines( linesOfCode, lineEnds, pCodeText );

		size_t lineCount = linesOfCode.size();

		if ( lineCount > 0 && linesOfCode.back().IsEmpty() )
			--lineCount;

		if ( lineCount < 2 )
			throw CRuntimeException( _T("You must select at least 2 lines of code!") );

		size_t lastBlankOffset = 0;
		{
			CString lastLine = linesOfCode.back();
			lastLine.TrimLeft();
			lastLine.TrimRight();

			if ( lastLine.IsEmpty() )
				lastBlankOffset = 1;				// exclude blank last line from sorting
		}

		if ( ascending )
			std::stable_sort( linesOfCode.begin(), linesOfCode.end() - lastBlankOffset, pred::TLess_NaturalPath() );		// str::StringLess()
		else
			std::stable_sort( linesOfCode.rbegin() + lastBlankOffset, linesOfCode.rend(), pred::TLess_NaturalPath() );	// str::StringGreater()

		return unsplitMultipleLines( linesOfCode, lineEnds );
	}

	/**
		Does the core formatting of a line of code (no line-ends inside, please)
	*/
	CString CFormatter::doFormatLineOfCode( const TCHAR lineOfCode[] )
	{
		CString outCode = lineOfCode;

		for ( int pos = 0; str::charAt( outCode, pos ) != _T('\0'); )
		{
			TCHAR chr = str::charAt( outCode, pos );
			int statementEndPos;

			if ( code::isWhitespaceChar( chr ) )
			{
				if ( !m_languageEngine.isProtectedLineTermination( statementEndPos, outCode, pos ) )
					pos = replaceMultipleWhiteSpace( outCode, pos );
				else
					pos = str::safePos( statementEndPos, outCode ); // reached the protected line termination: [whitespaces] [comment] [whitespaces] [line-end]
			}
			else if ( IsBraceCharAt( outCode, pos ) )
			{
				if ( chr == _T('(') )
					if ( m_multiWhitespacePolicy == UseSplitPrototypePolicy )
						m_multiWhitespacePolicy = ReplaceMultipleWhiteSpace; // reset the whitespace protection for the prototype argument list, until the end

				pos = formatBrace( outCode, pos );
			}
			else if ( code::isQuoteChar( chr ) )
			{	// skip quoted string
				int matchingQuotePos = code::findMatchingQuotePos( outCode, pos );

				if ( matchingQuotePos != -1 )
					pos = matchingQuotePos + 1;
				else
					pos = outCode.GetLength(); // fatal syntax error -> abort
			}
			else if ( m_languageEngine.isCommentStatement( statementEndPos, outCode, pos ) )
				pos = str::safePos( statementEndPos, outCode );
			else if ( m_docLanguage == DocLang_Cpp && m_languageEngine.isUnicodePortableStringConstant( statementEndPos, outCode, pos ) )
				pos = formatUnicodePortableStringConstant( outCode, pos );
			else
				pos = formatDefault( outCode, pos );
		}

		return outCode;
	}

	CString CFormatter::doAdjustWhitespaceLineOfCode( const TCHAR lineOfCode[] )
	{
		CString outCode = lineOfCode;

		for ( int pos = 0; str::charAt( outCode, pos ) != _T('\0'); )
		{
			TCHAR chr = str::charAt( outCode, pos );
			int statementEndPos;

			if ( code::isWhitespaceChar( chr ) )
			{
				if ( !m_languageEngine.isProtectedLineTermination( statementEndPos, outCode, pos ) )
					pos = replaceMultipleWhiteSpace( outCode, pos );
				else
					pos = str::safePos( statementEndPos, outCode ); // reached the protected line termination: [whitespaces] [comment] [whitespaces] [line-end]
			}
			else if ( code::isQuoteChar( chr ) )
			{	// skip quoted string
				int matchingQuotePos = code::findMatchingQuotePos( outCode, pos );

				if ( matchingQuotePos != -1 )
					pos = matchingQuotePos + 1;
				else
					pos = outCode.GetLength(); // fatal syntax error -> abort
			}
			else if ( m_languageEngine.isCommentStatement( statementEndPos, outCode, pos ) )
				pos = str::safePos( statementEndPos, outCode );
			else
				++pos;
		}

		return outCode;
	}

	bool CFormatter::IsBraceCharAt( const TCHAR code[], size_t pos ) const
	{
		TCHAR chr = str::charAt( code, pos );

		if ( !isValidBraceChar( chr, m_validArgListOpenBraces ) )
			return false;

		// do we have a matching operator with length greater than of a brace?
		if ( const CFormatterOptions::COperatorRule* pOpRule = m_options.FindOperatorRule( str::ptrAt( code, pos ) ) )
			if ( pOpRule->GetOperatorLength() > 1 )
				return false;							// operator match takes precedence, so not a brace

		return true;
	}

	TokenSpacing CFormatter::MustSpaceBrace( TCHAR chrBrace ) const
	{
		ASSERT( m_disableBracketSpacingCounter >= 0 );

		if ( m_disableBracketSpacingCounter > 0 )
			if ( chrBrace == _T('(') || chrBrace == _T(')') )
				return RemoveSpace; // don't space () in C cast statements

		return m_options.MustSpaceBrace( chrBrace );
	}

	/**
		Replaces multiple whitespaces (space and tab) with one single space
	*/
	int CFormatter::replaceMultipleWhiteSpace( CString& targetString, int pos, const TCHAR* newWhitespace /*= _T(" ")*/ )
	{
		TokenRange whitespaces( pos );

		if ( code::isWhitespaceChar( str::charAt( targetString, pos ) ) )
			whitespaces.m_end = m_languageEngine.findIfNot( targetString, pos, code::isWhitespaceChar );

		bool preserveMultipleWhiteSpace = m_options.m_preserveMultipleWhiteSpace;

		switch ( m_multiWhitespacePolicy )
		{
			case UseOptionsPolicy:
				break;
			case ReplaceMultipleWhiteSpace:
				preserveMultipleWhiteSpace = false;
				break;
			case UseSplitPrototypePolicy:
				/**
					Preserve multiple whitespace for the leading part of the prototype, that is up to the argument list.
					This ensures that the leading part of a tabbed method declaration will be preserved:
					Example:
						CString			someMethodPrototype( const TCHAR* text, bool arg1 = true, bool arg2 = false );
					will be split to:
						CString			someMethodPrototype( const TCHAR* text, bool arg1 = true,
															 bool arg2 = false );
				*/

				preserveMultipleWhiteSpace = true;
				break;
		}

		if ( preserveMultipleWhiteSpace &&
			 ( whitespaces.getLength() > 1 && 1 == str::Length( newWhitespace ) ) )
		{	// preserve multiple whitespaces, e.g. avoid replacing "    " with " "
		}
		else
			whitespaces.smartReplaceWithToken( &targetString, newWhitespace );

		return whitespaces.m_end;
	}

	/**
		Inserts/removes whitespace(s) AFTER pos according to 'mustSpaceIt'
	*/
	int CFormatter::resolveSpaceAfterToken( CString& targetString, const TokenRange& tokenRange, bool mustSpaceIt )
	{
		ASSERT( !code::isWhitespaceChar( str::charAt( targetString, tokenRange.m_start ) ) );

		int statementEndPos;

		if ( m_languageEngine.isProtectedLineTermination( statementEndPos, targetString, tokenRange.m_end ) )
			return str::safePos( statementEndPos, targetString ); // reached the protected line termination: [whitespaces] [comment] [whitespaces] [line-end]

		return replaceMultipleWhiteSpace( targetString, tokenRange.m_end, mustSpaceIt ? _T(" ") : _T("") );
	}

	/**
		Inserts/removes whitespace(s) BEFORE pos according to 'mustSpaceIt'
	*/
	int CFormatter::resolveSpaceBeforeToken( CString& targetString, const TokenRange& tokenRange, bool mustSpaceIt )
	{
		ASSERT( !code::isWhitespaceChar( str::charAt( targetString, tokenRange.m_start ) ) );

		int beforePos = tokenRange.m_start;

		while ( beforePos > 0 && code::isWhitespaceChar( str::charAt( targetString, beforePos - 1 ) ) )
			--beforePos;

		if ( mustSpaceIt && tokenRange.m_start == 0 )
			return tokenRange.m_end; // We don't space before when operator is at the beggining of the line

		// return the position corresponding to the end of the original token
		return replaceMultipleWhiteSpace( targetString, beforePos, mustSpaceIt ? _T(" ") : _T("") ) + tokenRange.getLength();
	}

	int CFormatter::formatBrace( CString& targetString, int pos )
	{
		ASSERT( pos != -1 && code::isBraceChar( str::charAt( targetString, pos ) ) );

		TCHAR chrBrace = str::charAt( targetString, pos );
		TokenSpacing mustSpaceIt = MustSpaceBrace( chrBrace );

		if ( mustSpaceIt == PreserveSpace )
			return pos + 1; // skip to the next char

		if ( isOpenBraceChar( chrBrace ) )
		{
			TCHAR closeBrace = code::getMatchingBrace( chrBrace );
			int nextNonWhitespacePos = pos + 1;

			while ( code::isWhitespaceChar( str::charAt( targetString, nextNonWhitespacePos ) ) )
				++nextNonWhitespacePos;

			if ( str::charAt( targetString, nextNonWhitespacePos ) == closeBrace )
			{
				TCHAR emptyBraces[] = { chrBrace, closeBrace, _T('\0') };
				TokenRange emptyBraceRange( pos, nextNonWhitespacePos + 1 );

				return emptyBraceRange.smartReplaceWithToken( &targetString, emptyBraces ).m_end;
			}

			if ( chrBrace == _T('(') )
			{
				int statementEndPos;

				if ( m_docLanguage == DocLang_Cpp && m_languageEngine.isCCastStatement( statementEndPos, targetString, pos ) )
				{	// format the cast statement, which can be nested or cascaded.
					++m_disableBracketSpacingCounter; // temporarily disable '()' brace spacing

					TokenRange statementRange( pos, statementEndPos );
					CString formattedCastStatement = doFormatLineOfCode( statementRange.getString( targetString ) );

					--m_disableBracketSpacingCounter;

					statementRange.smartReplaceWithToken( &targetString, formattedCastStatement );
					return statementRange.m_end;
				}
			}

			return resolveSpaceAfterToken( targetString, TokenRange( pos, pos + 1 ), mustSpaceIt != RemoveSpace );
		}
		else
			return resolveSpaceBeforeToken( targetString, TokenRange( pos, pos + 1 ), mustSpaceIt != RemoveSpace ); // after closing brace
	}

	int CFormatter::formatUnicodePortableStringConstant( CString& targetString, int pos )
	{
		++m_disableBracketSpacingCounter;

		int openBracePos = targetString.Find( _T('('), pos );

		ASSERT( openBracePos != -1 );
		formatBrace( targetString, openBracePos );

		int closeBracePos = code::BraceParityStatus().findMatchingBracePos( targetString, openBracePos, m_docLanguage );

		ASSERT( closeBracePos != -1 );
		pos = formatBrace( targetString, closeBracePos );

		--m_disableBracketSpacingCounter;
		return pos;
	}

	int CFormatter::formatDefault( CString& targetString, int pos )
	{
		if ( _istpunct( str::charAt( targetString, pos ) ) )
			if ( const CFormatterOptions::COperatorRule* pOpRule = m_options.FindOperatorRule( str::ptrAt( targetString, pos ) ) )
			{
				int operatorLength = str::Length( pOpRule->m_pOperator );

				if ( m_languageEngine.isTokenMatchBefore( targetString, pos, _T("operator") ) )
					pos += operatorLength;
				else
				{
					if ( pOpRule->m_spaceBefore != PreserveSpace )
					{
						pos = resolveSpaceBeforeToken( targetString, TokenRange( pos, pos + operatorLength ),
													   pOpRule->m_spaceBefore != RemoveSpace );
						pos -= operatorLength;
					}

					if ( pOpRule->m_spaceBefore != PreserveSpace )
						pos = resolveSpaceAfterToken( targetString, TokenRange( pos, pos + operatorLength ),
													  pOpRule->m_spaceAfter != RemoveSpace );
					else
						pos += operatorLength;
				}

				return pos;
			}

		return ++pos;
	}

	int CFormatter::splitMultipleLines( std::vector< CString >& outLinesOfCode, std::vector< CString >& outLineEnds, const TCHAR* pCodeText )
	{
		if ( pCodeText != nullptr && pCodeText[ 0 ] != _T('\0') )
			for ( int pos = 0; ; )
			{
				TokenRange endOfLinePos = str::findStringPos( pCodeText, code::lineEnd, pos );

				if ( endOfLinePos.m_start > 1 )
					if ( code::isLineBreakEscapeChar( pCodeText[ endOfLinePos.m_start - 1 ], m_docLanguage ) )
						--endOfLinePos.m_start; // e.g. include "_\r\n" in BASIC line end

				if ( endOfLinePos.m_start != -1 )
				{
					TokenRange lineRange( pos, endOfLinePos.m_start );

					outLinesOfCode.push_back( lineRange.getString( pCodeText ) );
					outLineEnds.push_back( endOfLinePos.getString( pCodeText ) );
					pos = endOfLinePos.m_end;
				}
				else
				{
					TokenRange lastLineRange( pCodeText, pos );

					outLinesOfCode.push_back( lastLineRange.getString( pCodeText ) );
					outLineEnds.push_back( CString() );
					break;
				}
			}

		return (int)outLinesOfCode.size();
	}

	CString CFormatter::unsplitMultipleLines( const std::vector< CString >& linesOfCode, const std::vector< CString >& lineEnds,
											 int lineCount /*= -1*/ ) const
	{
		if ( lineCount == -1 )
			lineCount = (int)linesOfCode.size();

		ASSERT( linesOfCode.size() == lineEnds.size() );
		ASSERT( lineCount <= (int)linesOfCode.size() );

		int requiredLength = 0;

		for ( int i = 0; i != lineCount; ++i )
			requiredLength += linesOfCode[ i ].GetLength() + lineEnds[ i ].GetLength();

		CString outCodeText;

		outCodeText.GetBuffer( requiredLength );
		outCodeText.ReleaseBuffer( requiredLength );

		outCodeText.Empty();

		for ( int i = 0; i != lineCount; ++i )
		{
			outCodeText += linesOfCode[ i ];
			outCodeText += lineEnds[ i ];
		}

		return outCodeText;
	}

	CString CFormatter::getArgListCodeText( const std::vector< CString >& linesOfCode ) const
	{
		CString resultCodeText = str::unsplit( linesOfCode,
											   m_docLanguage != DocLang_Basic ? code::lineEnd : code::basicMidLineEnd );

		if ( m_docLanguage == DocLang_Basic )
		{
			TokenRange lastBasicLineEndRange = str::reverseFindStringPos( resultCodeText, code::basicMidLineEnd );

			if ( lastBasicLineEndRange.m_end == resultCodeText.GetLength() )
				resultCodeText.Delete( lastBasicLineEndRange.m_start, 1 ); // Remove '_' from the last '_\r\n'
		}

		return resultCodeText;
	}

	/**
		- Takes as input a multiple line unformatted method prototype (or implementation).
		- Converts line-ends to Windows line-ends by stripping Unix line-ends.
		- Strips BASIC line breakers '_' at end of line.
		- Splits the resulting prototype into multiple lines, replaces terminal single-line
		  comments into multi-line comments if available in the language, otherwise removes them.
		- Than builds a single line by concatenating each broken line.
		- Analyses the brace syntax for parity, and filters-out from m_validArgListOpenBraces
		  the odd braces.
		- Formats the single line using current formatting rules, but without preserving multiple whitespaces
		  inside the argument list.
		- Untabifies the leading whitespaces so index is equal to editor visual index
		- The output contains the single line prototype, that is pre-formatted and has the original leading whitespaces.
	*/
	CString CFormatter::makeNormalizedFormattedPrototype( const TCHAR* pMethodProto, bool forImplementation /*= false*/ )
	{
		CString normalizedCode = pMethodProto;

		code::convertToWindowsLineEnds( normalizedCode );

		if ( DocLang_Basic == m_docLanguage )
			normalizedCode.Replace( code::basicMidLineEnd, code::lineEnd );

		std::vector< CString > linesOfCode, __lineEnds;

		// break the code into lines
		splitMultipleLines( linesOfCode, __lineEnds, normalizedCode );

		// Build the normalized line by concatenating each line.
		normalizedCode.Empty();

		for ( std::vector< CString >::iterator itLineOfCode = linesOfCode.begin(); itLineOfCode != linesOfCode.end(); ++itLineOfCode )
		{
			CString line = transformTrailingSingleLineComment( *itLineOfCode, forImplementation ? RemoveComment : ToMultiLineComment );

			if ( forImplementation )
				cppFilterPrototypeForImplementation( line );

			if ( !line.IsEmpty() )
				normalizedCode += line;
		}

		{	// Analyse brace syntax for parity
			code::BraceParityStatus braceStatus;

			braceStatus.analyzeBraceParity( normalizedCode, m_docLanguage );
			if ( braceStatus.filterOutOddBraces( m_validArgListOpenBraces ) )
				TRACE( str::formatString( _T("Filter-out odd braces '%s'\nREASONS:\n%s\n"),
										  (LPCTSTR)braceStatus.getOddBracesAsString(),
										  (LPCTSTR)braceStatus.getErrorMessage( _T("\n") ) ) );
		}

		MultiWhitespacePolicy org_multiWhitespacePolicy = m_multiWhitespacePolicy;
		// Temporarily overwrite whitespace replacement policy
		m_multiWhitespacePolicy = forImplementation ? ReplaceMultipleWhiteSpace : UseSplitPrototypePolicy;

		normalizedCode = formatLineOfCode( normalizedCode, true );	// format the normalized line
		normalizedCode = tabifyLineOfCode( normalizedCode, false );	// untabify leading whitespaces in order to make true editor indexes workable

		m_multiWhitespacePolicy = org_multiWhitespacePolicy;

		return normalizedCode;
	}

	/**
		Replaces terminal single-line comments into multi-line comments if available in the language, otherwise removes them.
	*/
	CString CFormatter::transformTrailingSingleLineComment( const TCHAR* lineOfCode, HandleSingleLineComments handleComments /*= ToMultiLineComment*/ )
	{
		TokenRange lastCommentRange( 0 );
		CString outCode = lineOfCode;

		for ( int pos = 0; str::charAt( outCode, pos ) != _T('\0'); )
		{
			TokenRange commentRange = m_languageEngine.findComment( outCode, pos );

			if ( !commentRange.IsEmpty() )
				lastCommentRange = commentRange;

			pos = commentRange.m_end;
		}

		int statementEndPos;

		if ( !lastCommentRange.IsEmpty() )
			if ( m_languageEngine.isProtectedLineTermination( statementEndPos, outCode, lastCommentRange.m_start ) )
				if ( handleComments == RemoveComment )
				{
					while ( lastCommentRange.m_start > 0 && code::isWhitespaceChar( str::charAt( outCode, lastCommentRange.m_start - 1 ) ) )
						--lastCommentRange.m_start;
					lastCommentRange.replaceWithToken( &outCode, _T("") );
				}
				else if ( handleComments == ToMultiLineComment )
				{
					while ( lastCommentRange.m_end > 0 && code::isWhitespaceChar( str::charAt( outCode, lastCommentRange.m_end - 1 ) ) )
						--lastCommentRange.m_end;

					CString comment = lastCommentRange.getString( outCode );

					while ( lastCommentRange.m_start > 0 && code::isWhitespaceChar( str::charAt( outCode, lastCommentRange.m_start - 1 ) ) )
						--lastCommentRange.m_start;

					// Delete the trailing [whitespaces] [comment] [whitespaces]
					outCode = lastCommentRange.getPrefixString( outCode );

					if ( !comment.IsEmpty() )
						switch ( m_docLanguage )
						{
							case DocLang_Cpp:
							case DocLang_IDL:
								if ( 0 == _tcsncmp( comment, _T("//"), 2 ) )
								{
									bool isSpaced = str::charAt( comment, 2 ) == _T(' ');

									comment.SetAt( 1, _T('*') );
									if ( isSpaced )
										comment += _T(" ");
									comment += _T("*/");
								}
								break;
							case DocLang_None:
							case DocLang_Basic:
								comment.Empty(); // no multi-line comment
								break;
							case DocLang_SQL:
								if ( 0 == _tcsncmp( comment, _T("--"), 2 ) )
								{
									bool isSpaced = str::charAt( comment, 2 ) == _T(' ');

									comment.SetAt( 0, _T('/') );
									comment.SetAt( 1, _T('*') );
									if ( isSpaced )
										comment += _T(" ");
									comment += _T("*/");
								}
								break;
							case DocLang_HtmlXml:
								break; // multi-line comments only, preserve the existing comment (if any)
						}

					if ( !comment.IsEmpty() )
					{
						if ( !outCode.IsEmpty() )
							outCode += _T(" ");
						outCode += comment;
					}
				}

		return outCode;
	}

	int CFormatter::doSplitArgumentList( std::vector< CString >& brokenLines, const TokenRange& openBraceRange, int maxEditorColIndex )
	{
		ASSERT( openBraceRange.getLength() == 1 );

		TCHAR openBrace = brokenLines.back()[ openBraceRange.m_start ];
		TCHAR closeBrace = code::getMatchingBrace( openBrace );

		if ( str::charAt( brokenLines.back(), openBraceRange.m_end ) == closeBrace )
			return openBraceRange.m_end + 1; // empty argument list, e.g. "()" -> skip it

		TokenRange breakToken = openBraceRange;

		while ( code::isWhitespaceChar( brokenLines.back()[ breakToken.m_end ] ) )
			++breakToken.m_end;

		int alignAtIndex = breakToken.m_end;
		int leadingWhitespaceCount = computeVisualEditorIndex( brokenLines.back(), alignAtIndex );
		CString breakLeadingWhiteSpace( _T(' '), leadingWhitespaceCount );

		for ( TokenRange prevBreakSepRange( -1 ); ; )
		{
			CString& currentLine = brokenLines.back();
			LineBreakTokenMatch match = findLineBreakToken( &breakToken, currentLine, breakToken.m_end );

			switch ( match )
			{
				case LBT_OpenBrace:
					// Recurse for nested argument lists
					breakToken.setEmpty( doSplitArgumentList( brokenLines, breakToken, maxEditorColIndex ) );
					prevBreakSepRange.setEmpty( -1 );
					break;

				case LBT_CloseBrace:
					ASSERT( str::charAt( currentLine, breakToken.m_start ) == closeBrace );
					if ( !prevBreakSepRange.IsValid() )
						return breakToken.m_end;
					// continue processing as for a separator
				case LBT_BreakSeparator:
				{
					breakToken.m_start = breakToken.m_end;

					int breakTokenEndEditorIndex = computeVisualEditorIndex( brokenLines.back(), breakToken.m_end );

					if ( breakTokenEndEditorIndex > maxEditorColIndex )
					{
						if ( prevBreakSepRange.IsValid() )
							breakToken = prevBreakSepRange;

						while ( code::isWhitespaceChar( str::charAt( currentLine, breakToken.m_end ) ) )
							++breakToken.m_end;

						CString nextBrokenLine = breakLeadingWhiteSpace + breakToken.getSuffixString( currentLine );

						currentLine = breakToken.getPrefixString( currentLine );
						brokenLines.push_back( nextBrokenLine );

						breakToken.setEmpty( alignAtIndex ); // go after leading whitespaces
						prevBreakSepRange.setEmpty( -1 );
					}
					else if ( match == LBT_CloseBrace )
						return breakToken.m_end;
					else
						prevBreakSepRange = breakToken; // remember the break separator history

					break;
				}
				case LBT_NoMatch:
					// shouldn't happen usually if the syntax is correct, which is presumably already verified
					return currentLine.GetLength();
			}
		}
	}

	LineBreakTokenMatch CFormatter::findLineBreakToken( TokenRange* pOutToken, const TCHAR* pCodeText, int startPos /*= 0*/ ) const
	{
		ASSERT_PTR( pOutToken );
		ASSERT_PTR( pCodeText );
		ASSERT( (size_t)startPos <= str::GetLength( pCodeText ) );

		const TCHAR* pCursor = pCodeText + startPos;

		while ( *pCursor != _T('\0') )
		{
			int statementEnd;
			const std::tstring* pBreakSeparatorFound = nullptr;

			if ( code::isQuoteChar( *pCursor ) )
			{
				int matchingQuotePos = code::findMatchingQuotePos( pCodeText, int( pCursor - pCodeText ) );

				if ( matchingQuotePos == -1 )
					break; // fatal syntax error -> abort searching
				pCursor = pCodeText + matchingQuotePos + 1; // go past closing quote
			}
			else if
			(
				m_languageEngine.isCommentStatement( statementEnd, pCodeText, int( pCursor - pCodeText ) ) ||
				(
					m_docLanguage == DocLang_Cpp && ( m_languageEngine.isCCastStatement( statementEnd, pCodeText, int( pCursor - pCodeText ) ) ||
													m_languageEngine.isUnicodePortableStringConstant( statementEnd, pCodeText, int( pCursor - pCodeText ) ) )
				)
			)
			{
				pCursor = pCodeText + str::safePos( statementEnd, pCodeText );
			}
			else if ( ( pBreakSeparatorFound = m_options.FindBreakSeparator( pCursor ) ) != nullptr )
			{
				pOutToken->setWithLength( int( pCursor - pCodeText ), pBreakSeparatorFound->length() );
				return LBT_BreakSeparator;
			}
			else if ( code::isOpenBraceChar( *pCursor ) && m_validArgListOpenBraces.Find( *pCursor ) != -1 )
			{
				pOutToken->setWithLength( int( pCursor - pCodeText ), 1 );
				return LBT_OpenBrace;
			}
			else if ( code::isCloseBraceChar( *pCursor ) && m_validArgListOpenBraces.Find( code::getMatchingBrace( *pCursor ) ) != -1 )
			{
				pOutToken->setWithLength( int( pCursor - pCodeText ), 1 );
				return LBT_CloseBrace;
			}
			else
				++pCursor;
		}

		pOutToken->gotoEnd( pCodeText );
		return LBT_NoMatch;
	}

	TEditorColumn CFormatter::computeVisualEditorColumn( const TCHAR* pCodeText, int index ) const
	{
		// search for the start of line that contains 'index' position
		int pos = str::reverseFindStringPos( pCodeText, code::lineEnd, index ).m_end;

		if ( pos == -1 )
			pos = 0; // 'index' belongs to the first line -> start at the beginning

		TEditorColumn visualEditorColumn = 1;

		ASSERT( pos <= index );

		for ( ; pos != index; ++pos )
			if ( pCodeText[ pos ] == _T('\t') )
			{
				int innerTabSize = m_tabSize - ( ( visualEditorColumn - 1 ) % m_tabSize );
				visualEditorColumn += innerTabSize;
			}
			else
				++visualEditorColumn;

		return visualEditorColumn;
	}

	CString CFormatter::makeLineIndentWhiteSpace( int editorColIndex ) const
	{
		return makeLineIndentWhiteSpace( editorColIndex, m_useTabs );
	}

	CString CFormatter::makeLineIndentWhiteSpace( int editorColIndex, bool doUseTabs ) const
	{
		ASSERT( editorColIndex >= 0 );

		if ( doUseTabs )
			return CString( _T('\t'), editorColIndex / m_tabSize ) + CString( _T(' '), editorColIndex % m_tabSize );
		else
			return CString( _T(' '), editorColIndex );
	}

	TokenRange CFormatter::getWhiteSpaceRange( const TCHAR* pCodeText, int pos /*= 0*/, bool includingComments /*= true*/ ) const
	{
		TokenRange whitespacesRange( pos );

		ASSERT( whitespacesRange.InStringBounds( pCodeText ) );

		while ( code::isWhitespaceChar( pCodeText[ whitespacesRange.m_end ] ) )
			++whitespacesRange.m_end;

		if ( includingComments )
		{
			int statementEndPos;

			if ( m_languageEngine.isCommentStatement( statementEndPos, pCodeText, whitespacesRange.m_end ) )
			{
				whitespacesRange.m_end = statementEndPos;
				while ( code::isWhitespaceChar( pCodeText[ whitespacesRange.m_end ] ) )
					++whitespacesRange.m_end;
			}
		}

		return whitespacesRange;
	}

	void CFormatter::cppFilterPrototypeForImplementation( CString& targetString ) const
	{
		static const TCHAR* s_declarativeTokens[] =
		{
			_T(";"),
			_T("virtual"),
			_T("static"),
			_T("friend"),
			_T("explicit"),
			_T("mutable"),
			//_T("override"),
			_T("final"),
			_T("afx_msg"),
			_T("public:"),
			_T("protected:"),
			_T("private:")
		};

		pred::IsAlphaNum isAlphaNum;

		for ( int i = 0; i != COUNT_OF( s_declarativeTokens ); ++i )
		{
			TokenRange foundToken = m_languageEngine.findString( targetString, s_declarativeTokens[ i ] );

			if ( foundToken.IsNonEmpty() )
				if ( !isAlphaNum( str::charAt( targetString, foundToken.m_start ) ) || !isAlphaNum( str::charAt( targetString, foundToken.m_end ) ) )
					foundToken.replaceWithToken( &targetString, _T("") );
		}

		// remove leading whitespaces
		TokenRange leadingWhiteSpaceRange = getWhiteSpaceRange( targetString );

		leadingWhiteSpaceRange.replaceWithToken( &targetString, _T("") );
	}

	CString CFormatter::comment( const TCHAR* pCodeText, bool isEntireLine, CommentState commentState ) const
	{
		ASSERT( commentState != NoComment );

		const CommentTokens& langCommentTokens = CommentTokens::getLanguageSpecific( m_docLanguage );
		const TCHAR* openComment = langCommentTokens.getOpenToken( commentState );

		static const TCHAR* leading4Spaces = _T("    ");
		CString commentedCodeText = pCodeText;
		TokenRange leadingSpaceRange( 0 );

		if ( isEntireLine )
			if ( str::isTokenMatch( commentedCodeText, leading4Spaces ) )
				leadingSpaceRange.m_end += str::Length( openComment );

		leadingSpaceRange.replaceWithToken( &commentedCodeText, openComment );

		if ( commentState == MultiLineComment )
			commentedCodeText += langCommentTokens.m_closeComment;

		return commentedCodeText;
	}

	CString CFormatter::uncomment( const TCHAR* pCodeText, bool isEntireLine ) const
	{
		const CommentTokens& langCommentTokens = CommentTokens::getLanguageSpecific( m_docLanguage );

		TokenRange coreRange( pCodeText );

		if ( langCommentTokens.m_singleLineComment != nullptr &&
			 str::isTokenMatch( pCodeText, langCommentTokens.m_singleLineComment, 0, getLanguageCase( m_docLanguage ) ) )
		{
			coreRange.m_start += str::Length( langCommentTokens.m_singleLineComment );
		}
		else if ( langCommentTokens.m_openComment != nullptr && langCommentTokens.m_closeComment != nullptr )
		{
			if ( str::isTokenMatch( pCodeText, langCommentTokens.m_openComment, 0, getLanguageCase( m_docLanguage ) ) )
				coreRange.m_start += str::Length( langCommentTokens.m_openComment );

			int closeCommentLength = str::Length( langCommentTokens.m_closeComment );

			if ( closeCommentLength <= coreRange.getLength() &&
				 str::isTokenMatch( pCodeText, langCommentTokens.m_closeComment, coreRange.m_end - closeCommentLength, getLanguageCase( m_docLanguage ) ) )
				coreRange.m_end -= closeCommentLength;
		}

		CString uncommentedCodeText = coreRange.getString( pCodeText );

		if ( isEntireLine && coreRange.m_start > 0 )
			if ( uncommentedCodeText.Left( 2 ) == _T("  ") )
				uncommentedCodeText = CString( _T(' '), coreRange.m_start ) + uncommentedCodeText;

		return uncommentedCodeText;
	}

} // namespace code
