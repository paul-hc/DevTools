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
	void TestStringSplit( void );
	void TestStringTokenize( void );
	void TestStringConversion( void );
	void TestStringSearch( void );
	void TestArgUtilities( void );
	void TestEnumTags( void );
	void TestFlagTags( void );
	void TestExpandKeysToValues( void );
	void TestWordSelection( void );
	void TestEnsureUniformNumPadding( void );
	void Test_vector_map( void );

	void TestNestedLocking( void );			// OS sync objects
	void TestFunctional( void );			// <functional>
};


#endif //_DEBUG


#endif // UtlTests_h
