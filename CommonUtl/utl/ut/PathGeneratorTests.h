#ifndef PathGeneratorTests_h
#define PathGeneratorTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CPathGeneratorTests : public ut::CConsoleTestCase
{
	CPathGeneratorTests( void );
public:
	static CPathGeneratorTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPathMaker( void );
	void TestPathFormatter( void );
	void TestNumSeqGeneration( void );
	void TestNumSeqFileGeneration( void );
	void TestFindNextAvailSeqCount( void );
	void TestWildcardGeneration( void );
	void TestWildcardFileGeneration( void );
};


#endif //_DEBUG


#endif // PathGeneratorTests_h
