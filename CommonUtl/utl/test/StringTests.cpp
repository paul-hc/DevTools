
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/StringTests.h"
#include "Algorithms.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "StringUtilities.h"
#include "StringParsing.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CStringTests::CStringTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStringTests& CStringTests::Instance( void )
{
	static CStringTests s_testCase;
	return s_testCase;
}

void CStringTests::TestCharTraits( void )
{
	static const char* s_nullA = nullptr;
	static const wchar_t* s_nullW = nullptr;

	{	// NARROW
		ASSERT_EQUAL( 0, str::CharTraits::GetLength( s_nullA ) );
		ASSERT_EQUAL( 0, str::CharTraits::GetLength( "" ) );
		ASSERT_EQUAL( 1, str::CharTraits::GetLength( "a" ) );
		ASSERT_EQUAL( 5, str::CharTraits::GetLength( "12345" ) );

		ASSERT_EQUAL( 'A', str::CharTraits::ToUpper( 'a' ) );
		ASSERT_EQUAL( 'A', str::CharTraits::ToUpper( 'A' ) );
		ASSERT_EQUAL( '3', str::CharTraits::ToUpper( '3' ) );

		ASSERT( str::CharTraits::IsDigit( '0' ) );
		ASSERT( !str::CharTraits::IsDigit( '-' ) );
		ASSERT( !str::CharTraits::IsDigit( '.' ) );
		ASSERT( !str::CharTraits::IsDigit( 'a' ) );

		// case-sensitive
		ASSERT_EQUAL( pred::Less, str::CharTraits::Compare( 'a', 'b' ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::Compare( 'a', 'a' ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::Compare( 'b', 'a' ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::Compare( "aa", "bb" ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::Compare( "aa", "aa" ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::Compare( "bb", "aa" ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareN( "ax", "bx", 1 ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareN( "ax", "ay", 1 ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareN( "bx", "ax", 1 ) );
		ASSERT( pred::Equal != str::CharTraits::CompareN( "ax", "ay", 2 ) );

		// case-insensitive
		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareI( 'a', 'B' ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareI( 'a', 'A' ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareI( 'B', 'a' ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareI( "AA", "bb" ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareI( "aa", "AA" ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareI( "bb", "AA" ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareIN( "ax", "BX", 1 ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareIN( "ax", "AY", 1 ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareIN( "BX", "ax", 1 ) );
		ASSERT( pred::Equal != str::CharTraits::CompareIN( "ax", "aY", 2 ) );
	}

	{	// WIDE
		ASSERT_EQUAL( 0, str::CharTraits::GetLength( s_nullW ) );
		ASSERT_EQUAL( 0, str::CharTraits::GetLength( L"" ) );
		ASSERT_EQUAL( 1, str::CharTraits::GetLength( L"a" ) );
		ASSERT_EQUAL( 5, str::CharTraits::GetLength( L"12345" ) );

		ASSERT_EQUAL( L'A', str::CharTraits::ToUpper( L'a' ) );
		ASSERT_EQUAL( L'A', str::CharTraits::ToUpper( L'A' ) );
		ASSERT_EQUAL( L'3', str::CharTraits::ToUpper( L'3' ) );

		ASSERT( str::CharTraits::IsDigit( L'0' ) );
		ASSERT( !str::CharTraits::IsDigit( L'-' ) );
		ASSERT( !str::CharTraits::IsDigit( L'.' ) );
		ASSERT( !str::CharTraits::IsDigit( L'a' ) );

		// case-sensitive
		ASSERT_EQUAL( pred::Less, str::CharTraits::Compare( L'a', L'b' ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::Compare( L'a', L'a' ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::Compare( L'b', L'a' ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::Compare( L"aa", L"bb" ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::Compare( L"aa", L"aa" ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::Compare( L"bb", L"aa" ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareN( L"ax", L"bx", 1 ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareN( L"ax", L"ay", 1 ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareN( L"bx", L"ax", 1 ) );
		ASSERT( pred::Equal != str::CharTraits::CompareN( L"ax", L"ay", 2 ) );

		// case-insensitive
		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareI( L'a', L'B' ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareI( L'a', L'A' ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareI( L'B', L'a' ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareI( L"AA", L"bb" ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareI( L"aa", L"AA" ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareI( L"bb", L"AA" ) );

		ASSERT_EQUAL( pred::Less, str::CharTraits::CompareIN( L"ax", L"BX", 1 ) );
		ASSERT_EQUAL( pred::Equal, str::CharTraits::CompareIN( L"ax", L"AY", 1 ) );
		ASSERT_EQUAL( pred::Greater, str::CharTraits::CompareIN( L"BX", L"ax", 1 ) );
		ASSERT( pred::Equal != str::CharTraits::CompareIN( L"ax", L"aY", 2 ) );
	}

	ASSERT( str::IsEmpty( s_nullA ) );
	ASSERT( str::IsEmpty( s_nullW ) );

	ASSERT( str::IsEmpty( "" ) );
	ASSERT( str::IsEmpty( L"" ) );

	ASSERT( !str::IsEmpty( "a" ) );
	ASSERT( !str::IsEmpty( L"a" ) );

	ASSERT_EQUAL( 0, str::GetLength( s_nullA ) );
	ASSERT_EQUAL( 0, str::GetLength( s_nullW ) );

	ASSERT_EQUAL( 0, str::GetLength( "" ) );
	ASSERT_EQUAL( 0, str::GetLength( L"" ) );
}

void CStringTests::TestIsCharType( void )
{
	ASSERT( utl::All( std::string( " \t\r\n" ), pred::IsSpace() ) );
	ASSERT( utl::All( std::string( "\a\b\r\n\v\f" ), pred::IsControl() ) );
	ASSERT( utl::All( std::string( ".,;" ), pred::IsPunct() ) );

	ASSERT( utl::All( std::string( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" ), pred::IsAlpha() ) );
	ASSERT( utl::All( std::string( "01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" ), pred::IsAlphaNum() ) );
	ASSERT( utl::All( std::string( "01234567890" ), pred::IsDigit() ) );
	ASSERT( utl::All( std::string( "01234567890abcdefABCDEF" ), pred::IsHexDigit() ) );

	ASSERT( utl::All( std::string( "_01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" ), pred::IsIdentifier() ) );
	ASSERT( utl::All( std::string( "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" ), pred::IsIdentifierLead() ) );

	ASSERT( utl::All( std::string( "abcdefghijklmnopqrstuvwxyz" ), pred::IsLower() ) );
	ASSERT( utl::All( std::string( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ), pred::IsUpper() ) );
}

void CStringTests::TestValueToString( void )
{
	{	// to NARROW
		ASSERT_EQUAL( "", str::ValueToString<std::string>( "" ) );
		ASSERT_EQUAL( "", str::ValueToString<std::string>( _T("") ) );

		ASSERT_EQUAL( "x", str::ValueToString<std::string>( 'x' ) );
		ASSERT_EQUAL( "x", str::ValueToString<std::string>( _T('x') ) );

		ASSERT_EQUAL( "abc", str::ValueToString<std::string>( "abc" ) );
		ASSERT_EQUAL( "abc", str::ValueToString<std::string>( _T("abc") ) );

		ASSERT_EQUAL( "name.ext", str::ValueToString<std::string>( fs::CPath( _T("name.ext") ) ) );
		ASSERT_EQUAL( "37", str::ValueToString<std::string>( 37 ) );
	}

	{	// to WIDE
		ASSERT_EQUAL( L"", str::ValueToString<std::wstring>( "" ) );
		ASSERT_EQUAL( L"", str::ValueToString<std::wstring>( _T("") ) );

		ASSERT_EQUAL( L"x", str::ValueToString<std::wstring>( 'x' ) );
		ASSERT_EQUAL( L"x", str::ValueToString<std::wstring>( _T('x') ) );

		ASSERT_EQUAL( L"abc", str::ValueToString<std::wstring>( "abc" ) );
		ASSERT_EQUAL( L"abc", str::ValueToString<std::wstring>( _T("abc") ) );

		ASSERT_EQUAL( L"name.ext", str::ValueToString<std::wstring>( fs::CPath( _T("name.ext") ) ) );

		ASSERT_EQUAL( L"37", str::ValueToString<std::wstring>( fs::CPath( _T("37") ) ) );
	}
}

void CStringTests::TestTrim( void )
{
	{
		static const char srcText[] = "\t  ab c \t\t";

		std::string text;

		str::Trim( text );
		ASSERT_EQUAL( "", text );

		str::TrimLeft( text = srcText );
		ASSERT_EQUAL( "ab c \t\t", text );

		str::TrimRight( text = srcText );
		ASSERT_EQUAL( "\t  ab c", text );

		str::Trim( text = srcText );
		ASSERT_EQUAL( "ab c", text );

		str::Trim( text = srcText, srcText );		// entire text as whitespace
		ASSERT_EQUAL( "", text );

		str::Trim( text = "C:/Users/Paul/AppData/Local/Temp/SliderTempClones" );
		ASSERT_EQUAL( "C:/Users/Paul/AppData/Local/Temp/SliderTempClones", text );
	}

	{
		static const TCHAR srcText[] = _T("\t  ab c \t\t");

		std::tstring text;

		str::Trim( text );
		ASSERT_EQUAL( _T(""), text );

		str::TrimLeft( text = srcText );
		ASSERT_EQUAL( _T("ab c \t\t"), text );

		str::TrimRight( text = srcText );
		ASSERT_EQUAL( _T("\t  ab c"), text );

		str::Trim( text = srcText );
		ASSERT_EQUAL( _T("ab c"), text );

		str::Trim( text = srcText, srcText );		// entire text as whitespace
		ASSERT_EQUAL( _T(""), text );

		str::Trim( text = _T("C:/Users/Paul/AppData/Local/Temp/SliderTempClones") );
		ASSERT_EQUAL( _T("C:/Users/Paul/AppData/Local/Temp/SliderTempClones"), text );
	}
}

void CStringTests::TestEnquote( void )
{
	// NARROW:
	{
		// base enquoting
		ASSERT_EQUAL( "{ abc }", str::Enquote( "abc", "{ ", " }" ) );
		ASSERT_EQUAL( "{  }", str::Enquote( "", "{ ", " }" ) );
		ASSERT_EQUAL( "", str::Enquote( "", "{ ", " }", true ) );

		// empty source
		ASSERT_EQUAL( "\"\"", str::Enquote( "" ) );
		ASSERT_EQUAL( "", str::Enquote( "", '"', true ) );

		// text source
		ASSERT_EQUAL( "\"abc\"", str::Enquote( "abc" ) );
		ASSERT_EQUAL( "\"abc\"", str::Enquote( "abc", '"' ) );

		// string source
		static const std::string s_abc( "abc" );
		ASSERT_EQUAL( "\"abc\"", str::EnquoteStr( s_abc ) );
		ASSERT_EQUAL( "'abc'", str::EnquoteStr( s_abc, '\'' ) );
	}

	// WIDE:
	{
		// empty source
		ASSERT_EQUAL( L"\"\"", str::Enquote( L"" ) );
		ASSERT_EQUAL( L"", str::Enquote( L"", L'"', true ) );

		// text source
		ASSERT_EQUAL( L"\"abc\"", str::Enquote( L"abc" ) );
		ASSERT_EQUAL( L"\"abc\"", str::Enquote( L"abc", L'"' ) );

		// string source
		static const std::wstring s_abc( L"abc" );
		ASSERT_EQUAL( L"\"abc\"", str::EnquoteStr( s_abc ) );
		ASSERT_EQUAL( L"'abc'", str::EnquoteStr( s_abc, L'\'' ) );

		// path source
		static const fs::CPath s_path( L"abc" );
		ASSERT_EQUAL( L"\"abc\"", str::EnquoteStr( s_path ) );
		ASSERT_EQUAL( L"'abc'", str::EnquoteStr( s_path, L'\'' ) );
	}

	// SINGLE QUOTES:
	{
		// empty source
		ASSERT_EQUAL( "''", str::sq::Enquote( "" ) );
		ASSERT_EQUAL( L"''", str::sq::Enquote( L"" ) );
		ASSERT_EQUAL( "", str::sq::Enquote( "", true ) );
		ASSERT_EQUAL( L"", str::sq::Enquote( L"", true ) );

		// text source
		ASSERT_EQUAL( "'abc'", str::sq::Enquote( "abc" ) );
		ASSERT_EQUAL( L"'abc'", str::sq::Enquote( L"abc" ) );

		// string source
		ASSERT_EQUAL( "'abc'", str::sq::EnquoteStr( std::string( "abc" ) ) );
		ASSERT_EQUAL( L"'abc'", str::sq::EnquoteStr( std::wstring( L"abc" ) ) );

		// path source
		ASSERT_EQUAL( L"''", str::sq::EnquoteStr( fs::CPath() ) );
		ASSERT_EQUAL( L"", str::sq::EnquoteStr( fs::CPath(), true ) );
		ASSERT_EQUAL( L"'abc'", str::sq::EnquoteStr( fs::CPath( _T("abc") ) ) );
		ASSERT_EQUAL( L"'abc'", str::sq::EnquoteStr( fs::CPath( _T("abc") ), true ) );
	}
}

void CStringTests::TestStringSplit( void )
{
	{	// WIDE strings
		std::vector<std::tstring> items;
		str::Split( items, _T(""), _T(",;") );
		ASSERT_EQUAL( 0, items.size() );

		str::Split( items, _T("apple"), _T(",;") );
		ASSERT_EQUAL( 1, items.size() );
		ASSERT_EQUAL( _T("apple"), items[ 0 ] );

		str::Split( items, _T("apple,;grape,;plum"), _T(",;") );
		ASSERT_EQUAL( 3, items.size() );
		ASSERT_EQUAL( _T("apple"), items[ 0 ] );
		ASSERT_EQUAL( _T("grape"), items[ 1 ] );
		ASSERT_EQUAL( _T("plum"), items[ 2 ] );

		str::Split( items, _T(""), _T(",;") );
		ASSERT_EQUAL( 0, items.size() );

		str::Split( items, _T("apple"), _T(",;") );
		ASSERT_EQUAL( 1, items.size() );
		ASSERT_EQUAL( _T("apple"), items[ 0 ] );

		str::Split( items, _T("apple,;grape,;plum"), _T(",;") );
		ASSERT_EQUAL( 3, items.size() );
		ASSERT_EQUAL( _T("apple"), items[ 0 ] );
		ASSERT_EQUAL( _T("grape"), items[ 1 ] );
		ASSERT_EQUAL( _T("plum"), items[ 2 ] );

		str::Split( items, _T(",;,;"), _T(",;") );
		ASSERT_EQUAL( 3, items.size() );
		ASSERT_EQUAL( _T(""), items[ 0 ] );
		ASSERT_EQUAL( _T(""), items[ 1 ] );
		ASSERT_EQUAL( _T(""), items[ 2 ] );
	}

	{	// ANSI strings
		std::vector<std::string> items;
		str::Split( items, "", ",;" );
		ASSERT_EQUAL( 0, items.size() );

		str::Split( items, "apple", ",;" );
		ASSERT_EQUAL( 1, items.size() );
		ASSERT_EQUAL( "apple", items[ 0 ] );

		str::Split( items, "apple,;grape,;plum", ",;" );
		ASSERT_EQUAL( 3, items.size() );
		ASSERT_EQUAL( "apple", items[ 0 ] );
		ASSERT_EQUAL( "grape", items[ 1 ] );
		ASSERT_EQUAL( "plum", items[ 2 ] );

		str::Split( items, ",;,;", ",;" );
		ASSERT_EQUAL( 3, items.size() );
		ASSERT_EQUAL( "", items[ 0 ] );
		ASSERT_EQUAL( "", items[ 1 ] );
		ASSERT_EQUAL( "", items[ 2 ] );
	}
}

void CStringTests::TestStringTokenize( void )
{
	static const TCHAR seps[] = _T(";,\n \t");
	std::vector<std::tstring> tokens;
	ASSERT_EQUAL( 0, str::Tokenize( tokens, _T(""), seps ) );
	ASSERT_EQUAL( 6, str::Tokenize( tokens, _T("\n\t,apple,grape;plum pear\tkiwi\nbanana \n\t"), seps ) );
	ASSERT_EQUAL( _T("apple|grape|plum|pear|kiwi|banana"), str::Join( tokens, _T("|") ) );
}

void CStringTests::TestStringPrefixSuffix( void )
{
	{	// ANSI
		ASSERT( str::HasPrefix( "abc_Item_xyz", "" ) );				// match empty prefix
		ASSERT( !str::HasPrefix( "", "abc_Item_xyz" ) );			// but not the other way around

		ASSERT( str::HasPrefix( "abc_Item_xyz", "abc" ) );
		ASSERT( !str::HasPrefix( "abc", "abc_Item_xyz" ) );

		ASSERT( !str::HasPrefix( "abc_Item_xyz", "ABC" ) );
		ASSERT( str::HasPrefixI( "abc_Item_xyz", "ABC" ) );

		ASSERT( !str::HasPrefix( "abc_Item_xyz", "abx" ) );
		ASSERT( str::HasPrefix( "abc_Item_xyz", "abx", 2 ) );		// N
		ASSERT( str::HasPrefixI( "abc_Item_xyz", "ABx", 2 ) );		// N

		ASSERT( str::HasSuffix( "abc_Item_xyz", "" ) );				// match empty suffix
		ASSERT( !str::HasSuffix( "", "abc_Item_xyz" ) );			// but not the other way around

		ASSERT( str::HasSuffix( "abc_Item_xyz", "xyz" ) );
		ASSERT( !str::HasSuffix( "xyz", "abc_Item_xyz" ) );

		ASSERT( !str::HasSuffix( "abc_Item_xyz", "XYZ" ) );
		ASSERT( str::HasSuffixI( "abc_Item_xyz", "XYZ" ) );

		ASSERT( !str::HasSuffixI( "abc_Item_xyz", "YZa" ) );
		ASSERT( str::HasSuffixI( "abc_Item_xyz", "YZa", 2 ) );		// N
		ASSERT( str::HasSuffix( "abc_Item_xyz", "yzA", 2 ) );		// N
	}

	{	// WIDE
		ASSERT( str::HasPrefix( _T("abc_Item_xyz"), _T("") ) );
		ASSERT( !str::HasPrefix( _T(""), _T("abc_Item_xyz") ) );

		ASSERT( str::HasPrefix( _T("abc_Item_xyz"), _T("abc") ) );
		ASSERT( !str::HasPrefix( _T("abc"), _T("abc_Item_xyz") ) );

		ASSERT( !str::HasPrefix( _T("abc_Item_xyz"), _T("ABC") ) );
		ASSERT( str::HasPrefixI( _T("abc_Item_xyz"), _T("ABC") ) );

		ASSERT( !str::HasPrefix( _T("abc_Item_xyz"), _T("abx") ) );
		ASSERT( str::HasPrefix( _T("abc_Item_xyz"), _T("abx"), 2 ) );		// N
		ASSERT( str::HasPrefixI( _T("abc_Item_xyz"), _T("ABx"), 2 ) );		// N

		ASSERT( str::HasSuffix( _T("abc_Item_xyz"), _T("") ) );
		ASSERT( !str::HasSuffix( _T(""), _T("abc_Item_xyz") ) );

		ASSERT( str::HasSuffix( _T("abc_Item_xyz"), _T("xyz") ) );
		ASSERT( !str::HasSuffix( _T("xyz"), _T("abc_Item_xyz") ) );

		ASSERT( !str::HasSuffix( _T("abc_Item_xyz"), _T("XYZ") ) );
		ASSERT( str::HasSuffixI( _T("abc_Item_xyz"), _T("XYZ") ) );

		ASSERT( !str::HasSuffixI( _T("abc_Item_xyz"), _T("YZa") ) );
		ASSERT( str::HasSuffixI( _T("abc_Item_xyz"), _T("YZa"), 2 ) );		// N
		ASSERT( str::HasSuffix( _T("abc_Item_xyz"), _T("yzA"), 2 ) );		// N
	}
}

void CStringTests::TestStringConversion( void )
{
	std::tstring io;

	io = _T("preFix");
	ASSERT( !str::StripPrefix( io, _T("PRE") ) );
	ASSERT( str::StripPrefix( io, _T("pre") ) );
	ASSERT_EQUAL( _T("Fix"), io );

	io = _T("preFix");
	ASSERT( !str::StripSuffix( io, _T("fix") ) );
	ASSERT( str::StripSuffix( io, _T("Fix") ) );
	ASSERT_EQUAL( _T("pre"), io );

	ASSERT_EQUAL( _T("proposition"), str::FormatTruncate( _T("proposition"), 256 ) );

	ASSERT_EQUAL( 11, str::GetLength( _T("proposition") ) );
	ASSERT_EQUAL( _T("proposition"), str::FormatTruncate( _T("proposition"), 11 ) );
	ASSERT_EQUAL( _T("proposit..."), str::FormatTruncate( _T("propositionX"), 11 ) );
	ASSERT_EQUAL( _T("proposit..."), str::FormatTruncate( _T("propositionXY"), 11 ) );

	ASSERT_EQUAL( _T("prop..."), str::FormatTruncate( _T("proposition"), 7 ) );
	ASSERT_EQUAL( _T("...tion"), str::FormatTruncate( _T("proposition"), 7, _T("..."), false ) );
	ASSERT_EQUAL( _T("propETC"), str::FormatTruncate( _T("proposition"), 7, _T("ETC") ) );
	ASSERT_EQUAL( _T("ETCtion"), str::FormatTruncate( _T("proposition"), 7, _T("ETC"), false ) );

	ASSERT_EQUAL( _T("i1\xB6i2\xB6i3\xB6"), str::FormatSingleLine( _T("i1\ni2\r\ni3\n") ) );
	ASSERT_EQUAL( _T("i1|i2|i3|"), str::FormatSingleLine( _T("i1\ni2\r\ni3\n"), utl::npos, _T("|") ) );
	ASSERT_EQUAL( _T("i1..."), str::FormatSingleLine( _T("i1\ni2\r\ni3\n"), 5, _T("|") ) );

	io = _T("a1b1c1d1");
	ASSERT_EQUAL( 0, str::Replace( io, _T(""), _T(",") ) );
	ASSERT_EQUAL( _T("a1b1c1d1"), io );

	ASSERT_EQUAL( 4, str::Replace( io, _T("1"), _T("3,") ) );
	ASSERT_EQUAL( _T("a3,b3,c3,d3,"), io );

	ASSERT_EQUAL( 4, str::Replace( io, _T("3,"), _T("") ) );
	ASSERT_EQUAL( _T("abcd"), io );
}

void CStringTests::TestStringSearch( void )
{
	ASSERT_EQUAL_STR( _T(";mn"), str::FindTokenEnd( _T("abc;mn"), _T(",;") ) );
	ASSERT_EQUAL_STR( _T(",xy"), str::FindTokenEnd( _T("abc;mn,xy"), _T(",") ) );
	ASSERT_EQUAL_STR( _T(""), str::FindTokenEnd( _T("abc;mn,xy"), _T(">") ) );
}

void CStringTests::TestStringMatch( void )
{
	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::Case>( _T("abcText"), _T("abc") ) );
	ASSERT_EQUAL_STR( _T("ABcText"), str::SkipPrefix<str::Case>( _T("ABcText"), _T("aBC") ) );

	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::IgnoreCase>( _T("abcText"), _T("abc") ) );
	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::IgnoreCase>( _T("ABcText"), _T("aBC") ) );

	str::TGetMatch getMatchFunc;
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T(""), _T("") ) );
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T("SomeText"), _T("SomeText") ) );
	ASSERT_EQUAL( str::MatchEqualDiffCase, getMatchFunc( _T("SomeText"), _T("sometext") ) );
	ASSERT_EQUAL( str::MatchNotEqual, getMatchFunc( _T("Some"), _T("Text") ) );
}

void CStringTests::TestStringSequence( void )
{
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "", str::CSequence<char>( "a text", 1 ) ) );
	ASSERT_EQUAL( 2, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"liquid", 2 ) ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"liquid", 3 ) ) );

	ASSERT_EQUAL( 2, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );
	ASSERT_EQUAL( 2, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"LIQUID", 2 ), pred::TCompareNoCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ), pred::TCompareCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ) ) );

	std::vector<std::string> items;
	ASSERT( !AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );

	items.push_back( "a line" );
	ASSERT( AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CSequence<char>( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );

	items.push_back( "OS linux" );
	ASSERT( AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CSequence<char>( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );

	items.push_back( "Red Hat Linux" );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );
}

void CStringTests::TestStringOccurenceCount( void )
{
	ASSERT_EQUAL( 0, str::GetCountOf<str::Case>( "abcde", "" ) );
	ASSERT_EQUAL( 0, str::GetCountOf<str::Case>( "abcde", " " ) );
	ASSERT_EQUAL( 1, str::GetCountOf<str::Case>( "abcde", "a" ) );
	ASSERT_EQUAL( 1, str::GetCountOf<str::Case>( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 2, str::GetCountOf<str::IgnoreCase>( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 2, str::GetCountOf<str::IgnoreCase>( _T("abcdeABC"), _T("a") ) );

	ASSERT_EQUAL( 0, str::GetSequenceCount( "abc", str::MakeSequence( "" ) ) );
	ASSERT_EQUAL( 1, str::GetSequenceCount( _T("abc"), str::MakeSequence( _T("b") ) ) );
	ASSERT_EQUAL( 1, str::GetSequenceCount( _T("abcA"), str::MakeSequence( _T("a") ) ) );

	static const TCHAR* sepArray[] = { _T(";"), _T("|"), _T("\r\n"), _T("\n") };
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("ABC") ) ) );
	ASSERT_EQUAL_STR( _T("\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A\nB\nC") ) ) );
	ASSERT_EQUAL_STR( _T("\r\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A\r\nB\r\nC") ) ) );
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A|B;C|D;E;F") ) ) );
}

void CStringTests::TestStringLines( void )
{
	static const char s_lineEnd[] = "\n";

	std::vector<std::string> lines, outLines;

	str::Split( lines, "", s_lineEnd );
	ASSERT_EQUAL( "", str::JoinLines( lines, s_lineEnd ) );
	str::SplitLines( outLines, str::JoinLines( lines, s_lineEnd ).c_str(), s_lineEnd );
	ASSERT( lines == outLines );

	str::Split( lines, "1", s_lineEnd );
	ASSERT_EQUAL( "1", str::JoinLines( lines, s_lineEnd ) );
	str::SplitLines( outLines, str::JoinLines( lines, s_lineEnd ).c_str(), s_lineEnd );
	ASSERT( lines == outLines );

	str::Split( lines, "1\n22", s_lineEnd );
	ASSERT_EQUAL( "1\n22\n", str::JoinLines( lines, s_lineEnd ) );							// final line terminator
	str::SplitLines( outLines, str::JoinLines( lines, s_lineEnd ).c_str(), s_lineEnd );
	ASSERT( lines == outLines );

	str::Split( lines, "1\n22\n333", s_lineEnd );
	ASSERT_EQUAL( "1\n22\n333\n", str::JoinLines( lines, s_lineEnd ) );						// final line terminator
	str::SplitLines( outLines, str::JoinLines( lines, s_lineEnd ).c_str(), s_lineEnd );
	ASSERT( lines == outLines );
}


void CStringTests::TestSearchEnclosedItems( void )
{
	str::CEnclosedParser<char> parser( "$(|%", ")|%" );
	std::string text = "lead_%MY_STUFF%_mid_$(MY_TOOLS)__%VAR.1%_trail";

	ASSERT( parser.IsValid() );

	{
		str::CEnclosedParser<char>::TSepMatchPos sepMatchPos;
		str::CEnclosedParser<char>::TIdentSpecPair specBounds;

		specBounds = parser.FindItem( &sepMatchPos, text );
		ASSERT( specBounds.first != std::string::npos );
		ASSERT_EQUAL( "%", parser.GetStartSep( sepMatchPos ) );
		ASSERT_EQUAL( "%", parser.GetEndSep( sepMatchPos ) );
		ASSERT_EQUAL( "%MY_STUFF%", parser.MakeSpec( specBounds, text ) );
		ASSERT_EQUAL( "MY_STUFF", parser.MakeIdentifier( sepMatchPos, specBounds, text ) );

		specBounds = parser.FindItem( &sepMatchPos, text, specBounds.second );
		ASSERT( specBounds.first != std::string::npos );
		ASSERT_EQUAL( "$(MY_TOOLS)", parser.MakeSpec( specBounds, text ) );
		ASSERT_EQUAL( "MY_TOOLS", parser.MakeIdentifier( sepMatchPos, specBounds, text ) );

		// test last spec, which is free-form, i.e. not an identifier
		str::CEnclosedParser<char>::TIdentSpecPair nonIdentSpecBounds;
		nonIdentSpecBounds = parser.FindItem( &sepMatchPos, text, specBounds.second );
		ASSERT( std::string::npos == nonIdentSpecBounds.first );		// "%VAR.1%" not found when matching identifiers
	}

	{
		std::vector<std::string> items;

		parser.QueryItems( items, text, true );
		ASSERT_EQUAL( "%MY_STUFF%,$(MY_TOOLS)", str::Join( items, "," ) );

		parser.QueryItems( items, text, false );
		ASSERT_EQUAL( "MY_STUFF,MY_TOOLS", str::Join( items, "," ) );
	}
	{	// including non-identifiers:
		str::CEnclosedParser<char> parserAny( "$(|%", ")|%", false );
		std::vector<std::string> items;

		parserAny.QueryItems( items, text, true );
		ASSERT_EQUAL( "%MY_STUFF%,$(MY_TOOLS),%VAR.1%", str::Join( items, "," ) );

		parserAny.QueryItems( items, text, false );
		ASSERT_EQUAL( "MY_STUFF,MY_TOOLS,VAR.1", str::Join( items, "," ) );
	}
}

void CStringTests::TestReplaceEnclosedItems( void )
{
	{
		std::string text = "%MY_STUFF%$(MY_TOOLS)%VAR.1%";
		str::CEnclosedParser<char> parser( "$(|%", ")|%" );
		ASSERT_EQUAL( 2, parser.ReplaceSeparators( text, "#{<", ">}" ) );
		ASSERT_EQUAL( "#{<MY_STUFF>}#{<MY_TOOLS>}%VAR.1%", text );
	}
	{
		std::string text = "%MY_STUFF%$(MY_TOOLS)%VAR.1%";
		str::CEnclosedParser<char> parserAny( "$(|%", ")|%", false );
		ASSERT_EQUAL( 3, parserAny.ReplaceSeparators( text, "#{<", ">}" ) );
		ASSERT_EQUAL( "#{<MY_STUFF>}#{<MY_TOOLS>}#{<VAR.1>}", text );
	}
}


void CStringTests::TestArgUtilities( void )
{
	ASSERT( arg::Equals( _T("apple"), _T("apple") ) );
	ASSERT( arg::Equals( _T("apple"), _T("APPLE") ) );
	ASSERT( !arg::Equals( _T("apple"), _T("appleX") ) );

	ASSERT( arg::EqualsAnyOf( _T("apple"), _T("apple") ) );
	ASSERT( arg::EqualsAnyOf( _T("apple"), _T("apple|orange") ) );
	ASSERT( arg::EqualsAnyOf( _T("apple"), _T("orange|apple") ) );
	ASSERT( !arg::EqualsAnyOf( _T("apple"), _T("orange|banana") ) );

	ASSERT( arg::StartsWith( _T("arg:345 ab cd"), _T("arg") ) );
	ASSERT( arg::StartsWith( _T("arg:345 ab cd"), _T("ARG") ) );

	ASSERT( arg::StartsWithAnyOf( _T("Source_Value"), _T("source|s"), _T("|") ) );
	ASSERT( arg::StartsWithAnyOf( _T("Source_Value"), _T("SOURCE|S"), _T(">|") ) );
	ASSERT( !arg::StartsWithAnyOf( _T("Source_Value"), _T("our|ource"), _T("|") ) );		// no prefix match
	ASSERT( !arg::StartsWithAnyOf( _T("Source_Value"), _T("SOURCE|S"), _T("$") ) );			// bad list sep

	std::tstring value;
	ASSERT( arg::ParseValuePair( value, _T("Source=Value"), _T("source") ) );
	ASSERT_EQUAL( _T("Value"), value );

	ASSERT( arg::ParseValuePair( value, _T("Source=Value1"), _T("s") ) );
	ASSERT_EQUAL( _T("Value1"), value );

	ASSERT( arg::ParseValuePair( value, _T("Source=Value2"), _T("source|s") ) );
	ASSERT_EQUAL( _T("Value2"), value );

	ASSERT( arg::ParseValuePair( value, _T("SOURCE:Value3"), _T("source|s"), _T(':') ) );
	ASSERT_EQUAL( _T("Value3"), value );

	ASSERT( !arg::ParseValuePair( value, _T("SOURCE:Value3"), _T("source|s"), _T(':'), _T("$") ) );

	ASSERT( !arg::ParseValuePair( value, _T("Source=Value3"), _T("source"), _T('>') ) );
	ASSERT( arg::ParseOptionalValuePair( &value, _T("Source=Value3"), _T("source"), _T('>') ) );


	ASSERT_EQUAL( _T(""), arg::Enquote( "" ) );
	ASSERT_EQUAL( _T(""), arg::Enquote( "", '\'' ) );
	ASSERT_EQUAL( _T("'x'"), arg::Enquote( "x", '\'' ) );

	ASSERT_EQUAL( _T("\"x\""), arg::Enquote( 'x' ) );
	ASSERT_EQUAL( _T("\"x\""), arg::Enquote( L'x' ) );
	ASSERT_EQUAL( _T("\"abc\""), arg::Enquote( "abc" ) );
	ASSERT_EQUAL( _T("\"abc\""), arg::Enquote( _T("abc") ) );
	ASSERT_EQUAL( _T("\"abc\""), arg::Enquote( std::string( "abc" ) ) );
	ASSERT_EQUAL( _T("\"abc\""), arg::Enquote( std::wstring( L"abc" ) ) );
	ASSERT_EQUAL( _T("\"name.ext\""), arg::Enquote( fs::CPath( _T("name.ext") ) ) );

	ASSERT_EQUAL( _T(""), arg::AutoEnquote( "" ) );
	ASSERT_EQUAL( _T(""), arg::AutoEnquote( '\0' ) );
	ASSERT_EQUAL( _T("abc"), arg::AutoEnquote( _T("abc") ) );
	ASSERT_EQUAL( _T("\"abc d\""), arg::AutoEnquote( _T("abc d") ) );
	ASSERT_EQUAL( _T("name.ext"), arg::AutoEnquote( fs::CPath( _T("name.ext") ) ) );
	ASSERT_EQUAL( _T("\"name space.ext\""), arg::AutoEnquote( fs::CPath( _T("name space.ext") ) ) );
}

void CStringTests::TestEnumTags( void )
{
	enum Fruit { Plum, Apple, Peach };
	CEnumTags tags( _T("Plums|Apples|Peaches"), _T("plum|apple|peach"), Apple );

	ASSERT_EQUAL( 3, tags.GetUiTags().size() );
	ASSERT_EQUAL( _T("plum"), tags.FormatKey( Plum ) );
	ASSERT_EQUAL( _T("apple"), tags.FormatKey( Apple ) );
	ASSERT_EQUAL( _T("peach"), tags.FormatKey( Peach ) );

	ASSERT_EQUAL( _T("Plums"), tags.FormatUi( Plum ) );
	ASSERT_EQUAL( _T("Apples"), tags.FormatUi( Apple ) );
	ASSERT_EQUAL( _T("Peaches"), tags.FormatUi( Peach ) );

	ASSERT_EQUAL( Plum, tags.ParseKey( _T("plum") ) );
	ASSERT_EQUAL( Apple, tags.ParseKey( _T("aPPLe") ) );
	ASSERT_EQUAL( Apple, tags.ParseKey( _T("grape") ) );

	ASSERT_EQUAL( Plum, tags.ParseUi( _T("Plums") ) );
	ASSERT_EQUAL( Peach, tags.ParseUi( _T("PEACHES") ) );
	ASSERT_EQUAL( Apple, tags.ParseUi( _T("grapes") ) );
}

void CStringTests::TestFlagTags( void )
{
	// continuous tags
	{
		CFlagTags shareTags( _T("Share Read|Share Write|Share Delete"), _T("FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE") );

		ASSERT_EQUAL( 3, shareTags.GetUiTags().size() );
		ASSERT_EQUAL( FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, shareTags.GetFlagsMask() );

		ASSERT_EQUAL( _T(""), shareTags.FormatKey( 0 ) );
		ASSERT_EQUAL( _T(""), shareTags.FormatKey( FILE_ATTRIBUTE_TEMPORARY ) );		// stray flag
		ASSERT_EQUAL( _T("FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE"), shareTags.FormatKey( FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE ) );
		ASSERT_EQUAL( _T("Share Write--Share Delete"), shareTags.FormatUi( FILE_SHARE_WRITE | FILE_SHARE_DELETE, _T("--") ) );
		ASSERT_EQUAL( _T("Share Delete"), shareTags.FormatUi( FILE_SHARE_DELETE ) );

		int flags = FILE_ATTRIBUTE_TEMPORARY;			// stray flag
		shareTags.ParseKey( &flags, _T("") );
		ASSERT_EQUAL( FILE_ATTRIBUTE_TEMPORARY, flags );

		flags = FILE_SHARE_READ | FILE_ATTRIBUTE_TEMPORARY;
		shareTags.ParseKey( &flags, _T("FILE_SHARE_DELETE|FILE_SHARE_WRITE") );
		ASSERT_EQUAL( FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_ATTRIBUTE_TEMPORARY, flags );

		flags = FILE_ATTRIBUTE_TEMPORARY;
		shareTags.ParseUi( &flags, _T("") );
		ASSERT_EQUAL( FILE_ATTRIBUTE_TEMPORARY, flags );

		flags = FILE_SHARE_READ | FILE_ATTRIBUTE_TEMPORARY;
		shareTags.ParseUi( &flags, _T("Share Delete|Share Write") );
		ASSERT_EQUAL( FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_ATTRIBUTE_TEMPORARY, flags );
	}

	// sparse tags
	{
		static const CFlagTags::FlagDef flagDefs[] =
		{
			{ FILE_ATTRIBUTE_READONLY, _T("RO") },
			{ FILE_ATTRIBUTE_HIDDEN, _T("H") },
			{ FILE_ATTRIBUTE_SYSTEM, _T("S") }
		};

		// key tags == UI tags
		{
			CFlagTags faTags( flagDefs, COUNT_OF( flagDefs ) );
			ASSERT_EQUAL( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, faTags.GetFlagsMask() );
			ASSERT_EQUAL( CFlagTags::FindBitPos( FILE_ATTRIBUTE_SYSTEM ) + 1, faTags.GetUiTags().size() );

			ASSERT_EQUAL( _T(""), faTags.FormatKey( 0 ) );
			ASSERT_EQUAL( _T(""), faTags.FormatKey( FILE_ATTRIBUTE_TEMPORARY ) );		// stray flag
			ASSERT_EQUAL( _T("RO|H|S"), faTags.FormatKey( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY ) );
			ASSERT_EQUAL( _T("H|S"), faTags.FormatKey( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN ) );

			int flags = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY;
			faTags.ParseKey( &flags, _T("foo|S|H") );
			ASSERT_EQUAL( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY, flags );
		}

		// key tags != UI tags
		{
			CFlagTags faTags( flagDefs, COUNT_OF( flagDefs ), _T("ReadOnly|Hidden|System") );
			ASSERT_EQUAL( faTags.GetKeyTags().size(), faTags.GetUiTags().size() );
			ASSERT( faTags.GetKeyTags() != faTags.GetUiTags() );
			ASSERT_EQUAL( _T("Hidden|System"), faTags.FormatUi( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY ) );

			int flags = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY;
			faTags.ParseUi( &flags, _T("foo|ReadOnly|Hidden") );
			ASSERT_EQUAL( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY, flags );
		}
	}
}

void CStringTests::TestWordSelection( void )
{
	static const std::tstring text = _T("some  'STUFF'?! ");

	ASSERT_EQUAL( word::WordStart, word::GetWordStatus( text, 0 ) );
	ASSERT_EQUAL( word::WordCore, word::GetWordStatus( text, 1 ) );
	ASSERT_EQUAL( word::WordCore, word::GetWordStatus( text, 3 ) );
	ASSERT_EQUAL( word::WordEnd, word::GetWordStatus( text, 4 ) );
	ASSERT_EQUAL( word::Whitespace, word::GetWordStatus( text, 5 ) );
	ASSERT_EQUAL( word::Whitespace, word::GetWordStatus( text, 6 ) );

	ASSERT_EQUAL( 0, word::FindPrevWordBreak( text, 0 ) );		// "some"
	ASSERT_EQUAL( 0, word::FindPrevWordBreak( text, 1 ) );
	ASSERT_EQUAL( 0, word::FindPrevWordBreak( text, 4 ) );
	ASSERT_EQUAL( 4, word::FindPrevWordBreak( text, 5 ) );
	ASSERT_EQUAL( 4, word::FindPrevWordBreak( text, 6 ) );
	ASSERT_EQUAL( 4, word::FindPrevWordBreak( text, 7 ) );		// "..STUFF"

	ASSERT_EQUAL( 7, word::FindPrevWordBreak( text, 8 ) );
	ASSERT_EQUAL( 7, word::FindPrevWordBreak( text, 12 ) );

	ASSERT_EQUAL( 12, word::FindPrevWordBreak( text, 13 ) );	// "'?! "
	ASSERT_EQUAL( 12, word::FindPrevWordBreak( text, 14 ) );
	ASSERT_EQUAL( 12, word::FindPrevWordBreak( text, 15 ) );
	ASSERT_EQUAL( 12, word::FindPrevWordBreak( text, 16 ) );

	ASSERT_EQUAL( 4, word::FindNextWordBreak( text, 0 ) );
	ASSERT_EQUAL( 4, word::FindNextWordBreak( text, 1 ) );
	ASSERT_EQUAL( 4, word::FindNextWordBreak( text, 3 ) );

	ASSERT_EQUAL( 7, word::FindNextWordBreak( text, 4 ) );
	ASSERT_EQUAL( 7, word::FindNextWordBreak( text, 5 ) );
	ASSERT_EQUAL( 7, word::FindNextWordBreak( text, 6 ) );

	ASSERT_EQUAL( 12, word::FindNextWordBreak( text, 7 ) );
	ASSERT_EQUAL( 12, word::FindNextWordBreak( text, 8 ) );
	ASSERT_EQUAL( 12, word::FindNextWordBreak( text, 11 ) );

	ASSERT_EQUAL( 16, word::FindNextWordBreak( text, 12 ) );
	ASSERT_EQUAL( 16, word::FindNextWordBreak( text, 13 ) );
	ASSERT_EQUAL( 16, word::FindNextWordBreak( text, 15 ) );
	ASSERT_EQUAL( 16, word::FindNextWordBreak( text, 16 ) );
}

void CStringTests::TestEnsureUniformNumPadding( void )
{
	static const TCHAR comma[] = _T(",");
	{
		std::vector<std::tstring> items;
		str::Split( items, _T("a,b"), comma );
		ASSERT_EQUAL( 0, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a,b"), str::Join( items, comma ) );
	}
	{
		std::vector<std::tstring> items;
		str::Split( items, _T("a1,b"), comma );
		ASSERT_EQUAL( 1, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a1,b"), str::Join( items, comma ) );
	}
	{
		std::vector<std::tstring> items;
		str::Split( items, _T("a1x,bcd23"), comma );
		ASSERT_EQUAL( 1, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a01x,bcd23"), str::Join( items, comma ) );
	}
	{
		std::vector<std::tstring> items;
		str::Split( items, _T(" 1 19 ,   23 "), comma );
		ASSERT_EQUAL( 2, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T(" 01 19 ,   23 "), str::Join( items, comma ) );
	}
	{
		std::vector<std::tstring> items;
		str::Split( items, _T("1 19,23 7-00"), comma );
		ASSERT_EQUAL( 3, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("01 19,23 07-0"), str::Join( items, comma ) );
	}
	{
		std::vector<std::tstring> items;
		str::Split( items, _T("1 19 3, 23 000007 1289 ,0-0-0-0"), comma );
		ASSERT_EQUAL( 4, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("01 19 0003, 23 07 1289 ,00-00-0000-0"), str::Join( items, comma ) );
	}
}

void CStringTests::TestTimeFormatting( void )
{
	static const std::tstring text = _T("27-12-2017 19:54:20");

	using namespace str;

	CTime dt = time_utl::ParseTimestamp( text );
	ASSERT_EQUAL( CTime( 2017, 12, 27, 19, 54, 20 ), dt );
	ASSERT_EQUAL( _T("27-12-2017 19:54:20"), time_utl::FormatTimestamp( dt ) );
}

void CStringTests::TestFunctional( void )
{
	// code demo - not a real unit test
	fs::CPath path( _T("C:\\Fruit\\apple.jpg") );

	std::function< const TCHAR* ( void ) > getNameExtMethod = std::bind( &fs::CPath::GetFilenamePtr, &path );
	ASSERT_EQUAL_STR( _T("apple.jpg"), getNameExtMethod() );

	using std::placeholders::_1;
	std::function< void ( const std::tstring& ) > setNameExtMethod = std::bind( &fs::CPath::SetFilename, &path, _1 );
	setNameExtMethod( _T("orange.png") );
	ASSERT_EQUAL_STR( _T("orange.png"), getNameExtMethod() );
}


void CStringTests::Run( void )
{
	RUN_TEST( TestCharTraits );
	RUN_TEST( TestIsCharType );
	RUN_TEST( TestValueToString );
	RUN_TEST( TestTrim );
	RUN_TEST( TestEnquote );
	RUN_TEST( TestStringSplit );
	RUN_TEST( TestStringTokenize );
	RUN_TEST( TestStringPrefixSuffix );
	RUN_TEST( TestStringConversion );
	RUN_TEST( TestStringSearch );
	RUN_TEST( TestStringMatch );
	RUN_TEST( TestStringSequence );
	RUN_TEST( TestStringOccurenceCount );
	RUN_TEST( TestStringLines );

	RUN_TEST( TestSearchEnclosedItems );
	RUN_TEST( TestReplaceEnclosedItems );
	RUN_TEST( TestArgUtilities );
	RUN_TEST( TestEnumTags );
	RUN_TEST( TestFlagTags );
	RUN_TEST( TestWordSelection );
	RUN_TEST( TestEnsureUniformNumPadding );
	RUN_TEST( TestTimeFormatting );

	RUN_TEST( TestFunctional );
}


#endif //USE_UT
