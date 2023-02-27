
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/NumericTests.h"
#include "NumericProcessor.h"
#include "StringUtilities.h"
#include "Crc32.h"
#include "utl/AppTools.h"
#include "utl/MemLeakCheck.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	std::tstring GenerateNumbersInSequence( const TCHAR srcItems[], unsigned int startingNumber = UINT_MAX ) throws_( CRuntimeException )
	{
		static const TCHAR s_sep[] = _T("|");
		std::vector<std::tstring> items;
		str::Split( items, srcItems, s_sep );

		num::GenerateNumbersInSequence( items, startingNumber );
		return str::Join( items, s_sep );
	}
}


CNumericTests::CNumericTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CNumericTests& CNumericTests::Instance( void )
{
	static CNumericTests s_testCase;
	return s_testCase;
}

const std::vector<UINT>& CNumericTests::GetReferenceCrc32Table( void )
{
	static const UINT table[] =
	{
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
		0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
		0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
		0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
		0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
		0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
		0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
		0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
		0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
		0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
		0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

		0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
		0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
		0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
		0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
		0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
		0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
		0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
		0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
		0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
		0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
		0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
	};
	static const std::vector<UINT> s_crc32Table( table, table + COUNT_OF( table ) );
	return s_crc32Table;
}

void CNumericTests::TestFormatNumber( void )
{
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (signed char)32 ) );
		ASSERT_EQUAL( _T("-16"), num::FormatNumber( (signed char)-16 ) );			// i.e. 240

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (BYTE)32 ) );
		ASSERT_EQUAL( _T("240"), num::FormatNumber( (BYTE)240 ) );					// i.e. 0xF0
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (short)32 ) );
		ASSERT_EQUAL( _T("-4096"), num::FormatNumber( (short)-4096 ) );				// i.e. 0xF000

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned short)32 ) );
		ASSERT_EQUAL( _T("61440"), num::FormatNumber( (unsigned short)61440 ) );	// i.e. 0xF000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (int)32 ) );
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( static_cast<int>( -268435456 ) ) );			// i.e. 0xF0000000
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( static_cast<int>( 4026531840 ) ) );			// i.e. -268435456

		ASSERT_EQUAL( _T("32"), num::FormatNumber( (unsigned int)32 ) );
		ASSERT_EQUAL( _T("4026531840"), num::FormatNumber( (unsigned int)4026531840 ) );				// i.e. 0xF0000000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (long)32 ) );
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( static_cast<long>( -268435456 ) ) );			// i.e. 0xF0000000
		ASSERT_EQUAL( _T("-268435456"), num::FormatNumber( static_cast<long>( 4026531840 ) ) );			// i.e. -268435456

		ASSERT_EQUAL( _T("32"), num::FormatNumber( static_cast<unsigned long>( 32 ) ) );
		ASSERT_EQUAL( _T("4026531840"), num::FormatNumber( static_cast<unsigned long>( 4026531840 ) ) );	// i.e. 0xF0000000
	}
	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( (__int64)32 ) );
		ASSERT_EQUAL( _T("-1152921504606846976"), num::FormatNumber( static_cast<__int64>( -1152921504606846976 ) ) );		// i.e. 0xF000000000000000
		ASSERT_EQUAL( _T("-1152921504606846976"), num::FormatNumber( static_cast<__int64>( 17293822569102704640 ) ) );		// i.e. -1152921504606846976

		ASSERT_EQUAL( _T("32"), num::FormatNumber( static_cast<unsigned __int64>( 32 ) ) );
		ASSERT_EQUAL( _T("17293822569102704640"), num::FormatNumber( static_cast<unsigned __int64>( 17293822569102704640 ) ) );	// i.e. 0xF000000000000000
	}

	{
		ASSERT_EQUAL( _T("32"), num::FormatNumber( 32.0 ) );
		ASSERT_EQUAL( _T("1234.56789"), num::FormatNumber( 1234.56789 ) );
		ASSERT_EQUAL( _T("-1234.56789"), num::FormatNumber( -1234.56789 ) );
	}

	{	// with precision
		ASSERT_EQUAL( _T("1234.56789"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 5 ) ) );
		ASSERT_EQUAL( _T("1234.5678"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 4 ) ) );
		ASSERT_EQUAL( _T("1234.567"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 3 ) ) );
		ASSERT_EQUAL( _T("1234.56"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 2 ) ) );
		ASSERT_EQUAL( _T("1234.5"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 1 ) ) );
		ASSERT_EQUAL( _T("1234"), num::FormatNumber( num::GetWithPrecision( 1234.56789, 0 ) ) );
	}

	{	// rounding with precision
		ASSERT_EQUAL( _T("1234.56789"), num::FormatDouble( 1234.56789, 5 ) );
		ASSERT_EQUAL( _T("1234.5679"), num::FormatDouble( 1234.56789, 4 ) );
		ASSERT_EQUAL( _T("1234.568"), num::FormatDouble( 1234.56789, 3 ) );
		ASSERT_EQUAL( _T("1234.57"), num::FormatDouble( 1234.56789, 2 ) );
		ASSERT_EQUAL( _T("1234.6"), num::FormatDouble( 1234.56789, 1 ) );
		ASSERT_EQUAL( _T("1234"), num::FormatDouble( 1234.56789, 0 ) );

		ASSERT_EQUAL( _T("789.1234"), num::FormatDouble( 789.1234, 4 ) );
		ASSERT_EQUAL( _T("789.123"), num::FormatDouble( 789.1234, 3 ) );
		ASSERT_EQUAL( _T("789.12"), num::FormatDouble( 789.1234, 2 ) );
		ASSERT_EQUAL( _T("789.1"), num::FormatDouble( 789.1234, 1 ) );
		ASSERT_EQUAL( _T("789"), num::FormatDouble( 789.1234, 0 ) );
	}
}

