#ifndef ShellDialogs_h
#define ShellDialogs_h
#pragma once

#include "ShellDialogs_fwd.h"


class CFileDialog;
namespace fs { class CPath; }


namespace shell
{
	// classic browse folder tree dialog
	bool BrowseForFolder( fs::CPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = NULL,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = NULL, bool useNetwork = false );

	bool BrowseForFile( fs::CPath& rFilePath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = NULL, DWORD flags = 0, const TCHAR* pTitle = NULL );

	// new Vista+ File Dialog for folders
	bool PickFolder( fs::CPath& rFilePath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options = 0, const TCHAR* pTitle = NULL );

	namespace impl
	{
		CFileDialog* MakeFileDialog( const fs::CPath& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags = 0, const TCHAR* pTitle = NULL );

		bool RunFileDialog( fs::CPath& rFilePath, CFileDialog* pFileDialog );
	}
}


#endif // ShellDialogs_h
