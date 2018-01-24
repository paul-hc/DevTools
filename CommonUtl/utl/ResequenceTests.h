#ifndef ResequenceTests_h
#define ResequenceTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CResequenceTests : public ut::CConsoleTestCase
{
	CResequenceTests( void );
public:
	static CResequenceTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestCanMove( void );
	void TestResequence( void );
	void TestDropMove( void );
};


#endif //_DEBUG


#endif // ResequenceTests_h
