#ifndef BatchTransactionTests_h
#define BatchTransactionTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CBatchTransactionTests : public ut::CConsoleTestCase
{
	CBatchTransactionTests( void );
public:
	static CBatchTransactionTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void Test__( void );
};


#endif //_DEBUG


#endif // BatchTransactionTests_h
