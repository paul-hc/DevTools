
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "utl/StringParsing.h"
#include "utl/Language.h"
#include "utl/CodeAlgorithms.h"
#include "utl/TextClipboard.h"
#include "test/LanguageTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Language.hxx"


CLanguageTests::CLanguageTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CLanguageTests& CLanguageTests::Instance( void )
{
	static CLanguageTests s_testCase;
	return s_testCase;
}

void CLanguageTests::TestBasicParsing( void )
{
	// heterogenous narrow and wide strings equality
	std::string narrow;
	std::wstring wide;

	ASSERT( str::EqualsSeq( narrow.begin(), narrow.end(), wide ) );
	ASSERT( str::EqualsSeqAt( narrow, 0, wide ) );
	ASSERT( str::EqualsSeq( wide.begin(), wide.end(), narrow ) );
	ASSERT( str::EqualsSeqAt( wide, 0, narrow ) );

	narrow = "a"; wide = L"a";
	ASSERT( str::EqualsSeq( narrow.begin(), narrow.end(), wide ) );
	ASSERT( str::EqualsSeqAt( narrow, 0, wide ) );
	ASSERT( str::EqualsSeq( wide.begin(), wide.end(), narrow ) );
	ASSERT( str::EqualsSeqAt( wide, 0, narrow ) );

	narrow = "abc1"; wide = L"abc1";
	ASSERT( str::EqualsSeq( narrow.begin(), narrow.end(), wide ) );
	ASSERT( str::EqualsSeqAt( narrow, 0, wide ) );
	ASSERT( str::EqualsSeq( wide.begin(), wide.end(), narrow ) );
	ASSERT( str::EqualsSeqAt( wide, 0, narrow ) );
}

void CLanguageTests::TestMyLanguage( void )
{
	const code::CLanguage<char> myLang( "'|\"|/*BEGIN|//", "'|\"|END*/|\n" );		// exotic asymetric comments syntax, to ensure reverse iteration is matching properly

	const std::string text =
		"size_t /*BEGIN skip(,'(',= END*/ PushPath( int depth = 5 ) { // SL-comment\n int t[17]; /*BEGIN skip],{,= END*/ } end";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itBegin = text.begin(), itEnd = text.end();
			{
				{	// Find API
					std::string::const_iterator itOpenBracket = myLang.FindNextBracket( itBegin, itEnd );
					ASSERT_HAS_PREFIX( "( int depth", &*itOpenBracket );

					std::string::const_iterator itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
					ASSERT_HAS_PREFIX( ") { // SL-", &*itCloseBracket );

					itOpenBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
					ASSERT_HAS_PREFIX( "{ // SL-", &*itOpenBracket );

					itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );		// also skip 2nd comment + single-line comment
					ASSERT_HAS_PREFIX( "} end", &*itCloseBracket );
				}

				{	// Skip API with iterator ranges
					Range<std::string::const_iterator> it( myLang.FindNextBracket( itBegin, itEnd ) );
					ASSERT_HAS_PREFIX( "( int depth", &*it.m_start );
					ENSURE( it.IsEmpty() );

					ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( " { // SL-", &*it.m_end );
					ASSERT_EQUAL( "( int depth = 5 )", str::ExtractString( it ) );

					it.SetEmptyRange( myLang.FindNextBracket( it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( "{ // SL-", &*it.m_start );

					ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
					ASSERT_HAS_PREFIX( " end", &*it.m_end );
					ASSERT_EQUAL( "{ // SL-comment\n int t[17]; /*BEGIN skip],{,= END*/ }", str::ExtractString( it ) );
				}
			}

			{	// find token character
				std::string::const_iterator itChar = myLang.FindNextChar( itBegin, itEnd, '=' );
				ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

				itChar = myLang.FindNextChar( itChar + 1, itEnd, '=' );
				ASSERT( itChar == itEnd );
			}

			{	// find sequence
				std::string::const_iterator itChar = myLang.FindNextSequence( itBegin, itEnd, "= 5" );
				ASSERT_HAS_PREFIX( "= 5", &*itChar );

				itChar = myLang.FindNextSequence( itChar + 1, itEnd, "int t" );
				ASSERT_HAS_PREFIX( "int t[17]", &*itChar );

				ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "skip(" ) );			// inside comment
				ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "SL-comment" ) );		// inside single-line comment
			}
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itBegin = text.rbegin(), itEnd = text.rend();
		{
			{
				std::string::const_reverse_iterator itOpenBracket = myLang.FindNextBracket( itBegin, itEnd );
				ASSERT_HAS_PREFIX( "} end", &*itOpenBracket );

				std::string::const_reverse_iterator itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( "{ // SL", &*itCloseBracket );

				itOpenBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
				ASSERT_HAS_PREFIX( ") { //", &*itOpenBracket );

				itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( "( int depth", &*itCloseBracket );

				itCloseBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
				ASSERT( itCloseBracket == itEnd );
			}

			{	// Skip API with reverse iterator ranges
				Range<std::string::const_reverse_iterator> it( myLang.FindNextBracket( itBegin, itEnd ) );
				ASSERT_HAS_PREFIX( "} end", &*it.m_start );
				ENSURE( it.IsEmpty() );

				ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( " { // SL-", &*it.m_end );
				ASSERT_EQUAL( "{ // SL-comment\n int t[17]; /*BEGIN skip],{,= END*/ }", str::ExtractString( it ) );

				it.SetEmptyRange( myLang.FindNextBracket( it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( ") { // SL-", &*it.m_start );

				ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
				ASSERT_HAS_PREFIX( "h( int depth", &*it.m_end );
				ASSERT_EQUAL( "( int depth = 5 )", str::ExtractString( it ) );
			}
		}

		{	// find token character
			std::string::const_reverse_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ) );
			ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

			itChar = myLang.FindNextChar( itChar + 1, itEnd, '=' );
			ASSERT( itChar == itEnd );
		}

		{	// find sequence
			std::string::const_reverse_iterator itChar = myLang.FindNextSequence( itBegin, itEnd, "int t" );
			ASSERT_HAS_PREFIX( "int t[17]", &*itChar );

			itChar = myLang.FindNextSequence( itChar + 1, itEnd, "= 5" );
			ASSERT_HAS_PREFIX( "= 5", &*itChar );

			ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "skip(" ) );			// inside comment
			ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "SL-comment" ) );		// inside single-line comment
		}
	}
}

