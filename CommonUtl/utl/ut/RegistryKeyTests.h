#ifndef RegistryKeyTests_h
#define RegistryKeyTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


namespace reg { class CKey; }


class CRegistryKeyTests : public ut::CConsoleTestCase
{
	CRegistryKeyTests( void );
public:
	static CRegistryKeyTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
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


#endif //_DEBUG


#endif // RegistryKeyTests_h
