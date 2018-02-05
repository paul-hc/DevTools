
#include "stdafx.h"
#include "ut/StringTests.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "StringUtilities.h"
#include "vector_map.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace str
{
	inline std::tostream& operator<<( std::tostream& oss, const fs::CPath& path )
	{
		return oss << path.Get();
	}
}


namespace func
{
	struct KeyToValue
	{
		std::tstring operator()( const std::tstring& key ) const
		{
			if ( key == _T("NAME") )
				return _T("UTL");
			else if ( key == _T("MAJOR") )
				return _T("3");
			else if ( key == _T("MINOR") )
				return _T("12");
			return str::Format( _T("?%s?"), key.c_str() );
		}
	};
}


CStringTests::CStringTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStringTests& CStringTests::Instance( void )
{
	static CStringTests testCase;
	return testCase;
}

void CStringTests::TestIgnoreCase( void )
{
	std::string s( "bcd" );
	ASSERT( "BCD" != s );
	ASSERT( s != "BCD" );

	using namespace str::ignore_case;

	ASSERT( "BCD" == s );
	ASSERT( s == "BCD" );
	ASSERT( s != "xy" );
}

void CStringTests::TestStringSplit( void )
{
	static const TCHAR whitespaceText[] = _T("	  ab c 		");
	std::tstring text;
	ASSERT_EQUAL( _T("ab c 		"), str::TrimLeft( text = whitespaceText ) );
	ASSERT_EQUAL( _T("	  ab c"), str::TrimRight( text = whitespaceText ) );

	ASSERT_EQUAL( _T("ab c"), str::Trim( text = whitespaceText ) );

	{	// WIDE strings
		std::vector< std::tstring > items;
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
		std::vector< std::string > items;
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
	std::vector< std::tstring > tokens;
	ASSERT_EQUAL( 0, str::Tokenize( tokens, _T(""), seps ) );
	ASSERT_EQUAL( 6, str::Tokenize( tokens, _T("\n\t,apple,grape;plum pear\tkiwi\nbanana \n\t"), seps ) );
	ASSERT_EQUAL_STR( _T("apple|grape|plum|pear|kiwi|banana"), str::Join( tokens, _T("|") ) );
}

void CStringTests::TestStringConversion( void )
{
	std::tstring io;

	io = _T("preFix");
	ASSERT( !str::StripPrefix( io, _T("PRE") ) );
	ASSERT( str::StripPrefix( io, _T("pre") ) );
	ASSERT_EQUAL_STR( _T("Fix"), io );

	io = _T("preFix");
	ASSERT( !str::StripSuffix( io, _T("fix") ) );
	ASSERT( str::StripSuffix( io, _T("Fix") ) );
	ASSERT_EQUAL_STR( _T("pre"), io );

	ASSERT_EQUAL_STR( _T("proposition"), str::FormatTruncate( _T("proposition"), 256 ) );
	ASSERT_EQUAL_STR( _T("prop..."), str::FormatTruncate( _T("proposition"), 7 ) );
	ASSERT_EQUAL_STR( _T("...tion"), str::FormatTruncate( _T("proposition"), 7, _T("..."), false ) );
	ASSERT_EQUAL_STR( _T("propETC"), str::FormatTruncate( _T("proposition"), 7, _T("ETC") ) );
	ASSERT_EQUAL_STR( _T("ETCtion"), str::FormatTruncate( _T("proposition"), 7, _T("ETC"), false ) );

	ASSERT_EQUAL_STR( _T("i1\xB6i2\xB6i3\xB6"), str::FormatSingleLine( _T("i1\ni2\r\ni3\n") ) );
	ASSERT_EQUAL_STR( _T("i1|i2|i3|"), str::FormatSingleLine( _T("i1\ni2\r\ni3\n"), utl::npos, _T("|") ) );
	ASSERT_EQUAL_STR( _T("i1..."), str::FormatSingleLine( _T("i1\ni2\r\ni3\n"), 5, _T("|") ) );

	io = _T("a1b1c1d1");
	ASSERT_EQUAL( 0, str::Replace( io, _T(""), _T(",") ) );
	ASSERT_EQUAL_STR( io, _T("a1b1c1d1") );

	ASSERT_EQUAL( 4, str::Replace( io, _T("1"), _T("3,") ) );
	ASSERT_EQUAL_STR( io, _T("a3,b3,c3,d3,") );

	ASSERT_EQUAL( 4, str::Replace( io, _T("3,"), _T("") ) );
	ASSERT_EQUAL_STR( io, _T("abcd") );
}

void CStringTests::TestStringSearch( void )
{
	ASSERT_EQUAL_STR( _T(";mn"), str::FindTokenEnd( _T("abc;mn"), _T(",;") ) );
	ASSERT_EQUAL_STR( _T(",xy"), str::FindTokenEnd( _T("abc;mn,xy"), _T(",") ) );
	ASSERT_EQUAL_STR( _T(""), str::FindTokenEnd( _T("abc;mn,xy"), _T(">") ) );
}

void CStringTests::TestStringMatch( void )
{
	str::GetMatch getMatchFunc;
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T(""), _T("") ) );
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T("SomeText"), _T("SomeText") ) );
	ASSERT_EQUAL( str::MatchEqualDiffCase, getMatchFunc( _T("SomeText"), _T("sometext") ) );
	ASSERT_EQUAL( str::MatchNotEqual, getMatchFunc( _T("Some"), _T("Text") ) );
}

