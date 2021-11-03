#ifndef RegistryTests_h
#define RegistryTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


namespace reg { class CKey; }


class CRegistryTests : public ut::CConsoleTestCase
{
	CRegistryTests( void );
public:
	static CRegistryTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestParseKeyFullPath( void );
	void TestKey( void );

	void Test_StringValue( reg::CKey& rKey );
	void Test_MultiStringValue( reg::CKey& rKey );
	void Test_NumericValues( reg::CKey& rKey );
	void Test_GuidValue( reg::CKey& rKey );
	void Test_BinaryValue( reg::CKey& rKey );
	void Test_BinaryBuffer( reg::CKey& rKey );
	void Test_SubKeys( reg::CKey& rKey );
	void Test_KeyInfo( const reg::CKey& rKey );
};


#endif //USE_UT


#endif // RegistryTests_h
