#ifndef ShellDialogs_h
#define ShellDialogs_h
#pragma once

#include "ShellDialogs_fwd.h"


class CFileDialog;


namespace shell
{
	bool BrowseForFolder( std::tstring& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName = NULL,
						  BrowseFlags flags = BF_FileSystem, const TCHAR* pTitle = NULL, bool useNetwork = false );

	bool BrowseForFile( std::tstring& rFilePath, CWnd* pParentWnd, BrowseMode browseMode = FileOpen,
						const TCHAR* pFileFilter = NULL, DWORD flags = 0, const TCHAR* pTitle = NULL );

	bool PickFolder( std::tstring& rFilePath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options = 0, const TCHAR* pTitle = NULL );

	namespace impl
	{
		CFileDialog* MakeFileDialog( const std::tstring& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags = 0, const TCHAR* pTitle = NULL );

		bool RunFileDialog( std::tstring& rFilePath, CFileDialog* pFileDialog );
	}
}


#endif // ShellDialogs_h
