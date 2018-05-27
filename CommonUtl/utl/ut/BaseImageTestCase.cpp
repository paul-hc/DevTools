
#include "stdafx.h"
#include "ut/BaseImageTestCase.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG


const fs::CPath& CBaseImageTestCase::GetImageSourceDirPath( void )
{
	static fs::CPath imagesDirPath = str::ExpandEnvironmentStrings( _T("%UTL_THUMB_SRC_IMAGE_PATH%") );
	if ( !imagesDirPath.IsEmpty() && !fs::IsValidDirectory( imagesDirPath.GetPtr() ) )
	{
		TRACE( _T("\n # Cannot find unit test images dir path: %s #\n"), imagesDirPath.GetPtr() );
		imagesDirPath.Clear();
	}
	return imagesDirPath;
}

const fs::CPath& CBaseImageTestCase::GetTestImagesDirPath( void )
{
	static fs::CPath dirPath = ut::GetTestDataDirPath() / fs::CPath( _T("images") );
	if ( !dirPath.IsEmpty() && !fs::IsValidDirectory( dirPath.GetPtr() ) )
	{
		TRACE( _T("\n * Cannot find the local test images dir path: %s\n"), dirPath.GetPtr() );
		dirPath.Clear();
	}
	return dirPath;
}

const TCHAR* CBaseImageTestCase::GetImageFilename( SrcImage srcImage )
{
	static const TCHAR* s_imagePaths[ _ImageCount ] = { _T("Flamingos.jpg"), _T("Dice.png"), _T("Animated.gif"), _T("Scissors.ico") };

	ASSERT( srcImage < _ImageCount );
	return s_imagePaths[ srcImage ];
}

fs::CFlexPath CBaseImageTestCase::MakeTestImageFilePath( SrcImage srcImage )
{
	fs::CFlexPath imagePath(
		( GetTestImagesDirPath() / fs::CPath( GetImageFilename( srcImage ) ) ).Get()
	);
	if ( !imagePath.IsEmpty() && !imagePath.FileExist() )
	{
		TRACE( _T("\n * Cannot find the local test image file: %s\n"), imagePath.GetPtr() );
		imagePath.Clear();
	}
	return imagePath;
}


#endif //_DEBUG
