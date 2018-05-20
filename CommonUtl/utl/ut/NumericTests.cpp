
#include "stdafx.h"
#include "ut/NumericTests.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CNumericTests::CNumericTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CNumericTests& CNumericTests::Instance( void )
{
	static CNumericTests testCase;
	return testCase;
}

void CNumericTests::TestFormatNumber( void )
{
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (signed char)32 ) );
		ASSERT_EQUAL( _T("-16"), num::FormatNumber( (signed char)-16 ) );		// i.e. 240

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (BYTE)32 ) );
		ASSERT_EQUAL( _T("240"), num::FormatNumber( (BYTE)240 ) );				// i.e. 0xF0
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (short)32 ) );
		ASSERT_EQUAL( _T("-4096"), num::FormatNumber( (short)-4096 ) );				// i.e. 0xF000

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned short)32 ) );
		ASSERT_EQUAL( _T("61440"), num::FormatNumber( (unsigned short)61440 ) );	// i.e. 0xF000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (int)32 ) );
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( (int)-268435456 ) );				// i.e. 0xF0000000
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( (int)4026531840 ) );				// i.e. -268435456

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned int)32 ) );
		ASSERT_EQUAL( _T("4026531840"), num::FormatNumber( (unsigned int)4026531840 ) );	// i.e. 0xF0000000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (long)32 ) );
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( (long)-268435456 ) );			// i.e. 0xF0000000
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( (long)4026531840 ) );			// i.e. -268435456

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned long)32 ) );
		ASSERT_EQUAL( _T("4026531840"), num::FormatNumber( (unsigned long)4026531840 ) );	// i.e. 0xF0000000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (__int64)32 ) );
		ASSERT_EQUAL( _T("-1152921504606846976"), num::FormatNumber( (__int64)-1152921504606846976 ) );				// i.e. 0xF000000000000000
		ASSERT_EQUAL( _T("-1152921504606846976"), num::FormatNumber( (__int64)17293822569102704640 ) );				// i.e. -1152921504606846976

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned __int64)32 ) );
		ASSERT_EQUAL( _T("17293822569102704640"), num::FormatNumber( (unsigned __int64)17293822569102704640 ) );	// i.e. 0xF000000000000000
	}

	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( 32.0 ) );
		ASSERT_EQUAL( _T("1234.56789"), num::FormatNumber( 1234.56789 ) );
		ASSERT_EQUAL( _T("-1234.56789"), num::FormatNumber( -1234.56789 ) );
	}
}

void CNumericTests::TestFormatNumberUserLocale( void )
{
	const std::locale& userLoc = str::GetUserLocale();

	ASSERT_EQUAL( _T("4,096"), num::FormatNumber( (short)4096, userLoc ) );
	ASSERT_EQUAL( _T("-4,096"), num::FormatNumber( (short)-4096, userLoc ) );				// i.e. 0xF000
	ASSERT_EQUAL( _T("61,440"), num::FormatNumber( (unsigned short)61440, userLoc ) );		// i.e. 0xF000

	ASSERT_EQUAL( _T("-268,435,456"), num::FormatNumber( (int)-268435456, userLoc ) );				// i.e. 0xF0000000
	ASSERT_EQUAL( _T("4,026,531,840"), num::FormatNumber( (unsigned int)4026531840, userLoc ) );	// i.e. 0xF0000000

	ASSERT_EQUAL( _T("-268,435,456"), num::FormatNumber( (long)-268435456, userLoc ) );				// i.e. 0xF0000000

	ASSERT_EQUAL( _T("-1,152,921,504,606,846,976"), num::FormatNumber( (__int64)-1152921504606846976, userLoc ) );				// i.e. 0xF000000000000000
	ASSERT_EQUAL( _T("17,293,822,569,102,704,640"), num::FormatNumber( (unsigned __int64)17293822569102704640, userLoc ) );	// i.e. 0xF000000000000000

	ASSERT_EQUAL( _T("1,234,000.56789"), num::FormatNumber( 1234000.56789, userLoc ) );
	ASSERT_EQUAL( _T("-1,234,000.56789"), num::FormatNumber( -1234000.56789, userLoc ) );
}