void CStringTests::TestStringPart( void )
{
	ASSERT_EQUAL( std::tstring::npos, str::FindPart( "", str::CPart< char >( "a text", 1 ) ) );
	ASSERT_EQUAL( 2, str::FindPart( L"a line", str::CPart< wchar_t >( L"liquid", 2 ) ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindPart( L"a line", str::CPart< wchar_t >( L"liquid", 3 ) ) );

	ASSERT_EQUAL( 2, str::FindPart( "a line", str::CPart< char >( "LIQUID", 2 ), pred::CompareNoCase() ) );
	ASSERT_EQUAL( 2, str::FindPart( L"a line", str::CPart< wchar_t >( L"LIQUID", 2 ), pred::CompareNoCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindPart( "a line", str::CPart< char >( "LIQUID", 2 ), pred::CompareCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindPart( "a line", str::CPart< char >( "LIQUID", 2 ) ) );

	std::vector< std::string > items;
	ASSERT( !AllContain( items, str::CPart< char >( "liquid", 2 ) ) );

	items.push_back( "a line" );
	ASSERT( AllContain( items, str::CPart< char >( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CPart< char >( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CPart< char >( "LIQUID", 2 ), pred::CompareNoCase() ) );

	items.push_back( "OS linux" );
	ASSERT( AllContain( items, str::CPart< char >( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CPart< char >( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CPart< char >( "LIQUID", 2 ), pred::CompareNoCase() ) );

	items.push_back( "Red Hat Linux" );
	ASSERT( AllContain( items, str::CPart< char >( "LIQUID", 2 ), pred::CompareNoCase() ) );
}

void CStringTests::TestStringOccurenceCount( void )
{
	ASSERT_EQUAL( 0, str::GetCountOf< str::Case >( "abcde", "" ) );
	ASSERT_EQUAL( 0, str::GetCountOf< str::Case >( "abcde", " " ) );
	ASSERT_EQUAL( 1, str::GetCountOf< str::Case >( "abcde", "a" ) );
	ASSERT_EQUAL( 1, str::GetCountOf< str::Case >( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 2, str::GetCountOf< str::IgnoreCase >( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 2, str::GetCountOf< str::IgnoreCase >( _T("abcdeABC"), _T("a") ) );

	ASSERT_EQUAL( 0, str::GetPartCount( "abc", str::MakePart( "" ) ) );
	ASSERT_EQUAL( 1, str::GetPartCount( _T("abc"), str::MakePart( _T("b") ) ) );
	ASSERT_EQUAL( 1, str::GetPartCount( _T("abcA"), str::MakePart( _T("a") ) ) );

	static const TCHAR* sepArray[] = { _T(";"), _T("|"), _T("\r\n"), _T("\n") };
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessPartCount< TCHAR >( _T("ABC") ) ) );
	ASSERT_EQUAL_STR( _T("\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessPartCount< TCHAR >( _T("A\nB\nC") ) ) );
	ASSERT_EQUAL_STR( _T("\r\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessPartCount< TCHAR >( _T("A\r\nB\r\nC") ) ) );
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessPartCount< TCHAR >( _T("A|B;C|D;E;F") ) ) );
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

void CStringTests::TestExpandKeysToValues( void )
{
	ASSERT_EQUAL( _T(""),
		str::ExpandKeysToValues( _T(""), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("About"),
		str::ExpandKeysToValues( _T("About"), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("??"),
		str::ExpandKeysToValues( _T("[]"), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("?[X]?"),
		str::ExpandKeysToValues( _T("[X]"), _T("["), _T("]"), func::KeyToValue(), true ) );

	ASSERT_EQUAL( _T("About UTL vesion 3.12 release ?FOO?."),
		str::ExpandKeysToValues( _T("About [NAME] vesion [MAJOR].[MINOR] release [FOO]."), _T("["), _T("]"), func::KeyToValue() ) );
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
		std::vector< std::tstring > items;
		str::Split( items, _T("a,b"), comma );
		ASSERT_EQUAL( 0, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a,b"), str::Join( items, comma ) );
	}
	{
		std::vector< std::tstring > items;
		str::Split( items, _T("a1,b"), comma );
		ASSERT_EQUAL( 1, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a1,b"), str::Join( items, comma ) );
	}
	{
		std::vector< std::tstring > items;
		str::Split( items, _T("a1x,bcd23"), comma );
		ASSERT_EQUAL( 1, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("a01x,bcd23"), str::Join( items, comma ) );
	}
	{
		std::vector< std::tstring > items;
		str::Split( items, _T(" 1 19 ,   23 "), comma );
		ASSERT_EQUAL( 2, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T(" 01 19 ,   23 "), str::Join( items, comma ) );
	}
	{
		std::vector< std::tstring > items;
		str::Split( items, _T("1 19,23 7-00"), comma );
		ASSERT_EQUAL( 3, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("01 19,23 07-0"), str::Join( items, comma ) );
	}
	{
		std::vector< std::tstring > items;
		str::Split( items, _T("1 19 3, 23 000007 1289 ,0-0-0-0"), comma );
		ASSERT_EQUAL( 4, num::EnsureUniformZeroPadding( items ) );
		ASSERT_EQUAL( _T("01 19 0003, 23 07 1289 ,00-00-0000-0"), str::Join( items, comma ) );
	}
}

void CStringTests::Test_vector_map( void )
{
	utl::vector_map< char, std::tstring > items;
	items[ '7' ] = _T("i7");
	items[ '9' ] = _T("i9");
	items[ '3' ] = _T("i3");
	items[ '1' ] = _T("i1");
	ASSERT_EQUAL( "1 3 7 9", ut::JoinKeys( items, _T(" ") ) );
	ASSERT_EQUAL( "i1,i3,i7,i9", ut::JoinValues( items, _T(",") ) );

	items.EraseKey( '3' );
	ASSERT_EQUAL( "1 7 9", ut::JoinKeys( items, _T(" ") ) );
}

void CStringTests::TestFunctional( void )
{
	// code demo - not a real unit test
	fs::CPath path( _T("C:\\Fruit\\apple.jpg") );

	// const TCHAR* GetNameExt( void ) const { return path::FindFilename( m_filePath.c_str() ); }

	std::function< const TCHAR* ( void ) > getNameExtMethod = std::bind( &fs::CPath::GetNameExt, &path );
	ASSERT_EQUAL_STR( _T("apple.jpg"), getNameExtMethod() );

	// void SetNameExt( const std::tstring& nameExt );

	using std::placeholders::_1;
	std::function< void ( const std::tstring& ) > setNameExtMethod = std::bind( &fs::CPath::SetNameExt, &path, _1 );
	setNameExtMethod( _T("orange.png") );
	ASSERT_EQUAL_STR( _T("orange.png"), getNameExtMethod() );
}

void CStringTests::Run( void )
{
	__super::Run();

	TestIgnoreCase();
	TestStringSplit();
	TestStringTokenize();
	TestStringConversion();
	TestStringSearch();
	TestStringMatch();
	TestStringPart();
	TestStringOccurenceCount();
	TestArgUtilities();
	TestEnumTags();
	TestFlagTags();
	TestExpandKeysToValues();
	TestWordSelection();
	TestEnsureUniformNumPadding();
	Test_vector_map();

//	TestFunctional();
}


#endif //_DEBUG
