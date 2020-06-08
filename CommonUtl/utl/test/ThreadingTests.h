#ifndef ThreadingTests_h
#define ThreadingTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CThreadingTests : public ut::CConsoleTestCase
{
	CThreadingTests( void );
public:
	static CThreadingTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestNestedLocking( void );			// OS sync objects
};


#endif //_DEBUG


#endif // ThreadingTests_h
