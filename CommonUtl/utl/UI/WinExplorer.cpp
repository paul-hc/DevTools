
#include "pch.h"
#include "WinExplorer.h"
#include "ShellTypes.h"
#include "ShellPidl.h"
#include "FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*	Explorer Shell object model

	IUnknown
		IExplorerBrowser
			{ ::SHCoCreateInstance( nullptr, &CLSID_ExplorerBrowser, nullptr, IID_PPV_ARGS( &m_pExplorerBrowser ) ) }

		IShellFolder
			IShellFolder2
			{ ::SHGetDesktopFolder( &pDesktopShellFolder ) }

		IShellItem
			IShellItem2

		IOleWindow
			IShellView
				IShellView2
					IShellView3
					{ IShellView2::SelectAndPositionItem() }

		IFolderView
			IFolderView2

		IPersist
			IPersistFolder
				IPersistFolder2
					IPersistFolder3

		IDataObject
*/

namespace shell
{
	CComPtr<IShellFolder> CWinExplorer::GetDesktopFolder( void ) const
	{
		CComPtr<IShellFolder> pDesktopFolder;
		Handle( ::SHGetDesktopFolder( &pDesktopFolder ) );
		return pDesktopFolder;
	}

	bool CWinExplorer::ParseToPidl( OUT PIDLIST_RELATIVE* pPidl, IShellFolder* pFolder, const TCHAR* pFnameExt ) const
	{
		ASSERT_PTR( pPidl );
		ASSERT_PTR( pFolder );
		ASSERT_PTR( pFnameExt );

		TCHAR displayName[ MAX_PATH * 2 ];
		_tcscpy( displayName, pFnameExt );

		return Handle( pFolder->ParseDisplayName( nullptr, nullptr, displayName, nullptr, pPidl, nullptr ) );
	}

	CComPtr<IShellFolder> CWinExplorer::FindShellFolder( const TCHAR* pDirPath ) const
	{
		CComPtr<IShellFolder> pDirFolder;
		if ( fs::IsValidDirectory( pDirPath ) )
			if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
			{
				CComHeapPtr<ITEMIDLIST_RELATIVE> workDirPidl;

				if ( ParseToPidl( &workDirPidl, pDesktopFolder, pDirPath ) )
					Handle( pDesktopFolder->BindToObject( workDirPidl, nullptr, IID_PPV_ARGS( &pDirFolder ) ) );
			}

		return pDirFolder;
	}


	CComPtr<IShellItem> CWinExplorer::FindShellItem( const fs::CPath& fullPath ) const
	{
		CComPtr<IShellItem> pShellItem;
		Handle( ::SHCreateItemFromParsingName( fullPath.GetPtr(), nullptr, IID_PPV_ARGS( &pShellItem ) ) );
		return pShellItem;
	}

	std::tstring CWinExplorer::GetItemDisplayName( IShellItem* pShellItem, SIGDN nameType /*= SIGDN_FILESYSPATH*/ ) const
	{
		ASSERT_PTR( pShellItem );

		CComHeapPtr<wchar_t> pDisplayName;

		if ( HR_OK( pShellItem->GetDisplayName( nameType, &pDisplayName ) ) )
			return (const wchar_t*)pDisplayName;

		return std::tstring();
	}

	HBITMAP CWinExplorer::ExtractThumbnail( const fs::CPath& filePath, const CSize& boundsSize, DWORD flags /*= 0*/ ) const
	{
		CComPtr<IShellFolder> pDirFolder = FindShellFolder( filePath.GetParentPath().GetPtr() );
		if ( pDirFolder != nullptr )
		{
			CComPtr<IExtractImage> pExtractImage = BindFileTo<IExtractImage>( pDirFolder, filePath.GetFilenamePtr() );
			if ( pExtractImage != nullptr )
			{
				// define thumbnail properties
				DWORD priority = IEIT_PRIORITY_NORMAL;
				TCHAR pathBuffer[ MAX_PATH ];
				if ( Handle( pExtractImage->GetLocation( pathBuffer, MAX_PATH, &priority, &boundsSize, 16, &flags ) ) )
				{
					// generate thumbnail
					HBITMAP hThumbBitmap = nullptr;
					if ( Handle( pExtractImage->Extract( &hThumbBitmap ) ) )
						return hThumbBitmap;
				}
			}
		}
		return nullptr;
	}

	HBITMAP CWinExplorer::ExtractThumbnail( IShellItem* pShellItem, const CSize& boundsSize, SIIGBF flags /*= SIIGBF_RESIZETOFIT*/ ) const
	{
		HBITMAP hBitmap = nullptr;
		CComQIPtr<IShellItemImageFactory> pImageFactory( pShellItem );
		if ( pImageFactory != nullptr )
			Handle( pImageFactory->GetImage( boundsSize, flags, &hBitmap ) );

		return hBitmap;
	}

	size_t CWinExplorer::ExploreAndSelectItems( const std::vector<fs::CPath>& itemPaths ) const
	{
		size_t selCount = 0;
		if ( !itemPaths.empty() )
		{
			fs::CPath folderPath = itemPaths.front().GetParentPath();

			CPidlAbsolute folderPidl;
			if ( folderPidl.CreateFromPath( folderPath.GetPtr() ) )
			{
				std::vector<PIDLIST_ABSOLUTE> itemPidls; itemPidls.reserve( itemPaths.size() );

				for ( std::vector<fs::CPath>::const_iterator itItemPath = itemPaths.begin(); itItemPath != itemPaths.end(); ++itItemPath )
					if ( folderPath == itItemPath->GetParentPath() )				// selectable in the same folder view?
					{
						CPidlAbsolute itemPidl;
						if ( itemPidl.CreateFromPath( itItemPath->GetPtr() ) )		// works with absolute PIDL as well
							itemPidls.push_back( itemPidl.Release() );
					}

				HR_AUDIT( ::SHOpenFolderAndSelectItems( folderPidl.Get(), static_cast<UINT>( itemPidls.size() ), (PCUITEMID_CHILD_ARRAY)&itemPidls.front(), 0 ) );

				selCount = itemPidls.size();
				shell::ClearOwningPidls( itemPidls );
			}
		}

		return selCount;
	}

} //namespace shell