void CLanguageTests::TestMyLanguageSingleLine( void )
{
	const code::CLanguage<char> myLang( "'|\"|/*BEGIN|//", "'|\"|END*/|\n" );		// exotic asymetric comments syntax, to ensure reverse iteration is matching properly

	// single-line code: no ending "\n" for "//" comment
	std::string lineText =
		"size_t /*BEGIN skip(,'(',= END*/ ListDir( char pathSep = '/', const char prefix[] = \"/scratch\" ) { int t[17]; /*BEGIN skip],{,= END*/ } end // SL-comment";

	{	// CEnclosedParser code features, forward iteration by position
		typedef str::CEnclosedParser<char> TParser;

		const TParser& myParser = myLang.GetParser();
		enum MatchPos { SingleQuote, DoubleQuote, Comment, LineComment };		// as defined in myLang

		TParser::TSepMatchPos sepMatchPos;
		TParser::TSpecPair specBounds = myParser.FindItemSpec( &sepMatchPos, lineText );

		ASSERT( Comment == sepMatchPos && specBounds.first != utl::npos );
		ASSERT_EQUAL( "/*BEGIN skip(,'(',= END*/", myParser.MakeSpec( specBounds, lineText ) );
		ASSERT_EQUAL( " skip(,'(',= ", myParser.ExtractItem( sepMatchPos, specBounds, lineText ) );

		specBounds = myParser.FindItemSpec( &sepMatchPos, lineText, specBounds.second );
		ASSERT( SingleQuote == sepMatchPos && specBounds.first != utl::npos );
		ASSERT_EQUAL( "'/'", myParser.MakeSpec( specBounds, lineText ) );
		ASSERT_EQUAL( "/", myParser.ExtractItem( sepMatchPos, specBounds, lineText ) );

		specBounds = myParser.FindItemSpec( &sepMatchPos, lineText, specBounds.second );
		ASSERT( DoubleQuote == sepMatchPos && specBounds.first != utl::npos );
		ASSERT_EQUAL( "\"/scratch\"", myParser.MakeSpec( specBounds, lineText ) );
		ASSERT_EQUAL( "/scratch", myParser.ExtractItem( sepMatchPos, specBounds, lineText ) );

		specBounds = myParser.FindItemSpec( &sepMatchPos, lineText, specBounds.second );
		ASSERT( Comment == sepMatchPos && specBounds.first != utl::npos );
		ASSERT_EQUAL( "/*BEGIN skip],{,= END*/", myParser.MakeSpec( specBounds, lineText ) );
		ASSERT_EQUAL( " skip],{,= ", myParser.ExtractItem( sepMatchPos, specBounds, lineText ) );

		specBounds = myParser.FindItemSpec( &sepMatchPos, lineText, specBounds.second );
		ASSERT( LineComment == sepMatchPos && specBounds.first != utl::npos );
		ASSERT_EQUAL( "// SL-comment", myParser.MakeSpec( specBounds, lineText ) );
		ASSERT_EQUAL( " SL-comment", myParser.ExtractItem( sepMatchPos, specBounds, lineText ) );

		specBounds = myParser.FindItemSpec( &sepMatchPos, lineText, specBounds.second );
		ASSERT( utl::npos == sepMatchPos && utl::npos == specBounds.first );		// no more matches past end
	}


	lineText =
		"size_t /*BEGIN skip(,'(',= END*/ PushPath( int depth = 5 ) { int t[17]; /*BEGIN skip],{,= END*/ } end // SL-comment";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itBegin = lineText.begin(), itEnd = lineText.end();
			{
				{	// Find API
					std::string::const_iterator itOpenBracket = myLang.FindNextBracket( itBegin, itEnd );
					ASSERT_HAS_PREFIX( "( int depth", &*itOpenBracket );

					std::string::const_iterator itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
					ASSERT_HAS_PREFIX( ") { int t[17]", &*itCloseBracket );

					itOpenBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
					ASSERT_HAS_PREFIX( "{ int t[17]", &*itOpenBracket );

					itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );		// also skip 2nd comment + single-line comment
					ASSERT_HAS_PREFIX( "} end", &*itCloseBracket );
				}

				{	// Skip API with iterator ranges
					Range<std::string::const_iterator> it( myLang.FindNextBracket( itBegin, itEnd ) );
					ASSERT_HAS_PREFIX( "( int depth", &*it.m_start );
					ENSURE( it.IsEmpty() );

					ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( " { int t[17]", &*it.m_end );
					ASSERT_EQUAL( "( int depth = 5 )", str::ExtractString( it ) );

					it.SetEmptyRange( myLang.FindNextBracket( it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( "{ int t[17]", &*it.m_start );

					ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
					ASSERT_HAS_PREFIX( " end", &*it.m_end );
					ASSERT_EQUAL( "{ int t[17]; /*BEGIN skip],{,= END*/ }", str::ExtractString( it ) );
				}
			}

			{	// find token character
				std::string::const_iterator itChar = myLang.FindNextChar( itBegin, itEnd, '=' );
				ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

				itChar = myLang.FindNextCharThat( itChar + 1, itEnd, pred::IsChar( '=' ) );
				ASSERT( itChar == itEnd );
			}

			{	// find sequence
				std::string::const_iterator itChar = myLang.FindNextSequence( itBegin, itEnd, "= 5" );
				ASSERT_HAS_PREFIX( "= 5", &*itChar );

				itChar = myLang.FindNextSequence( itChar + 1, itEnd, "int t" );
				ASSERT_HAS_PREFIX( "int t[17]", &*itChar );

				ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "skip(" ) );			// inside comment
				ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "SL-comment" ) );		// inside single-line comment
			}
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itBegin = lineText.rbegin(), itEnd = lineText.rend();
		{
			{
				std::string::const_reverse_iterator itOpenBracket = myLang.FindNextBracket( itBegin, itEnd );
				ASSERT_HAS_PREFIX( "} end", &*itOpenBracket );

				std::string::const_reverse_iterator itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( "{ int t[17]", &*itCloseBracket );

				itOpenBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
				ASSERT_HAS_PREFIX( ") { int t[17]", &*itOpenBracket );

				itCloseBracket = myLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( "( int depth", &*itCloseBracket );

				itCloseBracket = myLang.FindNextBracket( itCloseBracket + 1, itEnd );
				ASSERT( itCloseBracket == itEnd );
			}

			{	// Skip API with reverse iterator ranges
				Range<std::string::const_reverse_iterator> it( myLang.FindNextBracket( itBegin, itEnd ) );
				ASSERT_HAS_PREFIX( "} end", &*it.m_start );
				ENSURE( it.IsEmpty() );

				ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( " { int t[17]", &*it.m_end );
				ASSERT_EQUAL( "{ int t[17]; /*BEGIN skip],{,= END*/ }", str::ExtractString( it ) );

				it.SetEmptyRange( myLang.FindNextBracket( it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( ") { int t[17]", &*it.m_start );

				ASSERT( myLang.SkipPastMatchingBracket( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
				ASSERT_HAS_PREFIX( "h( int depth", &*it.m_end );
				ASSERT_EQUAL( "( int depth = 5 )", str::ExtractString( it ) );
			}
		}

		{	// find token character
			std::string::const_reverse_iterator itChar = myLang.FindNextChar( itBegin, itEnd, '=' );
			ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

			itChar = myLang.FindNextCharThat( itChar + 1, itEnd, pred::IsChar( '=' ) );
			ASSERT( itChar == itEnd );
		}

		{	// find sequence
			std::string::const_reverse_iterator itChar = myLang.FindNextSequence( itBegin, itEnd, "int t" );
			ASSERT_HAS_PREFIX( "int t[17]", &*itChar );

			itChar = myLang.FindNextSequence( itChar + 1, itEnd, "= 5" );
			ASSERT_HAS_PREFIX( "= 5", &*itChar );

			ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "skip(" ) );			// inside comment

			// REVERSE iteration: not possible to detect single-line comments for code without the ending "\n"
			//ASSERT( itEnd == myLang.FindNextSequence( itBegin, itEnd, "SL-comment" ) );	// inside single-line comment
		}
	}
}

