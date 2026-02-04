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
	bool BrowseForFile( OUT fs::CPath& rFilePath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = nullptr, DWORD flags = 0, const TCHAR* pTitle = nullptr );

	bool PickFolder( OUT shell::TDirPath& rFolderShellPath, CWnd* pParentWnd, FILEOPENDIALOGOPTIONS options = 0, const TCHAR* pTitle = nullptr );
		// rFolderShellPath can contain a wildcard pattern, which will be preserved.
		// pass FOS_ALLNONSTORAGEITEMS in options to pick a virtual folder path (such as Control Panel, etc).

	bool BrowseAutoPath( OUT fs::CPath& rFilePath, CWnd* pParent, const TCHAR* pFileFilter = nullptr );	// choose the browse file/folder based on current path


	// classic browse folder tree dialog
	bool BrowseForFolder( OUT shell::TDirPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = nullptr,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = nullptr, bool useNetwork = false );


	namespace impl
	{
		CFileDialog* MakeFileDialog( const fs::CPath& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags = 0, const TCHAR* pTitle = nullptr );

		bool RunFileDialog( OUT fs::CPath& rFilePath, CFileDialog* pFileDialog );
	}


	// diagnostics
	const CFlagTags& GetTags_OFN_Flags( void );						// OFN_ legacy flags
	const CFlagTags& GetTags_FILEOPENDIALOGOPTIONS( void );		// Vista-style flags
}


#endif // ShellDialogs_h
