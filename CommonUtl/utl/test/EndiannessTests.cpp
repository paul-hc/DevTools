
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/EndiannessTests.h"
#include "StringUtilities.h"
#include "utl/AppTools.h"
#include "utl/Endianness.h"
#include "utl/TextEncoding.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CEndiannessTests::CEndiannessTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CEndiannessTests& CEndiannessTests::Instance( void )
{
	static CEndiannessTests s_testCase;
	return s_testCase;
}

void CEndiannessTests::TestSwapBytesValues( void )
{
	using namespace endian;

	{	// no byte swapping
		const char value = 'A';
		char swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT_EQUAL( swapped, value );

		char swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}

	{
		const wchar_t value = L'A';
		ASSERT_EQUAL( L'\x0041', value );
		wchar_t swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT_EQUAL( L'\x4100', swapped );

		wchar_t swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}

	{
		const int value = 0x2211;
		int swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT_EQUAL( 0x11220000, swapped );

		int swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}

	{
		const long long value = 0x1234ll;
		long long swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT_EQUAL( 0x3412000000000000ll, swapped );

		long long swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}

	{
		const float value = 1.23f;
		float swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT( swapped != value );

		float swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}

	{
		const double value = 7.43;
		double swapped = GetBytesSwapped<Little, Big>()( value );
		ASSERT( swapped != value );

		double swappedBack = GetBytesSwapped<Big, Little>()( swapped );
		ASSERT_EQUAL( value, swappedBack );
	}
}

void CEndiannessTests::TestSwapBytesString( void )
{
	using namespace endian;

	// wchar_t (UTF16)
	{
		const std::wstring value = L"ABCD";
		ASSERT_EQUAL( L"\x0041\x0042\x0043\x0044", value );

		std::wstring swappedText = value;
		std::for_each( swappedText.begin(), swappedText.end(), func::SwapBytes<Little, Big>() );
		ASSERT_EQUAL( L"\x4100\x4200\x4300\x4400", swappedText );

		endian::SwapBytes<Big, Little>( swappedText );		// swap back
		ASSERT_EQUAL( value, swappedText );
	}

	// char32_t (UTF32)
	{
		const str::wstring4 value = str::FromWide( L"ABCD" );			// U"ABCD" - VC9 doesn't define char32_t and the U"..." literals
		ASSERT_EQUAL( 0x00000041, value[ 0 ] );
		ASSERT_EQUAL( 0x00000042, value[ 1 ] );
		ASSERT_EQUAL( 0x00000043, value[ 2 ] );
		ASSERT_EQUAL( 0x00000044, value[ 3 ] );

		str::wstring4 swappedText = value;
		endian::SwapBytes<Little, Big>( swappedText );		// swap back
		ASSERT_EQUAL( 0x41000000, swappedText[ 0 ] );
		ASSERT_EQUAL( 0x42000000, swappedText[ 1 ] );
		ASSERT_EQUAL( 0x43000000, swappedText[ 2 ] );
		ASSERT_EQUAL( 0x44000000, swappedText[ 3 ] );

		std::for_each( swappedText.begin(), swappedText.end(), func::SwapBytes<Big, Little>() );
		ASSERT_EQUAL( value, swappedText );
	}
}

void CEndiannessTests::TestSwapBytesContainer( void )
{
	using namespace endian;

	const WORD wordsArray[] = { 0xAABB, 0xCCDD, 0xEEFF };
	const std::vector< WORD > words( wordsArray, wordsArray + COUNT_OF( wordsArray ) );

	std::vector< WORD > swappedWords = words;
	std::for_each( swappedWords.begin(), swappedWords.end(), func::SwapBytes<Little, Big>() );
	ASSERT_EQUAL( 0xBBAA, swappedWords[ 0 ] );
	ASSERT_EQUAL( 0xDDCC, swappedWords[ 1 ] );
	ASSERT_EQUAL( 0xFFEE, swappedWords[ 2 ] );

	std::for_each( swappedWords.begin(), swappedWords.end(), func::SwapBytes<Big, Little>() );
	ASSERT( words == swappedWords );
}

void CEndiannessTests::TestSwapBytesSameEndianness( void )
{
	using namespace endian;

	// same endianness, no swapping
	wchar_t wc = GetBytesSwapped<Little, Little>()( L'A' );
	ASSERT_EQUAL( L'A', wc );

	UINT ui = GetBytesSwapped<Little, Little>()( 0x2211u );
	ASSERT_EQUAL( 0x2211u, ui );

	ULONGLONG ui64 = GetBytesSwapped<Big, Big>()( (ULONGLONG)0x2211ull );
	ASSERT_EQUAL( 0x2211ull, ui64 );

	float f = GetBytesSwapped<Big, Big>()( 1.23f );
	ASSERT_EQUAL( 1.23f, f );

	double d = GetBytesSwapped<Big, Big>()( 7.43 );
	ASSERT_EQUAL( 7.43, d );


	const std::wstring value = L"ABCD";

	std::wstring swappedText = value;
	std::for_each( swappedText.begin(), swappedText.end(), func::SwapBytes<Little, Little>() );
	ASSERT_EQUAL( value, swappedText );
}


void CEndiannessTests::Run( void )
{
	__super::Run();

	TestSwapBytesValues();
	TestSwapBytesString();
	TestSwapBytesContainer();
	TestSwapBytesSameEndianness();
}


#endif //USE_UT
