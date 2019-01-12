
#include "stdafx.h"
#include "ut/RegistryKeyTests.h"
#include "Registry.h"
#include "MultiThreading.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"
#include <shobjidl.h>				// for IShellFolder

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
}


CRegistryKeyTests::CRegistryKeyTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CRegistryKeyTests& CRegistryKeyTests::Instance( void )
{
	static CRegistryKeyTests testCase;
	return testCase;
}

void CRegistryKeyTests::TestKey( void )
{
	const fs::CPath s_rootPath( _T("Software\\Paul Cocoveanu") );
	const fs::CPath s_test( _T("UTL-Test") );

	{	// set-up
		reg::CKey key;
		ASSERT( key.Create( HKEY_CURRENT_USER, s_rootPath / s_test ) );
		key.DeleteAll();
		ASSERT( key.IsOpen() );
		ASSERT_PTR( key.Get() );

		Test_StringValue( key );
		Test_MultiStringValue( key );
		Test_NumericValues( key );
		Test_GuidValue( key );
		Test_BinaryValue( key );
		Test_BinaryBuffer( key );

		Test_SubKeys( key );
		Test_KeyInfo( key );
	}

	{	// tear-down
		reg::CKey rootKey;
		ASSERT( rootKey.Open( HKEY_CURRENT_USER, s_rootPath ) );

		std::vector< std::tstring > subKeyNames;
		rootKey.QuerySubKeyNames( subKeyNames );
		ASSERT( utl::FindPos( subKeyNames, s_test.Get() ) != utl::npos );

		ASSERT( !rootKey.DeleteSubKey( s_test.GetPtr(), Shallow ) );				// has a sub-key (locked)
		ASSERT( rootKey.DeleteSubKey( s_test.GetPtr(), Deep ) );					// recurse delete

		rootKey.Close();
		ENSURE( !rootKey.IsOpen() );
	}
}


void CRegistryKeyTests::Test_StringValue( reg::CKey& rKey )
{
	ASSERT( !rKey.HasValue( _T("StrName") ) );
	ASSERT_EQUAL( _T(""), rKey.ReadStringValue( _T("StrName") ) );
	ASSERT_EQUAL( _T("blah"), rKey.ReadStringValue( _T("StrName"), _T("blah") ) );

	ASSERT( rKey.WriteStringValue( _T("StrName"), _T("text") ) );
	ASSERT_EQUAL( _T("text"), rKey.ReadStringValue( _T("StrName") ) );
	ASSERT_EQUAL( _T("text"), rKey.ReadStringValue( _T("StrName"), _T("blah") ) );

	ASSERT( rKey.HasValue( _T("StrName") ) );
	ASSERT_EQUAL( REG_SZ, rKey.GetValueType( _T("StrName") ) );
	ASSERT_EQUAL( ( 4 + 1 ) * 2, rKey.GetValueBufferSize( _T("StrName") ) );
}

void CRegistryKeyTests::Test_MultiStringValue( reg::CKey& rKey )
{
	std::vector< fs::CPath > paths;
	ASSERT( !rKey.HasValue( _T("MultiPaths") ) );
	ASSERT( !rKey.ReadMultiString( _T("MultiPaths"), paths ) );
	ASSERT( paths.empty() );

	str::Split( paths, _T("C:\\My\\file.txt|C:\\My\\Image\\img.jpg"), _T("|") );
	ASSERT( rKey.WriteMultiString( _T("MultiPaths"), paths ) );

	paths.clear();
	ASSERT( rKey.ReadMultiString( _T("MultiPaths"), paths ) );
	ASSERT_EQUAL( _T("C:\\My\\file.txt|C:\\My\\Image\\img.jpg"), str::Join( paths, _T("|") ) );
	ASSERT( rKey.HasValue( _T("MultiPaths") ) );
	ASSERT_EQUAL( REG_MULTI_SZ, rKey.GetValueType( _T("MultiPaths") ) );
}

