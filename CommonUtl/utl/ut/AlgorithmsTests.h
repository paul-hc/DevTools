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
	void TestBuffer( void );
	void TestLookup( void );
	void TestBinaryLookup( void );
	void TestIsOrdered( void );
	void TestQuery( void );
	void TestAssignment( void );
	void TestInsert( void );
	void TestRemove( void );
	void TestMixedTypes( void );
	void TestCompareContents( void );
	void TestAdvancePos( void );
	void TestOwningContainer( void );
	void Test_vector_map( void );
private:
	static const TCHAR s_sep[];
};


#endif //_DEBUG


#endif // AlgorithmsTests_h
