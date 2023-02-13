
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "utl/StringParsing.h"
#include "utl/CodeParsing.h"
#include "test/CodeParserTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCodeParserTests::CCodeParserTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCodeParserTests& CCodeParserTests::Instance( void )
{
	static CCodeParserTests s_testCase;
	return s_testCase;
}

void CCodeParserTests::TestCustomLanguage( void )
{
	const code::CLanguage<char> myLang( "'|\"|/*BEGIN|//", "'|\"|END*/|\n" );		// exotic asymetric comments syntax, to ensure reverse iteration is matching properly

	const std::string text =
		"size_t /*BEGIN skip(,'(',= END*/ PushPath( int depth = 5 ) { // SL-comment\n int t[17]; /*BEGIN skip],{,= END*/ } end";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itBegin = text.begin(), itEnd = text.end();
			{
				{	// Find API
					std::string::const_iterator itOpenBrace = myLang.FindNextBrace( itBegin, itEnd );
					ASSERT_HAS_PREFIX( "( int depth", &*itOpenBrace );

					std::string::const_iterator itCloseBrace = myLang.FindMatchingBrace( itOpenBrace, itEnd );
					ASSERT_HAS_PREFIX( ") { // SL-", &*itCloseBrace );

					itOpenBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd );
					ASSERT_HAS_PREFIX( "{ // SL-", &*itOpenBrace );

					itCloseBrace = myLang.FindMatchingBrace( itOpenBrace, itEnd );		// also skip 2nd comment + single-line comment
					ASSERT_HAS_PREFIX( "} end", &*itCloseBrace );
				}

				{	// Skip API with iterator ranges
					Range<std::string::const_iterator> it( myLang.FindNextBrace( itBegin, itEnd ) );
					ASSERT_HAS_PREFIX( "( int depth", &*it.m_start );
					ENSURE( it.IsEmpty() );

					ASSERT( myLang.SkipPastMatchingBrace( &it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( " { // SL-", &*it.m_end );
					ASSERT_EQUAL( "( int depth = 5 )", str::ExtractString( it ) );

					it.SetEmptyRange( myLang.FindNextBrace( it.m_end, itEnd ) );
					ASSERT_HAS_PREFIX( "{ // SL-", &*it.m_start );

					ASSERT( myLang.SkipPastMatchingBrace( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
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
				std::string::const_reverse_iterator itOpenBrace = myLang.FindNextBrace( itBegin, itEnd );
				ASSERT_HAS_PREFIX( "} end", &*itOpenBrace );

				std::string::const_reverse_iterator itCloseBrace = myLang.FindMatchingBrace( itOpenBrace, itEnd );
				ASSERT_HAS_PREFIX( "{ // SL", &*itCloseBrace );

				itOpenBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd );
				ASSERT_HAS_PREFIX( ") { //", &*itOpenBrace );

				itCloseBrace = myLang.FindMatchingBrace( itOpenBrace, itEnd );
				ASSERT_HAS_PREFIX( "( int depth", &*itCloseBrace );

				itCloseBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd );
				ASSERT( itCloseBrace == itEnd );
			}

			{	// Skip API with reverse iterator ranges
				Range<std::string::const_reverse_iterator> it( myLang.FindNextBrace( itBegin, itEnd ) );
				ASSERT_HAS_PREFIX( "} end", &*it.m_start );
				ENSURE( it.IsEmpty() );

				ASSERT( myLang.SkipPastMatchingBrace( &it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( " { // SL-", &*it.m_end );
				ASSERT_EQUAL( "{ // SL-comment\n int t[17]; /*BEGIN skip],{,= END*/ }", str::ExtractString( it ) );

				it.SetEmptyRange( myLang.FindNextBrace( it.m_end, itEnd ) );
				ASSERT_HAS_PREFIX( ") { // SL-", &*it.m_start );

				ASSERT( myLang.SkipPastMatchingBrace( &it.m_end, itEnd ) );		// also skip 2nd comment + single-line comment
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

void CCodeParserTests::TestBraceParity( void )
{
	const code::CLanguage<char>& cppLang = code::GetCppLanguage<char>();

	const std::string text = "std::pair<ObjectT*, cache::TStatusFlags> CLoader< std::pair<PathT, size_t>, ObjectT >::Acquire( const PathT& pathKey )";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			{
				std::string::const_iterator itOpenBrace = cppLang.FindNextBrace( text.begin(), itEnd );
				ASSERT_HAS_PREFIX( "<ObjectT*,", &*itOpenBrace );

				std::string::const_iterator itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );
				ASSERT_HAS_PREFIX( "> CLoader", &*itCloseBrace );
			}

			{	// ranges
				Range<std::string::const_iterator> itRange( cppLang.FindNextBrace( text.begin(), itEnd ) );
				ASSERT_HAS_PREFIX( "<ObjectT*,", &*itRange.m_start );

				ASSERT( cppLang.SkipPastMatchingBrace( &itRange.m_end, itEnd ) );
				ASSERT_EQUAL( "<ObjectT*, cache::TStatusFlags>", str::ExtractString( itRange ) );

				Range<size_t> posRange = str::MakePosRange( itRange, text.begin() );
				ASSERT_EQUAL( Range<size_t>( 9, 40 ), posRange );
			}
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "<ObjectT*,", pOpenBrace );

			ASSERT_HAS_PREFIX( "> CLoader", cppLang.FindMatchingBrace( pOpenBrace, pEnd ) );
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itEnd = text.rend();
		{
			std::string::const_reverse_iterator itOpenBrace = utl::RevIterAtFwdPos( text, text.rfind( ">" ) );
			ASSERT_HAS_PREFIX( ">::Acquire(", &*itOpenBrace );

			std::string::const_reverse_iterator itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );
			ASSERT_HAS_PREFIX( "< std::pair<PathT, size_t>", &*itCloseBrace );
		}

		{	// ranges
			Range<std::string::const_reverse_iterator> itRange( cppLang.FindNextBrace( text.rbegin(), itEnd ) );
			ASSERT_HAS_PREFIX( ")", &*itRange.m_start );

			ASSERT( cppLang.SkipPastMatchingBrace( &itRange.m_end, itEnd ) );
			ASSERT_EQUAL( "( const PathT& pathKey )", str::ExtractString( itRange ) );
			  ASSERT_HAS_PREFIX( "e( const PathT&", &*itRange.m_end );		// points 1 position beyond the matching brace, just as for forward skipping

			Range<size_t> posRange = str::MakeFwdPosRange( itRange, text );
			ASSERT_EQUAL( "( const PathT& pathKey )", str::ExtractString( posRange, text ) );
		}
	}
}

void CCodeParserTests::TestBraceMismatch( void )
{
	const code::CLanguage<char>& cppLang = code::GetCppLanguage<char>();

	const std::string text = "void CLoader< std::pair<PathT, size_t>, (ObjectT[] >::Acquire( const PathT& pathKey )";

	{	// FORWARD iteration - brace mismatch: code syntax error
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			std::string::const_iterator itOpenBrace = cppLang.FindNextBrace( text.begin(), itEnd );
			ASSERT_HAS_PREFIX( "< std::pair", &*itOpenBrace );

			std::string::const_iterator itBraceMismatch = itEnd;
			std::string::const_iterator itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd, &itBraceMismatch );

			// matching brace not found
			ASSERT( itCloseBrace == itEnd );
			ASSERT_HAS_PREFIX( "< std::pair", &*itBraceMismatch );		// first (originating) mismatching brace
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "< std::pair", pOpenBrace );

			const char* pBraceMismatch = pEnd;
			const char* pCloseBrace = cppLang.FindMatchingBrace( pOpenBrace, pEnd, &pBraceMismatch );

			// matching brace not found
			ASSERT( pCloseBrace == pEnd );
			ASSERT_HAS_PREFIX( "< std::pair", pBraceMismatch );		// first (originating) mismatching brace
		}
	}
}

