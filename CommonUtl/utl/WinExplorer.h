#ifndef WinExplorer_h
#define WinExplorer_h
#pragma once

#include "Path.h"
#include "ThrowMode.h"
#include <shlobj.h>
#include <atlbase.h>


namespace shell
{
	// provide access to Windows Explorer's file system representation

	class CWinExplorer : public CThrowMode
	{
	public:
		CWinExplorer( bool throwMode = false ) : CThrowMode( throwMode ) {}

		CComPtr< IShellFolder > GetDesktopFolder( void ) const;
		CComPtr< IShellItem > FindShellItem( const fs::CPath& filePath ) const;		// works with shortcuts only if you pass the link path "shortcut.lnk"

		bool ParsePidl( PIDLIST_RELATIVE* pPidl, IShellFolder* pFolder, const TCHAR* pFnameExt ) const;

		CComPtr< IShellFolder > FindShellFolder( const TCHAR* pDirPath ) const;		// no trailing backslash, please

		template< typename Interface >
		CComPtr< Interface > BindFileTo( IShellFolder* pFolder, const TCHAR* pFnameExt ) const;

		// caller must delete the bitmap
		HBITMAP ExtractThumbnail( const fs::CPath& filePath, const CSize& boundsSize, DWORD flags = 0 ) const;		// IEIFLAG_ASPECT corrupts the original aspect ratio, forcing to boundsSize

		// faster
		HBITMAP ExtractThumbnail( IShellItem* pShellItem, const CSize& boundsSize, SIIGBF flags = SIIGBF_RESIZETOFIT ) const;		// SIIGBF_BIGGERSIZEOK improves performance
	};


	// template code

	template< typename Interface >
	CComPtr< Interface > CWinExplorer::BindFileTo( IShellFolder* pFolder, const TCHAR* pFnameExt ) const
	{
		ASSERT_PTR( pFolder );
		CComPtr< Interface > pInterface;

		CComHeapPtr< ITEMIDLIST > pidlFile;
		if ( ParsePidl( &pidlFile, pFolder, pFnameExt ) )
		{
			LPITEMIDLIST p_pidlFile = pidlFile;			// (!) pointer to pointer
			Good( pFolder->GetUIObjectOf( NULL, 1, (PCUITEMID_CHILD_ARRAY)&p_pidlFile, __uuidof( Interface ), NULL, (void**)&pInterface ) );
		}
		return pInterface;
	}

} //namespace shell


#endif // WinExplorer_h
