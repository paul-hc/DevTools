#ifndef PathGeneratorTests_h
#define PathGeneratorTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CPathGeneratorTests : public ut::CConsoleTestCase
{
	CPathGeneratorTests( void );
public:
	static CPathGeneratorTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPathUniqueMaker( void );
	void TestPathMaker( void );
	void TestPathFormatter( void );
	void TestNumSeqGeneration( void );
	void TestNumSeqFileGeneration( void );
	void TestFindNextAvailSeqCount( void );
	void TestWildcardGeneration( void );
	void TestWildcardFileGeneration( void );
};


#endif //USE_UT


#endif // PathGeneratorTests_h