void CRegistryKeyTests::Test_NumericValues( reg::CKey& rKey )
{
	ASSERT_EQUAL( -1, rKey.ReadNumberValue( _T("Int16"), static_cast< short >( -1 ) ) );
	ASSERT( rKey.WriteNumberValue( _T("Int16"), static_cast< short >( 55 ) ) );
	ASSERT_EQUAL( 55, rKey.ReadNumberValue( _T("Int16"), static_cast< short >( -1 ) ) );
	ASSERT_EQUAL( REG_MULTI_SZ, rKey.GetValueType( _T("MultiPaths") ) );

	ASSERT_EQUAL( -1, rKey.ReadNumberValue( _T("Int32"), static_cast< int >( -1 ) ) );
	ASSERT( rKey.WriteNumberValue( _T("Int32"), static_cast< int >( 55 ) ) );
	ASSERT_EQUAL( 55, rKey.ReadNumberValue( _T("Int32"), static_cast< int >( -1 ) ) );

	ASSERT_EQUAL( 1000, rKey.ReadNumberValue( _T("UInt64"), static_cast< ULONGLONG >( 1000 ) ) );
	ASSERT( rKey.WriteNumberValue( _T("UInt64"), static_cast< ULONGLONG >( 55 ) ) );
	ASSERT_EQUAL( 55, rKey.ReadNumberValue( _T("UInt64"), static_cast< ULONGLONG >( 1000 ) ) );
}

void CRegistryKeyTests::Test_GuidValue( reg::CKey& rKey )
{
	mt::CScopedInitializeCom scopedCom;
	GUID itfID = IID_IShellFolder;

	ASSERT( rKey.WriteGuidValue( _T("InterfaceID"), itfID ) );

	itfID = IID_IUnknown;
	ASSERT( rKey.ReadGuidValue( _T("InterfaceID"), itfID ) );
	ASSERT( ::IsEqualGUID( IID_IShellFolder, itfID ) );

	ASSERT( rKey.HasValue( _T("InterfaceID") ) );
	ASSERT_EQUAL( REG_SZ, rKey.GetValueType( _T("InterfaceID") ) );
}

void CRegistryKeyTests::Test_BinaryValue( reg::CKey& rKey )
{
	CRect rect( -10, -20, 33, 45 );

	ASSERT( rKey.WriteBinaryValue( _T("Rect"), rect ) );

	rect.SetRectEmpty();
	ASSERT( rKey.ReadBinaryValue( _T("Rect"), &rect ) );
	ASSERT( rKey.HasValue( _T("Rect") ) );
	ASSERT_EQUAL( REG_BINARY, rKey.GetValueType( _T("Rect") ) );
	ASSERT_EQUAL( sizeof( CRect ), rKey.GetValueBufferSize( _T("Rect") ) );
}

void CRegistryKeyTests::Test_BinaryBuffer( reg::CKey& rKey )
{
	std::vector< double > numbers;
	numbers.push_back( 123.4567 );
	numbers.push_back( 357.98 );

	ASSERT( rKey.WriteBinaryBuffer( _T("Doubles"), numbers ) );

	std::vector< double > persistNumbers;
	ASSERT( rKey.ReadBinaryBuffer( _T("Doubles"), persistNumbers ) );
	ASSERT( numbers == persistNumbers );
	ASSERT_EQUAL( REG_BINARY, rKey.GetValueType( _T("Doubles") ) );
	ASSERT_EQUAL( sizeof( double ) * 2, rKey.GetValueBufferSize( _T("Rect") ) );
}

void CRegistryKeyTests::Test_SubKeys( reg::CKey& rKey )
{
	reg::CKey subKey;
	ASSERT( subKey.Create( rKey.Get(), fs::CPath( _T("Details") ) ) );
	ASSERT_PTR( subKey.Get() );
	ASSERT( rKey.HasSubKey( _T("Details") ) );

	// move assignment
	reg::CKey destSubKey = subKey;
	ASSERT( !subKey.IsOpen() );
	ASSERT( destSubKey.IsOpen() );
}

void CRegistryKeyTests::Test_KeyInfo( const reg::CKey& rKey )
{
	reg::CKeyInfo keyInfo( rKey );
	ASSERT_EQUAL( 1, keyInfo.m_subKeyCount );
	ASSERT_EQUAL( 8, keyInfo.m_valueCount );
	ASSERT( CTime::GetCurrentTime() == keyInfo.m_lastWriteTime );

	std::vector< std::tstring > subKeyNames;
	rKey.QuerySubKeyNames( subKeyNames );
	ASSERT_EQUAL( _T("Details"), str::Join( subKeyNames, _T("|") ) );

	std::vector< std::tstring > valueNames;
	rKey.QueryValueNames( valueNames );
	ASSERT_EQUAL( _T("StrName|MultiPaths|Int16|Int32|UInt64|InterfaceID|Rect|Doubles"), str::Join( valueNames, _T("|") ) );
}


void CRegistryKeyTests::Run( void )
{
	__super::Run();

	TestKey();
}


#endif //_DEBUG