void CNumericTests::TestFormatNumberUserLocale( void )
{
	const std::locale& userLoc = str::GetUserLocale();

	ASSERT_EQUAL( _T("4,096"), num::FormatNumber( (short)4096, userLoc ) );
	ASSERT_EQUAL( _T("-4,096"), num::FormatNumber( (short)-4096, userLoc ) );						// i.e. 0xF000
	ASSERT_EQUAL( _T("61,440"), num::FormatNumber( (unsigned short)61440, userLoc ) );				// i.e. 0xF000

	ASSERT_EQUAL( _T("-268,435,456"), num::FormatNumber( (int)-268435456, userLoc ) );				// i.e. 0xF0000000
	ASSERT_EQUAL( _T("4,026,531,840"), num::FormatNumber( (unsigned int)4026531840, userLoc ) );	// i.e. 0xF0000000

	ASSERT_EQUAL( _T("-268,435,456"), num::FormatNumber( (long)-268435456, userLoc ) );				// i.e. 0xF0000000

	ASSERT_EQUAL( _T("-1,152,921,504,606,846,976"), num::FormatNumber( (__int64)-1152921504606846976, userLoc ) );				// i.e. 0xF000000000000000
	ASSERT_EQUAL( _T("17,293,822,569,102,704,640"), num::FormatNumber( (unsigned __int64)17293822569102704640, userLoc ) );		// i.e. 0xF000000000000000

	ASSERT_EQUAL( _T("1,234,000.56789"), num::FormatNumber( 1234000.56789, userLoc ) );
	ASSERT_EQUAL( _T("-1,234,000.56789"), num::FormatNumber( -1234000.56789, userLoc ) );
}

namespace ut
{
	template< typename ValueType >
	std::pair<bool, ValueType> ParseNumber( const std::tstring& text, size_t* pSkipLength = nullptr, const std::locale& loc = num::GetEmptyLocale() )
	{
		std::pair<bool, ValueType> result;
		result.first = num::ParseNumber( result.second, text, pSkipLength, loc );
		return result;
	}
}

