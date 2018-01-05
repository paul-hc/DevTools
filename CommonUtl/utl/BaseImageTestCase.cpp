
#include "stdafx.h"
#include "BaseImageTestCase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG


const std::tstring& CBaseImageTestCase::GetImageSourceDirPath( void )
{
	static std::tstring imagesDirPath = str::ExpandEnvironmentStrings( _T("%UTL_THUMB_SRC_IMAGE_PATH%") );
	if ( !imagesDirPath.empty() && !fs::IsValidDirectory( imagesDirPath.c_str() ) )
	{
		TRACE( _T("\n # Cannot find unit test images dir path: %s #\n"), imagesDirPath.c_str() );
		imagesDirPath.clear();
	}
	return imagesDirPath;
}

const std::tstring& CBaseImageTestCase::GetTestImagesDirPath( void )
{
	static std::tstring dirPath = ut::CombinePath( ut::GetTestDataDirPath(), _T("images") );
	if ( !dirPath.empty() && !fs::IsValidDirectory( dirPath.c_str() ) )
	{
		TRACE( _T("\n * Cannot find the local test images dir path: %s\n"), dirPath.c_str() );
		dirPath.clear();
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
	fs::CFlexPath imagePath( ut::CombinePath( GetTestImagesDirPath(), GetImageFilename( srcImage ) ) );
	if ( !imagePath.IsEmpty() && !imagePath.FileExist() )
	{
		TRACE( _T("\n * Cannot find the local test image file: %s\n"), imagePath.GetPtr() );
		imagePath.Clear();
	}
	return imagePath;
}


#endif //_DEBUG