namespace ut
{
	template< typename ValueType >
	std::pair< bool, ValueType > ParseNumber( const std::tstring& text, size_t* pSkipLength = NULL, const std::locale& loc = num::GetEmptyLocale() )
	{
		std::pair< bool, ValueType > result;
		result.first = num::ParseNumber( result.second, text, pSkipLength, loc );
		return result;
	}
}

void CNumericTests::TestParseNumber( void )
{
	{
		ASSERT_EQUAL( std::make_pair( true, (signed char)32 ), ut::ParseNumber< signed char >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (signed char)-16 ), ut::ParseNumber< signed char >( _T("-16") ) );

		ASSERT_EQUAL( std::make_pair( true, (BYTE)32 ), ut::ParseNumber< BYTE >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (BYTE)240 ), ut::ParseNumber< BYTE >( _T("240") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (short)32 ), ut::ParseNumber< short >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (short)-4096 ), ut::ParseNumber< short >( _T("-4096") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned short)32 ), ut::ParseNumber< unsigned short >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned short)61440 ), ut::ParseNumber< unsigned short >( _T("61440") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (int)32 ), ut::ParseNumber< int >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (int)-268435456 ), ut::ParseNumber< int >( _T("-268435456") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned int)32 ), ut::ParseNumber< unsigned int >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned int)4026531840 ), ut::ParseNumber< unsigned int >( _T("4026531840") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (long)32 ), ut::ParseNumber< long >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (long)-268435456 ), ut::ParseNumber< long >( _T("-268435456") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned long)32 ), ut::ParseNumber< unsigned long >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned long)4026531840 ), ut::ParseNumber< unsigned long >( _T("4026531840") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (__int64)32 ), ut::ParseNumber< __int64 >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (__int64)-1152921504606846976 ), ut::ParseNumber< __int64 >( _T("-1152921504606846976") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)32 ), ut::ParseNumber< unsigned __int64 >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)17293822569102704640 ), ut::ParseNumber< unsigned __int64 >( _T("17293822569102704640") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, 32.0 ), ut::ParseNumber< double >( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, 1234.56789 ), ut::ParseNumber< double >( _T("1234.56789") ) );
		ASSERT_EQUAL( std::make_pair( true, -1234.56789 ), ut::ParseNumber< double >( _T("-1234.56789") ) );
	}

	// with skip length
	{
		static const std::tstring numText = _T(" -375some");
		size_t skipLength;
		ASSERT_EQUAL( std::make_pair( true, (int)-375 ), ut::ParseNumber< int >( numText, &skipLength ) );
		ASSERT_EQUAL( 5, skipLength );
		ASSERT_EQUAL( _T("some"), numText.substr( skipLength ) );
	}
}

void CNumericTests::TestParseNumberUserLocale( void )
{
	const std::locale& userLoc = str::GetUserLocale();
	{
		ASSERT_EQUAL( std::make_pair( true, (short)-4096 ), ut::ParseNumber< short >( _T("-4,096"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned short)61440 ), ut::ParseNumber< unsigned short >( _T("61,440"), NULL, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (int)32 ), ut::ParseNumber< int >( _T("32"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (int)-268435456 ), ut::ParseNumber< int >( _T("-268,435,456"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned int)4026531840 ), ut::ParseNumber< unsigned int >( _T("4,026,531,840"), NULL, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (long)-268435456 ), ut::ParseNumber< long >( _T("-268,435,456"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned long)4026531840 ), ut::ParseNumber< unsigned long >( _T("4,026,531,840"), NULL, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (__int64)-1152921504606846976 ), ut::ParseNumber< __int64 >( _T("-1,152,921,504,606,846,976"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)17293822569102704640 ), ut::ParseNumber< unsigned __int64 >( _T("17,293,822,569,102,704,640"), NULL, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, 32.0 ), ut::ParseNumber< double >( _T("32"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, 1234000.56789 ), ut::ParseNumber< double >( _T("1,234,000.56789"), NULL, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, -1234000.56789 ), ut::ParseNumber< double >( _T("-1,234,000.56789"), NULL, userLoc ) );
	}
}


void CNumericTests::Run( void )
{
	__super::Run();

	TestFormatNumber();
	TestFormatNumberUserLocale();
	TestParseNumber();
	TestParseNumberUserLocale();
}


#endif //_DEBUG
