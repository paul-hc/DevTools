#ifndef WicImageTests_h
#define WicImageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UI/test/BaseImageTestCase.h"


namespace ut { class CTestDevice; }


class CWicImageTests : public CBaseImageTestCase
{
	CWicImageTests( void );
public:
	static CWicImageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestImage( ut::CTestDevice* pTestDev );
	void TestImageCache( ut::CTestDevice* pTestDev );
};


#endif //USE_UT


#endif // WicImageTests_h
