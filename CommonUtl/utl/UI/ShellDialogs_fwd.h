#ifndef ShellDialogs_fwd_h
#define ShellDialogs_fwd_h
#pragma once

#include <afxdlgs.h>


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
}


#endif // ShellDialogs_fwd_h
