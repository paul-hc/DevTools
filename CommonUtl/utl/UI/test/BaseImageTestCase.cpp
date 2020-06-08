
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/BaseImageTestCase.h"
#include "StringUtilities.h"

#define new DEBUG_NEW


const TCHAR* CBaseImageTestCase::GetImageFilename( SrcImage srcImage )
{
	static const TCHAR* s_imagePaths[ _ImageCount ] = { _T("Flamingos.jpg"), _T("Dice.png"), _T("Animated.gif"), _T("Scissors.ico") };

	ASSERT( srcImage < _ImageCount );
	return s_imagePaths[ srcImage ];
}

fs::CFlexPath CBaseImageTestCase::MakeTestImageFilePath( SrcImage srcImage )
{
	fs::CFlexPath imagePath(
		( ut::GetTestImagesDirPath() / fs::CPath( GetImageFilename( srcImage ) ) ).Get()
	);

	if ( !imagePath.IsEmpty() && !imagePath.FileExist() )
	{
		TRACE( _T("\n * Cannot find the local test image file: %s\n"), imagePath.GetPtr() );
		imagePath.Clear();
	}
	return imagePath;
}


#endif //_DEBUG
