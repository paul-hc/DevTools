#ifndef ShellDialogs_h
#define ShellDialogs_h
#pragma once

#include "Path_fwd.h"
#include "ShellDialogs_fwd.h"


class CFileDialog;
class CFlagTags;


namespace shell
{
	// new Vista+ File Dialog for folders:
	bool BrowseForFile( IN OUT shell::TPath& rShellPath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = nullptr, DWORD flags = 0, const TCHAR* pTitle = nullptr );
	bool BrowseForFiles( IN OUT std::vector<shell::TPath>& rShellPaths, CWnd* pParentWnd,
						 const TCHAR* pFileFilter = nullptr, DWORD flags = 0, const TCHAR* pTitle = nullptr );

	bool PickFolder( IN OUT shell::TFolderPath& rFolderShellPath, CWnd* pParentWnd, FILEOPENDIALOGOPTIONS options = 0, const TCHAR* pTitle = nullptr );
		// rFolderShellPath can contain a wildcard pattern, which will be preserved.
		// pass FOS_ALLNONSTORAGEITEMS in options to pick a virtual folder path (such as Control Panel, etc).

	bool BrowseAutoPath( IN OUT shell::TPath& rShellPath, CWnd* pParent, const TCHAR* pFileFilter = nullptr );	// choose the browse file/folder based on current path


	// legacy/classic browse folder tree dialog
	bool BrowseForFolder( IN OUT shell::TFolderPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = nullptr,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = nullptr, bool useNetwork = false );


	// diagnostics
	const CFlagTags& GetTags_OFN_Flags( void );						// OFN_ legacy flags
	const CFlagTags& GetTags_FILEOPENDIALOGOPTIONS( void );		// Vista-style flags
}


#endif // ShellDialogs_h
