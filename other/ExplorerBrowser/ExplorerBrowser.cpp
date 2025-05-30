
#include "pch.h"
#include "ExplorerBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	std::tstring GetFolderDisplayName( PCIDLIST_ABSOLUTE pidlFolder )
	{
		std::tstring displayName;

		CComPtr<IShellFolder> pDesktopFolder;
		if ( HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
		{
			STRRET strDisplayName;
			if ( HR_OK( pDesktopFolder->GetDisplayNameOf( pidlFolder, SHGDN_FORPARSING, &strDisplayName ) ) )
			{
				LPTSTR szDisplayName = NULL;
				StrRetToStr( &strDisplayName, pidlFolder, &szDisplayName );
				displayName = szDisplayName;
				::CoTaskMemFree( szDisplayName );
			}
		}

		return displayName;
	}


	// CExplorerBrowser implementation

	CExplorerBrowser::~CExplorerBrowser()
	{
		if ( m_pExplorerBrowser != NULL )
			m_pExplorerBrowser->Destroy();
	}

	bool CExplorerBrowser::Create( CWnd* pParent, bool showFolders, FOLDERVIEWMODE filePaneViewMode )
	{
		CRect browserRect;
		pParent->GetClientRect( &browserRect );

		if ( HR_OK( ::SHCoCreateInstance( NULL, &CLSID_ExplorerBrowser, NULL, IID_PPV_ARGS( &m_pExplorerBrowser ) ) ) )
		{
			if ( showFolders )
				m_pExplorerBrowser->SetOptions( EBO_SHOWFRAMES );

			FOLDERSETTINGS settings = { filePaneViewMode, FWF_NONE };
			if ( HR_OK( m_pExplorerBrowser->Initialize( pParent->GetSafeHwnd(), &browserRect, &settings ) ) )
				return true;
		}

		return false;
	}

	CComPtr<IShellView> CExplorerBrowser::GetShellView( void ) const
	{
		CComPtr<IShellView> pShellView;
		if ( HR_OK( m_pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pShellView ) ) ) )
			return pShellView;

		return NULL;
	}

	CComPtr<IFolderView2> CExplorerBrowser::GetFolderView( void ) const
	{
		CComPtr<IFolderView2> pFolderView;
		if ( HR_OK( m_pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pFolderView ) ) ) )
			return pFolderView;

		return NULL;
	}

	CComPtr<IShellFolder> CExplorerBrowser::GetCurrentFolder( void ) const
	{
		CComPtr<IShellFolder> pCurrShellFolder;

		if ( CComPtr<IFolderView2> pFolderView = GetFolderView() )
			if ( HR_OK( pFolderView->GetFolder( IID_PPV_ARGS( &pCurrShellFolder ) ) ) )
				return pCurrShellFolder;

		return NULL;
	}

	bool CExplorerBrowser::GetCurrentDirPidl( CPidl& rCurrDirPidl ) const
	{
		rCurrDirPidl.Reset();

		if ( CComPtr<IFolderView2> pFolderView = GetFolderView() )
		{
			CComPtr<IPersistFolder2> pCurrFolder;
			if ( HR_OK( pFolderView->GetFolder( IID_PPV_ARGS( &pCurrFolder ) ) ) )
				if ( HR_OK( pCurrFolder->GetCurFolder( &rCurrDirPidl ) ) )
					return true;
		}

		return false;
	}

	std::tstring CExplorerBrowser::GetCurrentDirPath( void ) const
	{
		CPidl currDirPidl;
		if ( GetCurrentDirPidl( currDirPidl ) )
		{
			TCHAR dirPath[ MAX_PATH ];
			if ( ::SHGetPathFromIDList( currDirPidl.Get(), dirPath ) )
				return dirPath;
		}
		return std::tstring();
	}

	void CExplorerBrowser::QuerySelectedFiles( std::vector<std::tstring>& rSelPaths ) const
	{
		if ( CComPtr<IShellView> pShellView = GetShellView() )
		{
			CComPtr<IDataObject> pDataObject;
			if ( HR_OK( pShellView->GetItemObject( SVGIO_SELECTION, IID_PPV_ARGS( &pDataObject ) ) ) )
			{
				// code adapted from http://www.codeproject.com/shell/shellextguide1.asp
				FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg;
				stg.tymed =  TYMED_HGLOBAL;

				if ( HR_OK( pDataObject->GetData( &fmt, &stg ) ) )
				{
					HDROP hDrop = (HDROP)::GlobalLock( stg.hGlobal );

					for ( UINT i = 0, fileCount = ::DragQueryFile( hDrop, 0xFFFFFFFF, NULL, 0 ); i != fileCount; ++i )
					{
						TCHAR filePath[ MAX_PATH ] = _T("");
						::DragQueryFile( hDrop, i, filePath, _countof( filePath ) );

						if ( filePath[ 0 ] != _T('\0') )
							rSelPaths.push_back( filePath );
					}

					::GlobalUnlock( stg.hGlobal );
					::ReleaseStgMedium( &stg );
				}
			}
		}
	}

	bool CExplorerBrowser::SelectItems( const std::vector<std::tstring>& itemFilenames )
	{
		CComPtr<IShellFolder> pCurrFolder = GetCurrentFolder();
		SVSIF selectFlags = SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS;
		size_t selCount = 0;

		if ( CComPtr<IShellView> pShellView = GetShellView() )
			for ( std::vector<std::tstring>::const_iterator itFilename = itemFilenames.begin(); itFilename != itemFilenames.end(); ++itFilename )
			{
				CPidl selItemPidl;
				if ( selItemPidl.CreateChild( pCurrFolder, itFilename->c_str() ) )
				{
					if ( HR_OK( pShellView->SelectItem( selItemPidl.Get(), selectFlags ) ) )
					{
						++selCount;
						ClearFlag( selectFlags, SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS );
					}
				}
			}

		return selCount == itemFilenames.size();			// all filename items selected?
	}

	FOLDERVIEWMODE CExplorerBrowser::GetFilePaneViewMode( UINT* pOutFolderFlags /*= NULL*/ ) const
	{
		ASSERT_PTR( m_pExplorerBrowser );

		if ( CComPtr<IShellView> pShellView = GetShellView() )
		{
			FOLDERSETTINGS settings;
			if ( HR_OK( pShellView->GetCurrentInfo( &settings ) ) )
			{
				if ( pOutFolderFlags != NULL )
					*pOutFolderFlags = settings.fFlags;

				return static_cast<FOLDERVIEWMODE>( settings.ViewMode );
			}
		}

		return FVM_AUTO;
	}

	bool CExplorerBrowser::SetFilePaneViewMode( FOLDERVIEWMODE filePaneViewMode, FOLDERFLAGS flags /*= FWF_NONE*/ ) const
	{
		FOLDERSETTINGS settings = { filePaneViewMode, flags };
		return HR_OK( m_pExplorerBrowser->SetFolderSettings( &settings ) );
	}

	bool CExplorerBrowser::NavigateTo( const TCHAR dirPath[] )
	{
		CPidl dirPathPidl;
		if ( HR_OK( ::SHParseDisplayName( dirPath, NULL, &dirPathPidl, 0, NULL ) ) )
			return HR_OK( m_pExplorerBrowser->BrowseToIDList( dirPathPidl.Get(), 0 ) );

		return false;
	}

	bool CExplorerBrowser::RenameFile( void )
	{
		ASSERT_PTR( m_pExplorerBrowser );

		if ( CComPtr<IFolderView2> pFolderView = GetFolderView() )
			return HR_OK( pFolderView->DoRename() );

		return false;
	}

} //namespace shell