void CLanguageTests::TestBracketParity( void )
{
	const code::CLanguage<char>& cppLang = code::GetLangCpp<char>();

	const std::string text = "std::pair<ObjectT*, cache::TStatusFlags> CLoader< std::pair<PathT, size_t>, ObjectT >::Acquire( const PathT& pathKey )";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			{
				std::string::const_iterator itOpenBracket = cppLang.FindNextBracket( text.begin(), itEnd );
				ASSERT_HAS_PREFIX( "<ObjectT*,", &*itOpenBracket );

				std::string::const_iterator itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( "> CLoader", &*itCloseBracket );
			}

			{	// ranges
				Range<std::string::const_iterator> itRange( cppLang.FindNextBracket( text.begin(), itEnd ) );
				ASSERT_HAS_PREFIX( "<ObjectT*,", &*itRange.m_start );

				ASSERT( cppLang.SkipPastMatchingBracket( &itRange.m_end, itEnd ) );
				ASSERT_EQUAL( "<ObjectT*, cache::TStatusFlags>", str::ExtractString( itRange ) );

				Range<size_t> posRange = str::MakePosRange( itRange, text.begin() );
				ASSERT_EQUAL( Range<size_t>( 9, 40 ), posRange );
			}
			{	// find with skipping arg-lists
				std::string::const_iterator itArgs2 = cppLang.FindNextSequence( text.begin(), itEnd, "std::pair<PathT" );
				ASSERT_HAS_PREFIX( "std::pair<PathT", &*itArgs2 );

				ASSERT_HAS_PREFIX( ", size_t", &*cppLang.FindNextChar( itArgs2, itEnd, ',' ) );				// the first ','
				{
					code::CLanguage<char>::CScopedSkipArgLists skipArgLists( &cppLang );
					ASSERT_HAS_PREFIX( ", ObjectT >", &*cppLang.FindNextChar( itArgs2, itEnd, ',' ) );		// the second ',' <= skipped "<...>" arg-list
				}
				ASSERT_HAS_PREFIX( ", size_t", &*cppLang.FindNextChar( itArgs2, itEnd, ',' ) );				// the first ',' again
			}
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBracket = cppLang.FindNextBracket( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "<ObjectT*,", pOpenBracket );

			ASSERT_HAS_PREFIX( "> CLoader", cppLang.FindMatchingBracket( pOpenBracket, pEnd ) );
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itEnd = text.rend();
		{
			std::string::const_reverse_iterator itOpenBracket = utl::RevIterAtFwdPos( text, text.rfind( ">" ) );
			ASSERT_HAS_PREFIX( ">::Acquire(", &*itOpenBracket );

			std::string::const_reverse_iterator itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );
			ASSERT_HAS_PREFIX( "< std::pair<PathT, size_t>", &*itCloseBracket );
		}

		{	// ranges
			Range<std::string::const_reverse_iterator> itRange( cppLang.FindNextBracket( text.rbegin(), itEnd ) );
			ASSERT_HAS_PREFIX( ")", &*itRange.m_start );

			ASSERT( cppLang.SkipPastMatchingBracket( &itRange.m_end, itEnd ) );
			ASSERT_EQUAL( "( const PathT& pathKey )", str::ExtractString( itRange ) );
			  ASSERT_HAS_PREFIX( "e( const PathT&", &*itRange.m_end );		// points 1 position beyond the matching bracket, just as for forward skipping

			Range<size_t> posRange = str::MakeFwdPosRange( itRange, text );
			ASSERT_EQUAL( "( const PathT& pathKey )", str::ExtractString( posRange, text ) );
		}
	}
}

