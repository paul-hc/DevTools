#ifndef ImagingD2DTests_h
#define ImagingD2DTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/UI/test/BaseImageTestCase.h"


namespace ut { class CTestDevice; }
namespace d2d { class CDCRenderTarget; }


class CImagingD2DTests : public CBaseImageTestCase
{
	CImagingD2DTests( void );
public:
	static CImagingD2DTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestImage( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget );
	void TestImageEffects( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget );
	void TestImageAnimation( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget );
};


#endif //USE_UT


#endif // ImagingD2DTests_h
