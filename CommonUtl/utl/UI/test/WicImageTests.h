#ifndef WicImageTests_h
#define WicImageTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

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


#endif //_DEBUG


#endif // WicImageTests_h