void CNumericTests::TestParseNumber( void )
{
	{
		ASSERT_EQUAL( std::make_pair( true, (signed char)32 ), ut::ParseNumber<signed char>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (signed char)-16 ), ut::ParseNumber<signed char>( _T("-16") ) );

		ASSERT_EQUAL( std::make_pair( true, (BYTE)32 ), ut::ParseNumber<BYTE>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (BYTE)240 ), ut::ParseNumber<BYTE>( _T("240") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (short)32 ), ut::ParseNumber<short>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (short)-4096 ), ut::ParseNumber<short>( _T("-4096") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned short)32 ), ut::ParseNumber<unsigned short>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned short)61440 ), ut::ParseNumber<unsigned short>( _T("61440") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (int)32 ), ut::ParseNumber<int>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (int)-268435456 ), ut::ParseNumber<int>( _T("-268435456") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned int)32 ), ut::ParseNumber<unsigned int>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned int)4026531840 ), ut::ParseNumber<unsigned int>( _T("4026531840") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (long)32 ), ut::ParseNumber<long>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (long)-268435456 ), ut::ParseNumber<long>( _T("-268435456") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned long)32 ), ut::ParseNumber<unsigned long>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned long)4026531840 ), ut::ParseNumber<unsigned long>( _T("4026531840") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (__int64)32 ), ut::ParseNumber<__int64>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (__int64)-1152921504606846976 ), ut::ParseNumber<__int64>( _T("-1152921504606846976") ) );

		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)32 ), ut::ParseNumber<unsigned __int64>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)17293822569102704640 ), ut::ParseNumber<unsigned __int64>( _T("17293822569102704640") ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, 32.0 ), ut::ParseNumber<double>( _T("32") ) );
		ASSERT_EQUAL( std::make_pair( true, 1234.56789 ), ut::ParseNumber<double>( _T("1234.56789") ) );
		ASSERT_EQUAL( std::make_pair( true, -1234.56789 ), ut::ParseNumber<double>( _T("-1234.56789") ) );
	}

	// with skip length
	{
		static const std::tstring numText = _T(" -375some");
		size_t skipLength;
		ASSERT_EQUAL( std::make_pair( true, (int)-375 ), ut::ParseNumber<int>( numText, &skipLength ) );
		ASSERT_EQUAL( 5, skipLength );
		ASSERT_EQUAL( _T("some"), numText.substr( skipLength ) );
	}
}