void CLanguageTests::TestBracketMismatch( void )
{
	const code::CLanguage<char>& cppLang = code::GetLangCpp<char>();

	const std::string text = "void CLoader< std::pair<PathT, size_t>, (ObjectT[] >::Acquire( const PathT& pathKey )";

	{	// FORWARD iteration - bracket mismatch: code syntax error
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			std::string::const_iterator itOpenBracket = cppLang.FindNextBracket( text.begin(), itEnd );
			ASSERT_HAS_PREFIX( "< std::pair", &*itOpenBracket );

			std::string::const_iterator itBracketMismatch = itEnd;
			std::string::const_iterator itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd, &itBracketMismatch );

			// matching bracket not found
			ASSERT( itCloseBracket == itEnd );
			ASSERT_HAS_PREFIX( "< std::pair", &*itBracketMismatch );		// first (originating) mismatching bracket

			ASSERT_THROWS( code::TSyntaxError, cppLang.FindMatchingBracket( itOpenBracket, itEnd ) );	// mismatching bracket exception
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBracket = cppLang.FindNextBracket( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "< std::pair", pOpenBracket );

			const char* pBracketMismatch = pEnd;
			const char* pCloseBracket = cppLang.FindMatchingBracket( pOpenBracket, pEnd, &pBracketMismatch );

			// matching bracket not found
			ASSERT( pCloseBracket == pEnd );
			ASSERT_HAS_PREFIX( "< std::pair", pBracketMismatch );		// first (originating) mismatching bracket

			ASSERT_THROWS( code::TSyntaxError, cppLang.FindMatchingBracket( pOpenBracket, pEnd ) );		// mismatching bracket exception
		}
	}
}

