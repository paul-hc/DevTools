#ifndef AlgorithmsTests_h
#define AlgorithmsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CAlgorithmsTests : public ut::CConsoleTestCase
{
	CAlgorithmsTests( void );
public:
	static CAlgorithmsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestIsOrdered( void );
};


#endif //_DEBUG


#endif // AlgorithmsTests_h