void CCodeParserTests::TestCodeDetails( void )
{
	const code::CLanguage<char>& cppLang = code::GetCppLanguage<char>();

	const std::string text =
		"size_t  \t\r\n /* skip(,'(' */ DisplayFilePaths( std::vector<fs::CPath>& rFilePaths, const fs::TDirPath& dirPath /*= StdDir() )*/, "
		"const TCHAR* pWildSpec = _T(\"*.*\"), fs::TEnumFlags flags = fs::TEnumFlags() ) { int t = 10; // comment\n }";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			{
				std::string::const_iterator itOpenBrace = cppLang.FindNextBrace( text.begin(), itEnd );
				ASSERT_HAS_PREFIX( "( std::vector<", &*itOpenBrace );

				std::string::const_iterator itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );
				ASSERT_HAS_PREFIX( ") { int t = 10;", &*itCloseBrace );

				itOpenBrace = cppLang.FindNextBrace( itCloseBrace + 1, itEnd );
				ASSERT_HAS_PREFIX( "{ int t", &*itOpenBrace );

				itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );		// skips the single-line comment!
				ASSERT_HAS_PREFIX( "}", &*itCloseBrace );
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
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "( std::vector<", pOpenBrace );

			const char* pCloseBrace = cppLang.FindMatchingBrace( pOpenBrace, pEnd );
			ASSERT_HAS_PREFIX( ") { int t = 10;", pCloseBrace );

			pOpenBrace = cppLang.FindNextBrace( pCloseBrace + 1, pEnd );
			ASSERT_HAS_PREFIX( "{ int t", pOpenBrace );

			pCloseBrace = cppLang.FindMatchingBrace( pOpenBrace, pEnd );		// skips the single-line comment
			ASSERT_HAS_PREFIX( "}", pCloseBrace );
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itEnd = text.rend();
		{
			std::string::const_reverse_iterator itOpenBrace = cppLang.FindNextBrace( text.rbegin(), itEnd );
			ASSERT_HAS_PREFIX( "}", &*itOpenBrace );

			std::string::const_reverse_iterator itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );
			ASSERT_HAS_PREFIX( "{ int t = 10;", &*itCloseBrace );

			itOpenBrace = cppLang.FindNextBrace( itCloseBrace + 1, itEnd );
			ASSERT_HAS_PREFIX( ") { int t", &*itOpenBrace );

			itCloseBrace = cppLang.FindMatchingBrace( itOpenBrace, itEnd );		// skips the single-line comment
			ASSERT_HAS_PREFIX( "( std::vector<fs::CPath>&", &*itCloseBrace );
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


void CCodeParserTests::Run( void )
{
	__super::Run();

	TestCustomLanguage();
	TestBraceParity();
	TestBraceMismatch();
	TestCodeDetails();
}


#endif //USE_UT