void CLanguageTests::TestCodeDetails( void )
{
	const code::CLanguage<char>& cppLang = code::GetLangCpp<char>();

	const std::string text =
		"size_t  \t\r\n /* skip(,'(' */ DisplayFilePaths( std::vector<fs::CPath>& rFilePaths, const fs::TDirPath& dirPath /*= StdDir() )*/, "
		"const TCHAR* pWildSpec = _T(\"*.*\"), fs::TEnumFlags flags = fs::TEnumFlags() ) { int t = 10; // comment\n }";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			{
				std::string::const_iterator itOpenBracket = cppLang.FindNextBracket( text.begin(), itEnd );
				ASSERT_HAS_PREFIX( "( std::vector<", &*itOpenBracket );

				std::string::const_iterator itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );
				ASSERT_HAS_PREFIX( ") { int t = 10;", &*itCloseBracket );

				itOpenBracket = cppLang.FindNextBracket( itCloseBracket + 1, itEnd );
				ASSERT_HAS_PREFIX( "{ int t", &*itOpenBracket );

				itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );		// skips the single-line comment!
				ASSERT_HAS_PREFIX( "}", &*itCloseBracket );
			}
			{
				std::string::const_iterator it = text.begin();
				ASSERT( cppLang.SkipIdentifier( &it, itEnd ) );
				ASSERT_HAS_PREFIX( "  \t", &*it );

				ASSERT( cppLang.SkipWhitespace( &it, itEnd ) );
				ASSERT_HAS_PREFIX( "DisplayFilePaths(", &*it );			// skips the comment
			}
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBracket = cppLang.FindNextBracket( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "( std::vector<", pOpenBracket );

			const char* pCloseBracket = cppLang.FindMatchingBracket( pOpenBracket, pEnd );
			ASSERT_HAS_PREFIX( ") { int t = 10;", pCloseBracket );

			pOpenBracket = cppLang.FindNextBracket( pCloseBracket + 1, pEnd );
			ASSERT_HAS_PREFIX( "{ int t", pOpenBracket );

			pCloseBracket = cppLang.FindMatchingBracket( pOpenBracket, pEnd );		// skips the single-line comment
			ASSERT_HAS_PREFIX( "}", pCloseBracket );
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itEnd = text.rend();
		{
			std::string::const_reverse_iterator itOpenBracket = cppLang.FindNextBracket( text.rbegin(), itEnd );
			ASSERT_HAS_PREFIX( "}", &*itOpenBracket );

			std::string::const_reverse_iterator itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );
			ASSERT_HAS_PREFIX( "{ int t = 10;", &*itCloseBracket );

			itOpenBracket = cppLang.FindNextBracket( itCloseBracket + 1, itEnd );
			ASSERT_HAS_PREFIX( ") { int t", &*itOpenBracket );

			itCloseBracket = cppLang.FindMatchingBracket( itOpenBracket, itEnd );		// skips the single-line comment
			ASSERT_HAS_PREFIX( "( std::vector<fs::CPath>&", &*itCloseBracket );
		}
		{
			std::string::const_reverse_iterator it = text.rbegin() + 1;
			ASSERT_HAS_PREFIX( " }", &*it );

			ASSERT( cppLang.SkipWhitespace( &it, itEnd ) );
			ASSERT_HAS_PREFIX( "; // comment", &*it );			// skips the single-line comment

			++it;
			ASSERT( cppLang.SkipWhile( &it, itEnd, pred::IsDigit() ) );
			ASSERT_HAS_PREFIX( " 10;", &*it );
		}
	}
}

