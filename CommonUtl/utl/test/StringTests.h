#ifndef StringTests_h
#define StringTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

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
	void TestIsCharType( void );
	void TestValueToString( void );
	void TestTrim( void );
	void TestEnquote( void );
	void TestStringSplit( void );
	void TestStringTokenize( void );
	void TestStringPrefixSuffix( void );
	void TestStringConversion( void );
	void TestStringLines( void );

	void TestSearchEnclosedItems( void );
	void TestReplaceEnclosedItems( void );

	void TestArgUtilities( void );
	void TestEnumTags( void );
	void TestFlagTags( void );
	void TestWordSelection( void );
	void TestEnsureUniformNumPadding( void );
	void TestTimeFormatting( void );

	void TestFunctional( void );			// <functional>
};


#endif //USE_UT


#endif // StringTests_h
