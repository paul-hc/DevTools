#ifndef StringSequenceTests_h
#define StringSequenceTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CStringSequenceTests : public ut::ITestCase
{
	CStringSequenceTests( void );
public:
	static CStringSequenceTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestCompareSimpleLCS( void );
	void TestCompareDiffCaseLCS( void );
	void TestCompareMidSubseqLCS( void );
	void TestFindLongestDuplicatedString( void );
	void TestFindLongestDuplicatedMultiSource( void );
};


#endif //_DEBUG


#endif // StringSequenceTests_h
