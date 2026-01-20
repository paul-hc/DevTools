#ifndef WinExplorer_h
#define WinExplorer_h
#pragma once

#include "Path.h"
#include "ErrorHandler.h"
#include <shlobj.h>
#include <atlbase.h>


namespace shell
{
	// provide access to Windows Explorer's file system representation

	class CWinExplorer : public CErrorHandler
	{
	public:
		CWinExplorer( utl::ErrorHandling handlingMode = utl::CheckMode ) : CErrorHandler( handlingMode ) {}

		// IShellFolder
		CComPtr<IShellFolder> GetDesktopFolder( void ) const;

		bool ParseToPidl( OUT PIDLIST_RELATIVE* pPidl, IShellFolder* pFolder, const TCHAR* pFnameExt ) const;

		CComPtr<IShellFolder> FindShellFolder( const TCHAR* pDirPath ) const;		// no trailing backslash, please

		template< typename Interface >
		CComPtr<Interface> BindFileTo( IShellFolder* pFolder, const TCHAR* pFnameExt ) const;

		// IShellItem
		CComPtr<IShellItem> FindShellItem( const fs::CPath& filePath ) const;		// works with shortcuts only if you pass the link path "shortcut.lnk"
		std::tstring GetItemDisplayName( IShellItem* pShellItem, SIGDN nameType = SIGDN_FILESYSPATH ) const;
		fs::CPath GetItemPath( IShellItem* pShellItem ) const { return GetItemDisplayName( pShellItem, SIGDN_FILESYSPATH ); }

		// caller must delete the bitmap
		HBITMAP ExtractThumbnail( const fs::CPath& filePath, const CSize& boundsSize, DWORD flags = 0 ) const;		// IEIFLAG_ASPECT corrupts the original aspect ratio, forcing to boundsSize

		// faster
		HBITMAP ExtractThumbnail( IShellItem* pShellItem, const CSize& boundsSize, SIIGBF flags = SIIGBF_RESIZETOFIT ) const;		// SIIGBF_BIGGERSIZEOK improves performance

		size_t ExploreAndSelectItems( const std::vector<fs::CPath>& itemPaths ) const;		// open an explorer.exe window and select the items
	};


	// template code

	template< typename Interface >
	CComPtr<Interface> CWinExplorer::BindFileTo( IShellFolder* pFolder, const TCHAR* pFnameExt ) const
	{
		ASSERT_PTR( pFolder );
		CComPtr<Interface> pInterface;

		CComHeapPtr<ITEMIDLIST_RELATIVE> filePidl;
		if ( ParseToPidl( &filePidl, pFolder, pFnameExt ) )
		{
			PCUIDLIST_RELATIVE pFilePidl = filePidl;			// (!) pointer to pointer
			Handle( pFolder->GetUIObjectOf( nullptr, 1, (PCUITEMID_CHILD_ARRAY)&pFilePidl, __uuidof( Interface ), nullptr, (void**)&pInterface ) );
		}
		return pInterface;
	}

} //namespace shell


#endif // WinExplorer_h
