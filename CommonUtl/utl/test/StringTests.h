#ifndef StringTests_h
#define StringTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CStringTests : public ut::CConsoleTestCase
{
	CStringTests( void );
public:
	static CStringTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestCharTraits( void );
	void TestValueToString( void );
	void TestStringSorting( void );
	void TestIntuitiveSort( void );
	void TestIntuitiveSortPunctuation( void );
	void TestIgnoreCase( void );
	void TestTrim( void );
	void TestEnquote( void );
	void TestStringSplit( void );
	void TestStringTokenize( void );
	void TestStringPrefixSuffix( void );
	void TestStringConversion( void );
	void TestStringSearch( void );
	void TestStringMatch( void );
	void TestStringPart( void );
	void TestStringOccurenceCount( void );
	void TestStringLines( void );
	void TestArgUtilities( void );
	void TestEnumTags( void );
	void TestFlagTags( void );
	void TestExpandKeysToValues( void );
	void TestWordSelection( void );
	void TestEnsureUniformNumPadding( void );
	void TestTimeFormatting( void );

	void TestFunctional( void );			// <functional>
};


#endif //_DEBUG


#endif // StringTests_h
