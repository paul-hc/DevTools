#ifndef ThumbnailTests_h
#define ThumbnailTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/UI/ut/BaseImageTestCase.h"


class CThumbnailTests : public CBaseImageTestCase
{
	CThumbnailTests( void );
public:
	static CThumbnailTests& Instance( void );

	static const fs::CPath& GetThumbSaveDirPath( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	enum { MaxImageFiles = 50 };

	static void DrawThumbs( const std::vector< CBitmap* >& thumbs );
private:
	void TestThumbConversion( void );
	void TestImageThumbs( void );
	void TestThumbnailCache( void );
};


#endif //_DEBUG


#endif // ThumbnailTests_h
