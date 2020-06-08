#ifndef NumericTests_h
#define NumericTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CNumericTests : public ut::CConsoleTestCase
{
	CNumericTests( void );

	static const std::vector< UINT >& GetReferenceCrc32Table( void );
public:
	static CNumericTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestFormatNumber( void );
	void TestFormatNumberUserLocale( void );
	void TestParseNumber( void );
	void TestParseNumberUserLocale( void );
	void TestFindNumber( void );
	void TestGenerateNumericSequence( void );
	void TestConvertFileSize( void );
	void TestFormatFileSize( void );
	void TestCrc32( void );
	void TestMemLeakCheck( void );
};


#endif //_DEBUG


#endif // NumericTests_h
