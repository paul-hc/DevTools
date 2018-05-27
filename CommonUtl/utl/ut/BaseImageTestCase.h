#ifndef BaseImageTestCase_h
#define BaseImageTestCase_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/FlexPath.h"
#include "ut/UnitTest.h"


class CBaseImageTestCase : public ut::CGraphicTestCase
{
protected:
	CBaseImageTestCase( void ) {}
public:
	static const fs::CPath& GetImageSourceDirPath( void );		// %UTL_THUMB_SRC_IMAGE_PATH%
	static const fs::CPath& GetTestImagesDirPath( void );		// %UTL_TESTDATA_PATH%\images

	enum SrcImage { Flamingos_jpg, Dice_png, Animated_gif, Scissors_ico, _ImageCount };

	static fs::CFlexPath MakeTestImageFilePath( SrcImage srcImage );
	static const TCHAR* GetImageFilename( SrcImage srcImage );
};


#endif //_DEBUG


#endif // BaseImageTestCase_h
