#ifndef CommandModelSerializerTests_h
#define CommandModelSerializerTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


class CCommandModelTextSerializer;


class CCommandModelSerializerTests : public ut::CConsoleTestCase
{
	CCommandModelSerializerTests( void );
public:
	static CCommandModelSerializerTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestLoadLog( void );
	void TestSaveLog( void );
};


#endif //_DEBUG


#endif // CommandModelSerializerTests_h