void CNumericTests::TestParseNumberUserLocale( void )
{
	const std::locale& userLoc = str::GetUserLocale();
	{
		ASSERT_EQUAL( std::make_pair( true, (short)-4096 ), ut::ParseNumber<short>( _T("-4,096"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned short)61440 ), ut::ParseNumber<unsigned short>( _T("61,440"), nullptr, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (int)32 ), ut::ParseNumber<int>( _T("32"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (int)-268435456 ), ut::ParseNumber<int>( _T("-268,435,456"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned int)4026531840 ), ut::ParseNumber<unsigned int>( _T("4,026,531,840"), nullptr, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (long)-268435456 ), ut::ParseNumber<long>( _T("-268,435,456"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned long)4026531840 ), ut::ParseNumber<unsigned long>( _T("4,026,531,840"), nullptr, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, (__int64)-1152921504606846976 ), ut::ParseNumber<__int64>( _T("-1,152,921,504,606,846,976"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, (unsigned __int64)17293822569102704640 ), ut::ParseNumber<unsigned __int64>( _T("17,293,822,569,102,704,640"), nullptr, userLoc ) );
	}
	{
		ASSERT_EQUAL( std::make_pair( true, 32.0 ), ut::ParseNumber<double>( _T("32"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, 1234000.56789 ), ut::ParseNumber<double>( _T("1,234,000.56789"), nullptr, userLoc ) );
		ASSERT_EQUAL( std::make_pair( true, -1234000.56789 ), ut::ParseNumber<double>( _T("-1,234,000.56789"), nullptr, userLoc ) );
	}
}

void CNumericTests::TestFindNumber( void )
{
	ASSERT_EQUAL( _T(""), num::FindNextNumber( _T("") ).Extract() );
	ASSERT_EQUAL( _T(""), num::FindNextNumber( _T("ab-cd") ).Extract() );
	ASSERT_EQUAL( _T(""), num::FindNextNumber( _T("0x") ).Extract() );
	ASSERT_EQUAL( _T(""), num::FindNextNumber( _T("0X") ).Extract() );
	ASSERT_EQUAL( _T(""), num::FindNextNumber( _T(" 0xGH") ).Extract() );

	ASSERT_EQUAL( _T("1"), num::FindNextNumber( _T(" 1 ") ).Extract() );
	ASSERT_EQUAL( _T("-30"), num::FindNextNumber( _T(" -30 ") ).Extract() );
	ASSERT_EQUAL( _T("-30"), num::FindNextNumber( _T(" a-30b ") ).Extract() );
	ASSERT_EQUAL( _T("321"), num::FindNextNumber( _T("321") ).Extract() );
	ASSERT_EQUAL( _T("321"), num::FindNextNumber( _T(" #321# ") ).Extract() );

	ASSERT_EQUAL( _T("0xDEADBEEF"), num::FindNextNumber( _T(" 0xDEADBEEF ") ).Extract() );
	ASSERT_EQUAL( _T("0Xdeadbeef"), num::FindNextNumber( _T(" 0Xdeadbeef ") ).Extract() );

	ASSERT_EQUAL( _T("0"), num::ExtractNumber( _T(" 0 ") ).FormatNextValue() );
	ASSERT_EQUAL( _T("987"), num::ExtractNumber( _T(" 987 ") ).FormatNextValue() );
	ASSERT_EQUAL( _T("DEADBEEF"), str::Format( _T("%X"), num::ExtractNumber( _T(" 0xDEADBEEF ") ).m_number ) );
}

void CNumericTests::TestGenerateNumericSequence( void )
{
	ASSERT_EQUAL( _T("file 10.txt|file 11.txt|file 12.txt"), ut::GenerateNumbersInSequence( _T("file 10.txt|file 20.txt|file 30.txt") ) );
	ASSERT_EQUAL( _T("file 15.txt|file 16.txt|file 17.txt"), ut::GenerateNumbersInSequence( _T("file 10.txt|file 20.txt|file 30.txt"), 15 ) );
	ASSERT_EQUAL( _T("file 0049.txt|file 0050.txt|file 0051.txt"), ut::GenerateNumbersInSequence( _T("file 0049.txt|file 2.txt|file 03.txt") ) );
}

void CNumericTests::TestConvertFileSize( void )
{
	ASSERT_EQUAL( std::make_pair( 0.0, num::Bytes ), num::ConvertFileSize( 0 ) );				// auto-conversion
	ASSERT_EQUAL( std::make_pair( 0.0, num::Bytes ), num::ConvertFileSize( 0, num::Bytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::KiloBytes ), num::ConvertFileSize( 0, num::KiloBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::MegaBytes ), num::ConvertFileSize( 0, num::MegaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::GigaBytes ), num::ConvertFileSize( 0, num::GigaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::TeraBytes ), num::ConvertFileSize( 0, num::TeraBytes ) );

	// 1 KB=1024 bytes
	ASSERT_EQUAL( std::make_pair( 1023.0, num::Bytes ), num::ConvertFileSize( 1023 ) );					// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1023.0, num::Bytes ), num::ConvertFileSize( 1023, num::Bytes ) );

	ASSERT_EQUAL( std::make_pair( 1.0, num::KiloBytes ), num::ConvertFileSize( 1024 ) );				// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1024.0, num::Bytes ), num::ConvertFileSize( 1024, num::Bytes ) );
	ASSERT_EQUAL( std::make_pair( 1.0, num::KiloBytes ), num::ConvertFileSize( 1024, num::KiloBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.001, num::MegaBytes ), num::ConvertFileSize( 1024, num::MegaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::GigaBytes ), num::ConvertFileSize( 1024, num::GigaBytes ) );

	// 1 MB=1048576 bytes (1024*1024)
	ASSERT_EQUAL( std::make_pair( 1023.9, num::KiloBytes ), num::ConvertFileSize( 1048575 ) );		// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1048575.0, num::Bytes ), num::ConvertFileSize( 1048575, num::Bytes ) );

	ASSERT_EQUAL( std::make_pair( 1.0, num::MegaBytes ), num::ConvertFileSize( 1048576 ) );				// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1048576.0, num::Bytes ), num::ConvertFileSize( 1048576, num::Bytes ) );
	ASSERT_EQUAL( std::make_pair( 1024.0, num::KiloBytes ), num::ConvertFileSize( 1048576, num::KiloBytes ) );
	ASSERT_EQUAL( std::make_pair( 1.0, num::MegaBytes ), num::ConvertFileSize( 1048576, num::MegaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.001, num::GigaBytes ), num::ConvertFileSize( 1048576, num::GigaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.0, num::TeraBytes ), num::ConvertFileSize( 1048576, num::TeraBytes ) );

	// 1 GB=1073741824 bytes (1024*1024*1024)
	ASSERT_EQUAL( std::make_pair( 1023.9, num::MegaBytes ), num::ConvertFileSize( 1073741823 ) );		// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1073741823.0, num::Bytes ), num::ConvertFileSize( 1073741823, num::Bytes ) );

	ASSERT_EQUAL( std::make_pair( 1.0, num::GigaBytes ), num::ConvertFileSize( 1073741824 ) );			// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1073741824.0, num::Bytes ), num::ConvertFileSize( 1073741824, num::Bytes ) );
	ASSERT_EQUAL( std::make_pair( 1048576.0, num::KiloBytes ), num::ConvertFileSize( 1073741824, num::KiloBytes ) );
	ASSERT_EQUAL( std::make_pair( 1024.0, num::MegaBytes ), num::ConvertFileSize( 1073741824, num::MegaBytes ) );
	ASSERT_EQUAL( std::make_pair( 1.0, num::GigaBytes ), num::ConvertFileSize( 1073741824, num::GigaBytes ) );
	ASSERT_EQUAL( std::make_pair( 0.001, num::TeraBytes ), num::ConvertFileSize( 1073741824, num::TeraBytes ) );

	// 1 TB=1099511627776 bytes (1024*1024*1024*1024)
	ASSERT_EQUAL( std::make_pair( 1023.9, num::GigaBytes ), num::ConvertFileSize( 1099511627775 ) );	// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1099511627775.0, num::Bytes ), num::ConvertFileSize( 1099511627775, num::Bytes ) );

	ASSERT_EQUAL( std::make_pair( 1.0, num::TeraBytes ), num::ConvertFileSize( 1099511627776 ) );		// auto-conversion
	ASSERT_EQUAL( std::make_pair( 1099511627776.0, num::Bytes ), num::ConvertFileSize( 1099511627776, num::Bytes ) );
	ASSERT_EQUAL( std::make_pair( 1073741824.0, num::KiloBytes ), num::ConvertFileSize( 1099511627776, num::KiloBytes ) );
	ASSERT_EQUAL( std::make_pair( 1048576.0, num::MegaBytes ), num::ConvertFileSize( 1099511627776, num::MegaBytes ) );
	ASSERT_EQUAL( std::make_pair( 1024.0, num::GigaBytes ), num::ConvertFileSize( 1099511627776, num::GigaBytes ) );
	ASSERT_EQUAL( std::make_pair( 1.0, num::TeraBytes ), num::ConvertFileSize( 1099511627776, num::TeraBytes ) );
}

void CNumericTests::TestFormatFileSize( void )
{
	ASSERT_EQUAL( _T("0 bytes"), num::FormatFileSize( 0 ) );						// auto-conversion
	ASSERT_EQUAL( _T("0 bytes"), num::FormatFileSize( 0, num::Bytes ) );

	// 1 KB=1024 bytes
	ASSERT_EQUAL( _T("1,023 bytes"), num::FormatFileSize( 1023 ) );					// auto-conversion
	ASSERT_EQUAL( _T("1,023 bytes"), num::FormatFileSize( 1023, num::Bytes ) );

	ASSERT_EQUAL( _T("1 KB"), num::FormatFileSize( 1024 ) );					// auto-conversion
	ASSERT_EQUAL( _T("1,024 bytes"), num::FormatFileSize( 1024, num::Bytes ) );
	ASSERT_EQUAL( _T("1 KB"), num::FormatFileSize( 1024, num::KiloBytes ) );
	ASSERT_EQUAL( _T("0.001 MB"), num::FormatFileSize( 1024, num::MegaBytes ) );
	ASSERT_EQUAL( _T("0 GB"), num::FormatFileSize( 1024, num::GigaBytes ) );
	ASSERT_EQUAL( _T("0 TB"), num::FormatFileSize( 1024, num::TeraBytes ) );

	// 1 MB=1048576 bytes (1024*1024)
	ASSERT_EQUAL( _T("1,023.9 KB"), num::FormatFileSize( 1048575 ) );			// auto-conversion
	ASSERT_EQUAL( _T("1,048,575 bytes"), num::FormatFileSize( 1048575, num::Bytes ) );

	ASSERT_EQUAL( _T("1 MB"), num::FormatFileSize( 1048576 ) );					// auto-conversion
	ASSERT_EQUAL( _T("1,048,576 bytes"), num::FormatFileSize( 1048576, num::Bytes ) );
	ASSERT_EQUAL( _T("1,024 KB"), num::FormatFileSize( 1048576, num::KiloBytes ) );
	ASSERT_EQUAL( _T("1 MB"), num::FormatFileSize( 1048576, num::MegaBytes ) );
	ASSERT_EQUAL( _T("0.001 GB"), num::FormatFileSize( 1048576, num::GigaBytes ) );
	ASSERT_EQUAL( _T("0 TB"), num::FormatFileSize( 1048576, num::TeraBytes ) );

	// 1 GB=1073741824 bytes (1024*1024*1024)
	ASSERT_EQUAL( _T("1,023.9 MB"), num::FormatFileSize( 1073741823 ) );		// auto-conversion
	ASSERT_EQUAL( _T("1,073,741,823 bytes"), num::FormatFileSize( 1073741823, num::Bytes ) );

	ASSERT_EQUAL( _T("1 GB"), num::FormatFileSize( 1073741824 ) );				// auto-conversion
	ASSERT_EQUAL( _T("1,073,741,824 bytes"), num::FormatFileSize( 1073741824, num::Bytes ) );
	ASSERT_EQUAL( _T("1,048,576 KB"), num::FormatFileSize( 1073741824, num::KiloBytes ) );
	ASSERT_EQUAL( _T("1,024 MB"), num::FormatFileSize( 1073741824, num::MegaBytes ) );
	ASSERT_EQUAL( _T("1 GB"), num::FormatFileSize( 1073741824, num::GigaBytes ) );
	ASSERT_EQUAL( _T("0.001 TB"), num::FormatFileSize( 1073741824, num::TeraBytes ) );

	// 1 TB=1099511627776 bytes (1024*1024*1024*1024)
	ASSERT_EQUAL( _T("1,023.9 GB"), num::FormatFileSize( 1099511627775 ) );	// auto-conversion
	ASSERT_EQUAL( _T("1,099,511,627,775 bytes"), num::FormatFileSize( 1099511627775, num::Bytes ) );

	ASSERT_EQUAL( _T("1 TB"), num::FormatFileSize( 1099511627776 ) );	// auto-conversion
	ASSERT_EQUAL( _T("1,099,511,627,776 bytes"), num::FormatFileSize( 1099511627776, num::Bytes ) );
	ASSERT_EQUAL( _T("1,073,741,824 KB"), num::FormatFileSize( 1099511627776, num::KiloBytes ) );
	ASSERT_EQUAL( _T("1,048,576 MB"), num::FormatFileSize( 1099511627776, num::MegaBytes ) );
	ASSERT_EQUAL( _T("1,024 GB"), num::FormatFileSize( 1099511627776, num::GigaBytes ) );
	ASSERT_EQUAL( _T("1 TB"), num::FormatFileSize( 1099511627776, num::TeraBytes ) );

	// misc. sizes with auto-conversion
	ASSERT_EQUAL( _T("37 bytes"), num::FormatFileSize( 37 ) );
	ASSERT_EQUAL( _T("37 bytes"), num::FormatFileSize( 37, num::AutoBytes, true ) );		// long unit

	ASSERT_EQUAL( _T("1.12 KB"), num::FormatFileSize( 1147 ) );				//	Explorer: "1.12 KB (1,147 bytes)"
	ASSERT_EQUAL( _T("4.25 KB"), num::FormatFileSize( 4357 ) );				//	Explorer: "4.25 KB (4,357 bytes)"
	ASSERT_EQUAL( _T("41.8 KB"), num::FormatFileSize( 42765 ) );			//	Explorer: "41.7 KB (42,765 bytes)"				*differs
	ASSERT_EQUAL( _T("110 KB"),  num::FormatFileSize( 113497 ) );			//	Explorer: "110 KB (113,497 bytes)"
	ASSERT_EQUAL( _T("1.58 MB"), num::FormatFileSize( 1661001 ) );			//	Explorer: "1.58 MB (1,661,001 bytes)"
	ASSERT_EQUAL( _T("2.53 MB"), num::FormatFileSize( 2649147 ) );			//	Explorer: "2.52 MB (2,649,147 bytes)"			*differs
	ASSERT_EQUAL( _T("119 MB"),  num::FormatFileSize( 125217369 ) );		//	Explorer: "119 MB (125,217,369 bytes)"
	ASSERT_EQUAL( _T("2.34 GB"), num::FormatFileSize( 2508506361 ) );		//	Explorer: "2.33 GB (2,508,506,361 bytes)"		*differs
	ASSERT_EQUAL( _T("6.54 GB"), num::FormatFileSize( 7018582830 ) );		//	Explorer: "6.53 GB (7,018,582,830 bytes)"		*differs
	ASSERT_EQUAL( _T("461 GB"),  num::FormatFileSize( 495761485824 ) );		//	Explorer: "461 GB (495,761,485,824 bytes)"
	ASSERT_EQUAL( _T("3.64 TB"), num::FormatFileSize( 4000617312256 ) );	//	Explorer: "3.63 TB (4,000,617,312,256 bytes)"	*differs
}

void CNumericTests::TestCrc32( void )
{
	ASSERT( GetReferenceCrc32Table() == utl::CCrc32::Instance().GetLookupTable() );

	ASSERT_EQUAL( 0, crc32::ComputeStringChecksum( (const char*)nullptr ) );
	{
		static const BYTE bytes[] = { 0 };
		ASSERT_EQUAL( 0xD202EF8D, crc32::ComputeChecksum( bytes, COUNT_OF( bytes ) ) );
	}

	static const UINT s_crc32_abc = 0x352441C2;
	{
		static const BYTE bytes[] = { 'a', 'b', 'c' };
		ASSERT_EQUAL( s_crc32_abc, crc32::ComputeChecksum( bytes, COUNT_OF( bytes ) ) );
	}

	ASSERT_EQUAL( s_crc32_abc, crc32::ComputeStringChecksum( "abc" ) );

	ASSERT_EQUAL( 0xAD957AB0, crc32::ComputeStringChecksum( L"abc" ) );		// wide string

	ASSERT_EQUAL( 0, crc32::ComputeFileChecksum( fs::CPath( _T("NoFile.xyz") ) ) );		// non-existing file CRC32
	ASSERT( crc32::ComputeFileChecksum( app::GetModulePath() ) != 0 );					// exe file CRC32

	const fs::TDirPath& imagesDirPath = ut::GetDestImagesDirPath();
	if ( !imagesDirPath.IsEmpty() )
	{
		fs::CPath gifPath = imagesDirPath / _T("Animated.gif");
		if ( gifPath.FileExist() )
			ASSERT_EQUAL( 0xA524D308, crc32::ComputeFileChecksum( gifPath ) );	// gif file CRC32
	}
}

void CNumericTests::TestMemLeakCheck( void )
{
	double* pNumber;
	{
		MEM_LEAK_START( testMemLeakCheck );
		MEM_LEAK_CHECK( testMemLeakCheck );
		ASSERT_NO_LEAKS( testMemLeakCheck );

		pNumber = new double();

		MEM_LEAK_CHECK( testMemLeakCheck );
		//ASSERT_NO_LEAKS( testMemLeakCheck );
	}
	delete pNumber;
}


void CNumericTests::Run( void )
{
	RUN_TEST( TestFormatNumber );
	RUN_TEST( TestFormatNumberUserLocale );
	RUN_TEST( TestParseNumber );
	RUN_TEST( TestParseNumberUserLocale );
	RUN_TEST( TestFindNumber );
	RUN_TEST( TestGenerateNumericSequence );

	RUN_TEST( TestConvertFileSize );
	RUN_TEST( TestFormatFileSize );
	RUN_TEST( TestCrc32 );

#if _MSC_VER < 1800		// MSVC++ 12.0 (Visual Studio 2013)
	// there are issues with CMemLeakCheck class on newer VC++
	RUN_TEST( TestMemLeakCheck );
#endif	//_MSC_VER
}


#endif //USE_UT
