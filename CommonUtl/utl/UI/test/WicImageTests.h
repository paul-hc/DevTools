#ifndef WicImageTests_h
#define WicImageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UI/test/BaseImageTestCase.h"


namespace fs { class CFlexPath; }
namespace ut { class CTestDevice; }


class CWicImageTests : public CBaseImageTestCase
{
	CWicImageTests( void );

	static void DisplayMultiFrameImageStrip( ut::CTestDevice& rTestDev, const fs::CFlexPath& imagePath );
public:
	static CWicImageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestIconMultiFrame_Scissors( ut::CTestDevice& rTestDev );
	void TestIconMultiFrame_RedBubbles( ut::CTestDevice& rTestDev );
	void TestImageCache( ut::CTestDevice& rTestDev );
};


#endif //USE_UT


#endif // WicImageTests_h
