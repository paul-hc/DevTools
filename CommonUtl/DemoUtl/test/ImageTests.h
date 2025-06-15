#ifndef ImageTests_h
#define ImageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CImageTests : public ut::CGraphicTestCase
{
	CImageTests( void );
public:
	static CImageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestGroupIconRes( void );
	void TestIcon( void );
	void TestImageList( void );
};


#endif //USE_UT


#endif // ImageTests_h
