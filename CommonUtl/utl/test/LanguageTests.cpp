
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "utl/StringParsing.h"
#include "utl/Language.h"
#include "utl/CodeAlgorithms.h"
#include "test/LanguageTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
				std::string::const_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ) );
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

			itChar = myLang.FindNextCharThat( itChar + 1, itEnd, pred::IsChar( '=' ) );
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
				std::string::const_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ) );
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
			std::string::const_reverse_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ) );
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
	const code::CLanguage<char>& cppLang = code::GetCppLang<char>();

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
	const code::CLanguage<char>& cppLang = code::GetCppLang<char>();

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
		}
	}
}

void CLanguageTests::TestCodeDetails( void )
{
	const code::CLanguage<char>& cppLang = code::GetCppLang<char>();

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

	RUN_TEST( TestUntabify );
}


#endif //USE_UT
