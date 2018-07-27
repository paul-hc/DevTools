#ifndef ExplorerBrowser_h
#define ExplorerBrowser_h
#pragma once

#include "ShellTypes.h"


namespace shell
{
	std::tstring GetFolderDisplayName( PCIDLIST_ABSOLUTE pidlFolder );


	class CExplorerBrowser
	{
	public:
		CExplorerBrowser( void ) {}
		~CExplorerBrowser();

		bool Create( CWnd* pParent, bool showFolders, FOLDERVIEWMODE filePaneViewMode );

		IExplorerBrowser* Get( void ) const { return m_pExplorerBrowser; }
		CComPtr< IShellView > GetShellView( void ) const;
		CComPtr< IFolderView2 > GetFolderView( void ) const;

		CComPtr< IShellFolder > GetCurrentFolder( void ) const;

		bool GetCurrentDirPidl( CPidl& rCurrDirPidl ) const;
		std::tstring GetCurrentDirPath( void ) const;		// dir path of the current folder in explorer view

		void QuerySelectedFiles( std::vector< std::tstring >& rSelPaths ) const;
		bool SelectItems( const std::vector< std::tstring >& itemFilenames );

		FOLDERVIEWMODE GetFilePaneViewMode( UINT* pOutFolderFlags = NULL ) const;
		bool SetFilePaneViewMode( FOLDERVIEWMODE filePaneViewMode, FOLDERFLAGS flags = FWF_NONE ) const;

		bool NavigateTo( const TCHAR dirPath[] );
		bool NavigateBack( void ) { return SUCCEEDED( m_pExplorerBrowser->BrowseToIDList( NULL, SBSP_NAVIGATEBACK ) ); }
		bool NavigateForward( void ) { return SUCCEEDED( m_pExplorerBrowser->BrowseToIDList( NULL, SBSP_NAVIGATEFORWARD ) ); }

		bool RenameFile( void );
	private:
		CComPtr< IExplorerBrowser > m_pExplorerBrowser;
	};
}


#endif // ExplorerBrowser_h
