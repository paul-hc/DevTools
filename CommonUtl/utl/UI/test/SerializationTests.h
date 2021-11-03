#ifndef SerializationTests_h
#define SerializationTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


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


#endif //USE_UT


#endif // SerializationTests_h
