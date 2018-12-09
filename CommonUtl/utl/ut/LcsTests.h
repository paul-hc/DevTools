#ifndef LcsTests_h
#define LcsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CLcsTests : public ut::CConsoleTestCase
{
	CLcsTests( void );
	~CLcsTests();
public:
	static CLcsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	// common substring
	void TestFindLongestDuplicatedString( void );		// single string multiple occurence
	void TestFindLongestCommonSubstring( void );		// string set common occurence
	void TestRandomLongestCommonSubstring( void );

	// matching sequence
	void TestMatchingSequenceSimple( void );
	void TestMatchingSequenceDiffCase( void );
	void TestMatchingSequenceMidCommon( void );
};


#endif //_DEBUG


#endif // LcsTests_h
