
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
				std::string::const_iterator itOpenBrace = myLang.FindNextBrace( itBegin, itEnd );
				ASSERT_HAS_PREFIX( "( int depth", &*itOpenBrace );

				std::string::const_iterator itCloseBrace = myLang.SkipBrace( itOpenBrace, itEnd );
				ASSERT_HAS_PREFIX( ") { //", &*itCloseBrace );

				itOpenBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd );
				ASSERT_HAS_PREFIX( "{ //", &*itOpenBrace );

				itCloseBrace = myLang.SkipBrace( itOpenBrace, itEnd );		// also skip 2nd comment + single-line comment
				ASSERT_HAS_PREFIX( "} end", &*itCloseBrace );
			}

			{	// find token character
				std::string::const_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ) );
				ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

				itChar = myLang.FindNextCharThat( itChar + 1, itEnd, pred::IsChar( '=' ) );
				ASSERT( itChar == itEnd );
			}
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itBegin = text.rbegin(), itEnd = text.rend();
		{
			{
				std::string::const_reverse_iterator itOpenBrace = myLang.FindNextBrace( itBegin, itEnd, str::ReverseIter );
				ASSERT_HAS_PREFIX( "} end", &*itOpenBrace );

				std::string::const_reverse_iterator itCloseBrace = myLang.SkipBrace( itOpenBrace, itEnd, str::ReverseIter );
				ASSERT_HAS_PREFIX( "{ //", &*itCloseBrace );

				itOpenBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd, str::ReverseIter );
				ASSERT_HAS_PREFIX( ") { //", &*itOpenBrace );

				itCloseBrace = myLang.SkipBrace( itOpenBrace, itEnd, str::ReverseIter );
				ASSERT_HAS_PREFIX( "( int depth", &*itCloseBrace );

				itCloseBrace = myLang.FindNextBrace( itCloseBrace + 1, itEnd, str::ReverseIter );
				ASSERT( itCloseBrace == itEnd );
			}
		}

		{	// find token character
			std::string::const_reverse_iterator itChar = myLang.FindNextCharThat( itBegin, itEnd, pred::IsChar( '=' ), str::ReverseIter );
			ASSERT_HAS_PREFIX( "= 5 ) {", &*itChar );

			itChar = myLang.FindNextCharThat( itChar + 1, itEnd, pred::IsChar( '=' ), str::ReverseIter );
			ASSERT( itChar == itEnd );
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
			std::string::const_iterator itOpenBrace = cppLang.FindNextBrace( text.begin(), itEnd );
			ASSERT_HAS_PREFIX( "<ObjectT*,", &*itOpenBrace );

			std::string::const_iterator itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd );
			ASSERT_HAS_PREFIX( "> CLoader", &*itCloseBrace );
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "<ObjectT*,", pOpenBrace );

			ASSERT_HAS_PREFIX( "> CLoader", cppLang.SkipBrace( pOpenBrace, pEnd ) );
		}
	}

	{	// REVERSE iteration
		std::string::const_reverse_iterator itEnd = text.rend();
		std::string::const_reverse_iterator itOpenBrace = utl::RevIterAtFwdPos( text, text.rfind( ">" ) );
		ASSERT_HAS_PREFIX( ">::Acquire(", &*itOpenBrace );

		std::string::const_reverse_iterator itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd, str::ReverseIter );
		ASSERT_HAS_PREFIX( "< std::pair<PathT, size_t>", &*itCloseBrace );
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
			std::string::const_iterator itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd, str::ForwardIter, &itBraceMismatch );

			// matching brace not found
			ASSERT( itCloseBrace == itEnd );
			ASSERT_HAS_PREFIX( "< std::pair", &*itBraceMismatch );		// first (originating) mismatching brace
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "< std::pair", pOpenBrace );

			const char* pBraceMismatch = pEnd;
			const char* pCloseBrace = cppLang.SkipBrace( pOpenBrace, pEnd, str::ForwardIter, &pBraceMismatch );

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
		"size_t /* skip(,'(' */ DisplayFilePaths( std::vector<fs::CPath>& rFilePaths, const fs::TDirPath& dirPath /*= StdDir() )*/, "
		"const TCHAR* pWildSpec = _T(\"*.*\"), fs::TEnumFlags flags = fs::TEnumFlags() ) { int t = 10; // comment\n }";

	{	// FORWARD iteration
		{	// by iterator
			std::string::const_iterator itEnd = text.end();
			std::string::const_iterator itOpenBrace = cppLang.FindNextBrace( text.begin(), itEnd );
			ASSERT_HAS_PREFIX( "( std::vector<", &*itOpenBrace );

			std::string::const_iterator itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd );
			ASSERT_HAS_PREFIX( ") { int t = 10;", &*itCloseBrace );

			itOpenBrace = cppLang.FindNextBrace( itCloseBrace + 1, itEnd );
			ASSERT_HAS_PREFIX( "{ int t", &*itOpenBrace );

			itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd );		// also skip the single-line comment!
			ASSERT_HAS_PREFIX( "}", &*itCloseBrace );
		}
		{	// by pointer
			const char* pEnd = str::s_end( text );
			const char* pOpenBrace = cppLang.FindNextBrace( str::s_begin( text ), pEnd );
			ASSERT_HAS_PREFIX( "( std::vector<", pOpenBrace );

			const char* pCloseBrace = cppLang.SkipBrace( pOpenBrace, pEnd );
			ASSERT_HAS_PREFIX( ") { int t = 10;", pCloseBrace );

			pOpenBrace = cppLang.FindNextBrace( pCloseBrace + 1, pEnd );
			ASSERT_HAS_PREFIX( "{ int t", pOpenBrace );

			pCloseBrace = cppLang.SkipBrace( pOpenBrace, pEnd );		// also skip the single-line comment!
			ASSERT_HAS_PREFIX( "}", pCloseBrace );
		}
	}

	{	// REVERSE iteration
		{
			std::string::const_reverse_iterator itEnd = text.rend();
			std::string::const_reverse_iterator itOpenBrace = cppLang.FindNextBrace( text.rbegin(), itEnd, str::ReverseIter );
			ASSERT_HAS_PREFIX( "}", &*itOpenBrace );

			std::string::const_reverse_iterator itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd, str::ReverseIter );
			ASSERT_HAS_PREFIX( "{ int t = 10;", &*itCloseBrace );

			itOpenBrace = cppLang.FindNextBrace( itCloseBrace + 1, itEnd, str::ReverseIter );
			ASSERT_HAS_PREFIX( ") { int t", &*itOpenBrace );

			itCloseBrace = cppLang.SkipBrace( itOpenBrace, itEnd, str::ReverseIter );		// also skip the single-line comment!
			ASSERT_HAS_PREFIX( "( std::vector<fs::CPath>&", &*itCloseBrace );
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
