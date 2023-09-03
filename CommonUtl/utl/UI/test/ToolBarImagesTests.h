#ifndef ToolBarImagesTests_h
#define ToolBarImagesTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UI/test/BaseImageTestCase.h"


namespace ut { class CTestDevice; }


class CToolBarImagesTests : public CBaseImageTestCase
{
	CToolBarImagesTests( void );
public:
	static CToolBarImagesTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestToolBarImagesBitmap( ut::CTestDevice* pTestDev );
	void TestToolBarImagesDetails( ut::CTestDevice* pTestDev );
};


#endif //USE_UT


#endif // ToolBarImagesTests_h
