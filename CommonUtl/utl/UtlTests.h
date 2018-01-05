#ifndef UtlTests_h
#define UtlTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CUtlTests : public ut::ITestCase
{
	CUtlTests( void );
public:
	static CUtlTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestStringUtilities( void );
	void TestArgUtilities( void );
	void TestEnumTags( void );
	void TestFlagTags( void );
	void TestExpandKeysToValues( void );
	void TestWordSelection( void );
	void TestEnsureUniformNumPadding( void );

	void TestNestedLocking( void );			// OS sync objects
	void TestFunctional( void );			// <functional>
};


#endif //_DEBUG


#endif // UtlTests_h
