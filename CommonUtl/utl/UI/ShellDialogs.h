#ifndef ShellDialogs_h
#define ShellDialogs_h
#pragma once

#include "Path_fwd.h"
#include "ShellDialogs_fwd.h"


class CFileDialog;


namespace shell
{
	// classic browse folder tree dialog
	bool BrowseForFolder( fs::TDirPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = nullptr,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = nullptr, bool useNetwork = false );

	bool BrowseForFile( fs::CPath& rFilePath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = nullptr, DWORD flags = 0, const TCHAR* pTitle = nullptr );

	// new Vista+ File Dialog for folders
	bool PickFolder( fs::TDirPath& rFilePath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options = 0, const TCHAR* pTitle = nullptr );

	bool BrowseAutoPath( fs::CPath& rFilePath, CWnd* pParent, const TCHAR* pFileFilter = nullptr );	// choose the browse file/folder based on current path

	namespace impl
	{
		CFileDialog* MakeFileDialog( const fs::CPath& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags = 0, const TCHAR* pTitle = nullptr );

		bool RunFileDialog( fs::CPath& rFilePath, CFileDialog* pFileDialog );
	}
}


#endif // ShellDialogs_h
