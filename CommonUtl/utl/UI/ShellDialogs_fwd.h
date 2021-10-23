#ifndef ShellDialogs_fwd_h
#define ShellDialogs_fwd_h
#pragma once


namespace shell
{
	enum BrowseMode { FileSaveAs, FileOpen, FileBrowse };

	enum BrowseFlags
	{
		BF_Computers,
		BF_Printers,
		BF_FileSystem,
		BF_FileSystemIncludeFiles,
		BF_AllIncludeFiles,
			BF_All
	};


	extern bool s_useVistaStyle;			// set to false to disable Vista style file dialog, which for certain apps is crashing; Vista style requires COM initialization


	bool IsValidDirectoryPattern( const fs::CPath& dirPatternPath, fs::CPath* pDirPath = NULL, std::tstring* pWildSpec = NULL );	// a valid directory path with a wildcard pattern?
}


#endif // ShellDialogs_fwd_h
