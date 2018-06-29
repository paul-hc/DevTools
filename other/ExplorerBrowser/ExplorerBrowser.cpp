
#include "stdafx.h"
#include "ExplorerBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	std::tstring GetFolderDisplayName( PCIDLIST_ABSOLUTE pidlFolder )
	{
		std::tstring displayName;

		CComPtr< IShellFolder > pDesktopFolder;
		if ( SUCCEEDED( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
		{
			STRRET sDisplayName;
			if ( SUCCEEDED( pDesktopFolder->GetDisplayNameOf( pidlFolder, SHGDN_FORPARSING, &sDisplayName ) ) )
			{
				LPTSTR szDisplayName = NULL;
				StrRetToStr( &sDisplayName, pidlFolder, &szDisplayName );
				displayName = szDisplayName;
				CoTaskMemFree( szDisplayName );
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

		if ( SUCCEEDED( ::SHCoCreateInstance( NULL, &CLSID_ExplorerBrowser, NULL, IID_PPV_ARGS( &m_pExplorerBrowser ) ) ) )
		{
			if ( showFolders )
				m_pExplorerBrowser->SetOptions( EBO_SHOWFRAMES );

			FOLDERSETTINGS settings = { filePaneViewMode, FWF_NONE };
			if ( SUCCEEDED( m_pExplorerBrowser->Initialize( pParent->GetSafeHwnd(), &browserRect, &settings ) ) )
				return true;
		}

		return false;
	}

	CComPtr< IShellView > CExplorerBrowser::GetShellView( void ) const
	{
		CComPtr< IShellView > pShellView;
		if ( SUCCEEDED( m_pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pShellView ) ) ) )
			return pShellView;

		return NULL;
	}

	CComPtr< IFolderView2 > CExplorerBrowser::GetFolderView( void ) const
	{
		CComPtr< IFolderView2 > pFolderView;
		if ( SUCCEEDED( m_pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pFolderView ) ) ) )
			return pFolderView;

		return NULL;
	}

	void CExplorerBrowser::QuerySelectedFiles( std::vector< std::tstring >& rSelPaths ) const
	{
		if ( CComPtr< IShellView > pShellView = GetShellView() )
		{
			CComPtr< IDataObject > pDataObject;
			if ( SUCCEEDED( pShellView->GetItemObject( SVGIO_SELECTION, IID_PPV_ARGS( &pDataObject ) ) ) )
			{
				// code adapted from http://www.codeproject.com/shell/shellextguide1.asp
				FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg;
				stg.tymed =  TYMED_HGLOBAL;

				if ( SUCCEEDED( pDataObject->GetData( &fmt, &stg ) ) )
				{
					HDROP hDrop = (HDROP)::GlobalLock( stg.hGlobal );

					for ( UINT i = 0, fileCount = ::DragQueryFile( hDrop, 0xFFFFFFFF, NULL, 0 ); i != fileCount; ++i )
					{
						TCHAR path[ MAX_PATH ] = _T("");
						::DragQueryFile( hDrop, i, path, _countof( path ) );

						if ( path[ 0 ] != _T('\0') )
							rSelPaths.push_back( path );
					}

					::GlobalUnlock( stg.hGlobal );
					::ReleaseStgMedium( &stg );
				}
			}
		}
	}

	FOLDERVIEWMODE CExplorerBrowser::GetFilePaneViewMode( UINT* pOutFolderFlags /*= NULL*/ ) const
	{
		ASSERT_PTR( m_pExplorerBrowser );

		if ( CComPtr< IShellView > pShellView = GetShellView() )
		{
			FOLDERSETTINGS settings;
			if ( SUCCEEDED( pShellView->GetCurrentInfo( &settings ) ) )
			{
				if ( pOutFolderFlags != NULL )
					*pOutFolderFlags = settings.fFlags;

				return static_cast< FOLDERVIEWMODE >( settings.ViewMode );
			}
		}

		return FVM_AUTO;
	}

	bool CExplorerBrowser::SetFilePaneViewMode( FOLDERVIEWMODE filePaneViewMode, FOLDERFLAGS flags /*= FWF_NONE*/ ) const
	{
		FOLDERSETTINGS settings = { filePaneViewMode, flags };
		return SUCCEEDED( m_pExplorerBrowser->SetFolderSettings( &settings ) );
	}

	bool CExplorerBrowser::NavigateTo( const TCHAR path[] )
	{
		LPITEMIDLIST pidlBrowse;
		if ( SUCCEEDED( ::SHParseDisplayName( path, NULL, &pidlBrowse, 0, NULL ) ) )
			if ( SUCCEEDED( m_pExplorerBrowser->BrowseToIDList( pidlBrowse, 0 ) ) )
			{
				::ILFree( pidlBrowse );
				return true;
			}

		return false;
	}

	bool CExplorerBrowser::RenameFile( void )
	{
		ASSERT_PTR( pExplorerBrowser );

		if ( CComPtr< IFolderView2 > pFolderView = GetFolderView() )
			return SUCCEEDED( pFolderView->DoRename() );

		return false;
	}

} //namespace shell
