#ifndef EndiannessTests_h
#define EndiannessTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CEndiannessTests : public ut::CConsoleTestCase
{
	CEndiannessTests( void );
public:
	static CEndiannessTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestSwapBytesValues( void );
	void TestSwapBytesString( void );
	void TestSwapBytesContainer( void );
	void TestSwapBytesSameEndianness( void );
};


#endif //_DEBUG


#endif // EndiannessTests_h