void CLanguageTests::TestC_EscapeSequences( void )
{
	const code::CEscaper& escaper = code::GetEscaperC();

	// NARROW:
	{
		std::string strLiteral = "\\t\\\"L1\\\"\\r\\n\\'±=\\xB1=\\261\\'\\r\\n»=\\xBB=\273\\r\\n\\a\\b\\f\\v\\?\\\\";		// '±'='\xB1'='\261'    '»'='\xBB'='\273'

		{	// decoding by pointer:
			const char* pSrc = strLiteral.c_str();

			ASSERT_EQUAL( '\t', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '"', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( 'L', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '1', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '"', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\r', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\n', escaper.DecodeCharAdvance( &pSrc ) );

			ASSERT_EQUAL( '\'', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '±', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '=', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '±', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '=', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '±', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\'', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\r', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\n', escaper.DecodeCharAdvance( &pSrc ) );

			ASSERT_EQUAL( '»', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '=', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '»', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '=', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '»', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\r', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\n', escaper.DecodeCharAdvance( &pSrc ) );

			ASSERT_EQUAL( '\a', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\b', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\f', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '\v', escaper.DecodeCharAdvance( &pSrc ) );
			ASSERT_EQUAL( '?', escaper.DecodeCharAdvance( &pSrc ) );
			// no advance
			ASSERT_EQUAL( '\\', escaper.DecodeChar( pSrc ) );
			ASSERT_EQUAL( '\\', escaper.DecodeChar( pSrc ) );

			ASSERT_EQUAL( '\\', escaper.DecodeCharAdvance( &pSrc ) );

			ASSERT_EQUAL( '\0', *pSrc );	// EOS
		}

		{	// decoding by iterator:
			std::string::const_iterator it = strLiteral.begin();

			ASSERT_EQUAL( '\t', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( '"', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( 'L', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( '1', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( '"', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( '\r', escaper.DecodeCharAdvance( &it ) );
			ASSERT_EQUAL( '\n', escaper.DecodeCharAdvance( &it ) );

			// no advance
			ASSERT_EQUAL( '\'', escaper.DecodeChar( it ) );
			ASSERT_EQUAL( '\'', escaper.DecodeChar( it ) );
		}

		std::string actualCode = escaper.Decode( strLiteral );
		ASSERT_EQUAL_SWAP( actualCode, "\
\t\"L1\"\r\n\
'±=±=±'\r\n\
»=»=»\r\n\
\a\b\f\v?\\" );

		ASSERT_EQUAL_SWAP( escaper.Encode( actualCode, false ), "\\t\\\"L1\\\"\\r\\n\\'\\xB1=\\xB1=\\xB1\\'\\r\\n\\xBB=\\xBB=\\xBB\\r\\n\\a\\b\\f\\v?\\\\" );
		ASSERT_EQUAL_SWAP( escaper.Encode( actualCode, false, "\t\?" ), "	\\\"L1\\\"\\r\\n\\'\\xB1=\\xB1=\\xB1\\'\\r\\n\\xBB=\\xBB=\\xBB\\r\\n\\a\\b\\f\\v?\\\\" );	// preserve tabs (don't encode them)

		std::string newLiteral = escaper.Encode( actualCode, true );

		// the expected quoted encoded string is hard to read -> uncomment the line below to paste the copied text into an editor for inspection
		//CTextClipboard::CopyText( newLiteral, CTextClipboard::CMessageWnd().GetWnd() );	// use a temporary message-only window for clipboard copy

		ASSERT_EQUAL_SWAP( newLiteral, "\
\"\\t\\\"L1\\\"\\r\\n\\\r\n\
\\'\\xB1=\\xB1=\\xB1\\'\\r\\n\\\r\n\
\\xBB=\\xBB=\\xBB\\r\\n\\\r\n\
\\a\\b\\f\\v?\\\\\"" );
	}

	// WIDE:
	{
		std::wstring strLiteral = L"\\t\\\"L1\\\"\\r\\n\\'±=\\xB1=\\261\\'\\r\\n\\a\\b\\f\\v\\?\\\\";		// '±'='\xB1'='\261'

		std::wstring actualCode = escaper.Decode( strLiteral );
		ASSERT_EQUAL_SWAP( actualCode, L"\
\t\"L1\"\r\n\
'±=±=±'\r\n\
\a\b\f\v?\\" );

		// note: for wide strings, the '±' character is printable, whereas for narrow strings it's not
		ASSERT_EQUAL_SWAP( escaper.Encode( actualCode, false ), L"\\t\\\"L1\\\"\\r\\n\\'±=±=±\\'\\r\\n\\a\\b\\f\\v?\\\\" );
		ASSERT_EQUAL_SWAP( escaper.Encode( actualCode, false, "\t\?" ), L"	\\\"L1\\\"\\r\\n\\'±=±=±\\'\\r\\n\\a\\b\\f\\v?\\\\" );	// preserve tabs (don't encode them)

		ASSERT_EQUAL_SWAP( escaper.Encode( actualCode, true ), L"\
\"\\t\\\"L1\\\"\\r\\n\\\r\n\
\\'±=±=±\\'\\r\\n\\\r\n\
\\a\\b\\f\\v?\\\\\"" );
	}
}

void CLanguageTests::TestCpp_ParseNumericLiteral( void )
{
	const char* pLiteral;

	// CRT API tests
	{
		char* pEnd = NULL;

		pLiteral = "\t end";		// invalid numeric literal
		pEnd = const_cast<char*>( pLiteral );
		ASSERT_EQUAL( 0, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL_STR( pLiteral, pEnd );
		ASSERT_EQUAL( .0, strtod( pEnd, &pEnd ) );
		ASSERT_EQUAL_STR( pLiteral, pEnd );

		pLiteral = "\t 1234 -4321 0xABCD 0Xcdef 0765 1111 1010 end";		// valid integer literals: decimal, hex, octal, binary
		pEnd = const_cast<char*>( pLiteral );
		ASSERT_EQUAL( 1234, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL( -4321, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL( 0xABCD, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL( 0xCDEF, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL( 0765, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_EQUAL( 0xF, _strtoi64( pEnd, &pEnd, 2 ) );
		ASSERT_EQUAL( 0xA, _strtoi64( pEnd, &pEnd, 2 ) );
		ASSERT_EQUAL_STR( " end", pEnd );

		pLiteral = "\t 1234.5678 111.222e-3 end";
		pEnd = const_cast<char*>( pLiteral );
		ASSERT_EQUAL( 1234, _strtoi64( pEnd, &pEnd, 0 ) );
		ASSERT_HAS_PREFIX( ".5678", pEnd );

		pEnd = const_cast<char*>( pLiteral );
		ASSERT_EQUAL( 1234.5678, strtod( pEnd, &pEnd ) );
		ASSERT_EQUAL( 111.222e-3, strtod( pEnd, &pEnd ) );
		ASSERT_EQUAL_STR( " end", pEnd );
	}

	// cpp utils tests
	{
		size_t numLength;

		pLiteral = "";				// invalid numeric literal
		ASSERT( !cpp::IsValidNumericLiteral( pLiteral, &numLength ) );
		ASSERT_EQUAL( 0, numLength );

		pLiteral = "\t end";		// invalid numeric literal
		ASSERT( !cpp::IsValidNumericLiteral( pLiteral, &numLength ) );
		ASSERT_EQUAL( 0, numLength );

		pLiteral = "\t 0 1234 -4321 0xABCD 0Xcdef 0765 0b1111 0B1010 1234.5678 -111.222e-3 end";	// valid number literals: decimal, hex, octal, binary + double

		// parse "0" decimal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( 0, cpp::DecimalLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;
		// parse "1234" decimal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( 1234, cpp::DecimalLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;
		// parse "-4321" negative decimal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( -4321, cpp::DecimalLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;

		// parse "0xABCD" hexa-decimal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( 0xABCD, cpp::HexLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;
		// parse "0Xcdef" hexa-decimal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( 0Xcdef, cpp::HexLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;

		// parse "0765" octal literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( std::make_pair( 0765, cpp::OctalLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
		pLiteral += numLength;

		// parse "0b1111" binary literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral, &numLength ) );
	#ifdef IS_CPP_14
		ASSERT_EQUAL( std::make_pair( 0b1111, cpp::BinaryLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
	#endif
		pLiteral += numLength;
		// parse "0B1010" binary literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral, &numLength ) );
	#ifdef IS_CPP_14
		ASSERT_EQUAL( std::make_pair( 0B1010, cpp::BinaryLiteral ), cpp::ParseIntegerLiteral<long long>( pLiteral, &numLength ) );
	#endif
		pLiteral += numLength;

		// parse "1234.5678" double literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( cpp::FloatingPointLiteral, cpp::ParseIntegerLiteral<long long>( pLiteral ).second );		// ignore testing irelevant value (which is 1234)
		ASSERT_EQUAL( std::make_pair( 1234.5678, cpp::FloatingPointLiteral ), cpp::ParseDoubleLiteral( pLiteral, &numLength ) );
		pLiteral += numLength;
		// parse "-111.222e-3" double literal
		ASSERT( cpp::IsValidNumericLiteral( pLiteral ) );
		ASSERT_EQUAL( cpp::FloatingPointLiteral, cpp::ParseIntegerLiteral<long long>( pLiteral ).second );		// ignore testing irelevant value (which is 1234)
		ASSERT_EQUAL( std::make_pair( -111.222e-3, cpp::FloatingPointLiteral ), cpp::ParseDoubleLiteral( pLiteral, &numLength ) );
		pLiteral += numLength;

		ASSERT( !cpp::IsValidNumericLiteral( pLiteral, &numLength ) );		// parse " end" invalid literal
		ASSERT_EQUAL( 0, numLength );
		ASSERT_EQUAL_STR( " end", pLiteral );
	}
}

void CLanguageTests::TestSpaceCode( void )
{
	std::string text;

	code::SpaceCode( &text, code::RetainSpace );
	ASSERT_EQUAL( "", text );

	code::SpaceCode( &text, code::AddSpace );
	ASSERT_EQUAL( "  ", text );

	code::SpaceCode( &text, code::TrimSpace );
	ASSERT_EQUAL( "", text );


	text = "\t void   ";
	code::SpaceCode( &text, code::RetainSpace );
	ASSERT_EQUAL( "\t void   ", text );

	code::SpaceCode( &text, code::AddSpace );
	ASSERT_EQUAL( " void ", text );

	code::SpaceCode( &text, code::TrimSpace );
	ASSERT_EQUAL( "void", text );
}

void CLanguageTests::TestEncloseCode( void )
{
	std::string text;

	code::EncloseCode( &text, "[[", "]]" );
	ASSERT_EQUAL( "[[]]", text );

	text = "index";
	code::EncloseCode( &text, "[[", "]]" );
	ASSERT_EQUAL( "[[index]]", text );
	text = "index";
	code::EncloseCode( &text, "[[", "]]", code::AddSpace );
	ASSERT_EQUAL( "[[ index ]]", text );
	text = "index";
	code::EncloseCode( &text, "[[", "]]", code::TrimSpace );
	ASSERT_EQUAL( "[[index]]", text );

	text = "\t index   ";
	code::EncloseCode( &text, "[[", "]]", code::RetainSpace );
	ASSERT_EQUAL( "[[\t index   ]]", text );
	text = "\t index   ";
	code::EncloseCode( &text, "[[", "]]", code::AddSpace );
	ASSERT_EQUAL( "[[ index ]]", text );
	text = "\t index   ";
	code::EncloseCode( &text, "[[", "]]", code::TrimSpace );
	ASSERT_EQUAL( "[[index]]", text );
}

void CLanguageTests::TestUntabify( void )
{
	const std::string textTabs = "\
class CFlag\n\
{\n\
public:\n\
\tCFlag( void );\t// constructor\n\
\tvirtual ~CFlag();\n\
private:\n\
\tint m_flag;\t\t// 1 bit\n\
};\n\
";

	ASSERT_EQUAL_SWAP( code::Untabify( textTabs ),
					   "\
class CFlag\n\
{\n\
public:\n\
    CFlag( void );  // constructor\n\
    virtual ~CFlag();\n\
private:\n\
    int m_flag;     // 1 bit\n\
};\n\
"
	);
}


void CLanguageTests::Run( void )
{
	RUN_TEST( TestBasicParsing );
	RUN_TEST( TestMyLanguage );
	RUN_TEST( TestMyLanguageSingleLine );
	RUN_TEST( TestBracketParity );
	RUN_TEST( TestBracketMismatch );
	RUN_TEST( TestCodeDetails );

	RUN_TEST( TestC_EscapeSequences );
	RUN_TEST( TestCpp_ParseNumericLiteral );
	RUN_TEST( TestSpaceCode );
	RUN_TEST( TestEncloseCode );
	RUN_TEST( TestUntabify );
}


#endif //USE_UT
