#ifndef SerializationTests_h
#define SerializationTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CSerializationTests : public ut::CConsoleTestCase
{
	CSerializationTests( void );
public:
	static CSerializationTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestUnicodeString( void );
};


#endif //_DEBUG


#endif // SerializationTests_h
