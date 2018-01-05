#ifndef PublicEnums_h
#define PublicEnums_h
#pragma once


enum ViewMode
{
	vmDefaultMode = -1,
	vmFileName,
	vmRelPathFileName,
	vmIncDirective,
	vmFullPath
};

enum Ordering
{
	ordNormal,
	ordReverse,
	ordAlphaText,
	ordAlphaFileName
};


enum FolderOptions			// (!) careful to maintain C++ Macros.dsm corresponding constants when changing these constants
{
	foUsePopups			= 0x00000001,
	foRecurseFolders	= 0x00000002,
	foCutDuplicates		= 0x00000004,
	foHideExtension		= 0x00000010,
	foRightJustifyExt	= 0x00000020,
	foDirNamePrefix		= 0x00000040,
	foNoOptionsPopup	= 0x00000100,
	foSortFolders		= 0x00001000,

	foDefault			= foUsePopups
};


enum PathField				// (!) careful to maintain C++ Macros.dsm corresponding constants when changing these constants
{
	pfDrive,
	pfDir,
	pfName,
	pfExt,
	pfDirName,
	pfFullPath,
	pfDirPath,
	pfDirNameExt,
	pfNameExt,
	pfCoreExt,

	pfBaseFieldCount = pfFullPath,
	pfFieldCount,
};


enum FolderLayout			// (!) careful to maintain C++ Macros.dsm corresponding constants when changing these constants
{
	flFoldersAsPopups,
	flFoldersAsRootPopups,
	flRootFoldersExpanded,
	flAllFoldersExpanded,
};


#endif // PublicEnums_h
