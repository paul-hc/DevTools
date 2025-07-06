#ifndef ImageTests_h
#define ImageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


namespace ut { class CTestDevice; }


class CImageTests : public ut::CGraphicTestCase
{
	CImageTests( void );
public:
	static CImageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestGroupIconRes( void );
	void TestIcon( ut::CTestDevice& rTestDev );
	void TestIconGroup( ut::CTestDevice& rTestDev );
	void TestImageListGuts( ut::CTestDevice& rTestDev );
	void TestImageListDisabled( ut::CTestDevice& rTestDev );
};


#endif //USE_UT


#endif // ImageTests_h
