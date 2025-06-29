#ifndef ThumbnailTests_h
#define ThumbnailTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/UI/test/BaseImageTestCase.h"


namespace ut { class CTestDevice; }


class CThumbnailTests : public CBaseImageTestCase
{
	CThumbnailTests( void );
public:
	static CThumbnailTests& Instance( void );

	static const fs::TDirPath& GetThumbSaveDirPath( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	enum { MaxImageFiles = 50 };

	typedef std::pair<CBitmap*, std::tstring> TBitmapPathPair;

	static void DrawThumbs( ut::CTestDevice& rTestDev, const std::vector<TBitmapPathPair>& thumbs );
private:
	void TestThumbConversion( void );
	void TestImageThumbs( ut::CTestDevice& rTestDev );
	void TestThumbnailCache( ut::CTestDevice& rTestDev );
};


#endif //USE_UT


#endif // ThumbnailTests_h
