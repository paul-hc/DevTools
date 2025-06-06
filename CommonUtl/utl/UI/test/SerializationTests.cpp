
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/SerializationTests.h"
#include "MfcUtilities.h"
#include "Serialization.h"
#include "ScopedValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Serialization.hxx"


namespace ut
{
	template< typename ValueT >
	class CValueSerializer : private serial::IStreamable
	{
	public:
		CValueSerializer( void ) {}

		bool Save( const fs::CPath& filePath )
		{
			ui::CAdapterDocument doc( this, filePath.Get() );
			return doc.Save();
		}

		bool Load( const fs::CPath& filePath )
		{
			ui::CAdapterDocument doc( this, filePath.Get() );
			return doc.Load();
		}

		// serial::IStreamable interface
		virtual void Save( CArchive& archive ) throws_( CException* )
		{
			archive << m_value;
		}

		virtual void Load( CArchive& archive ) throws_( CException* )
		{
			archive >> m_value;
		}
	public:
		ValueT m_value;
	};


	std::wstring SaveAndLoadAs( const std::wstring& wideStr, serial::UnicodeEncoding encoding, const fs::CPath& filePath )
	{
		CScopedValue<serial::UnicodeEncoding> scopedEncoding( &serial::CPolicy::s_strEncoding, encoding );

		CValueSerializer<std::wstring> stringSerializer;
		stringSerializer.m_value = wideStr;
		stringSerializer.Save( filePath );

		stringSerializer.m_value.clear();
		stringSerializer.Load( filePath );
		return stringSerializer.m_value;
	}
}


CSerializationTests::CSerializationTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CSerializationTests& CSerializationTests::Instance( void )
{
	static CSerializationTests s_testCase;
	return s_testCase;
}

void CSerializationTests::TestUnicodeString( void )
{
	static const std::wstring s_wideStr = L"abc Σὲ γνωρίζω ἀπὸ τὴν κόψη xyz";			// Greek anthem (Unicode) - L"\x03a3\x1f72 \x03b3\x03bd\x03c9\x03c1\x1f77\x03b6\x03c9"

	ut::CTempFilePool tempPool( _T("SerialWide.dat|SerialUtf8.dat") );

	{
		const fs::CPath& widePath = tempPool.GetFilePaths()[ 0 ];

		std::wstring destStr = ut::SaveAndLoadAs( s_wideStr, serial::WideEncoding, widePath );
		ASSERT_EQUAL( s_wideStr, destStr );
	}

	{
		const fs::CPath& utf8Path = tempPool.GetFilePaths()[ 1 ];

		std::wstring destStr = ut::SaveAndLoadAs( s_wideStr, serial::Utf8Encoding, utf8Path );
		ASSERT_EQUAL( s_wideStr, destStr );
	}
}


void CSerializationTests::Run( void )
{
	RUN_TEST( TestUnicodeString );
}


#endif //USE_UT
