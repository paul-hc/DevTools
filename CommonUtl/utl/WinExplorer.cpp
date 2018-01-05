
#include "stdafx.h"
#include "WinExplorer.h"
#include "FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr< IShellFolder > CWinExplorer::GetDesktopFolder( void ) const
	{
		CComPtr< IShellFolder > pDesktopFolder;
		Good( ::SHGetDesktopFolder( &pDesktopFolder ) );
		return pDesktopFolder;
	}

	CComPtr< IShellItem > CWinExplorer::FindShellItem( const fs::CPath& fullPath ) const
	{
		CComPtr< IShellItem > pShellItem;
		Good( ::SHCreateItemFromParsingName( fullPath.GetPtr(), NULL, IID_PPV_ARGS( &pShellItem ) ) );
		return pShellItem;
	}

	bool CWinExplorer::ParsePidl( PIDLIST_RELATIVE* pPidl, IShellFolder* pFolder, const TCHAR* pFnameExt ) const
	{
		ASSERT_PTR( pPidl );
		ASSERT_PTR( pFolder );
		ASSERT_PTR( pFnameExt );

		TCHAR displayName[ MAX_PATH * 2 ];
		_tcscpy( displayName, pFnameExt );

		return Good( pFolder->ParseDisplayName( NULL, NULL, displayName, NULL, pPidl, NULL ) );
	}

	CComPtr< IShellFolder > CWinExplorer::FindShellFolder( const TCHAR* pDirPath ) const
	{
		CComPtr< IShellFolder > pDirFolder;
		if ( fs::IsValidDirectory( pDirPath ) )
			if ( CComPtr< IShellFolder > pDesktopFolder = GetDesktopFolder() )
			{
				CComHeapPtr< ITEMIDLIST > pidlWorkDir;
				if ( ParsePidl( &pidlWorkDir, pDesktopFolder, pDirPath ) )
					Good( pDesktopFolder->BindToObject( pidlWorkDir, NULL, IID_PPV_ARGS( &pDirFolder ) ) );
			}

		return pDirFolder;
	}

	HBITMAP CWinExplorer::ExtractThumbnail( const fs::CPath& filePath, const CSize& boundsSize, DWORD flags /*= 0*/ ) const
	{
		CComPtr< IShellFolder > pDirFolder = FindShellFolder( filePath.GetDirPath().c_str() );
		if ( pDirFolder != NULL )
		{
			CComPtr< IExtractImage > pExtractImage = BindFileTo< IExtractImage >( pDirFolder, filePath.GetNameExt() );
			if ( pExtractImage != NULL )
			{
				// define thumbnail properties
				DWORD priority = IEIT_PRIORITY_NORMAL;
				TCHAR pathBuffer[ MAX_PATH ];
				if ( Good( pExtractImage->GetLocation( pathBuffer, MAX_PATH, &priority, &boundsSize, 16, &flags ) ) )
				{
					// generate thumbnail
					HBITMAP hThumbBitmap = NULL;
					if ( Good( pExtractImage->Extract( &hThumbBitmap ) ) )
						return hThumbBitmap;
				}
			}
		}
		return NULL;
	}

	HBITMAP CWinExplorer::ExtractThumbnail( IShellItem* pShellItem, const CSize& boundsSize, SIIGBF flags /*= SIIGBF_RESIZETOFIT*/ ) const
	{
		HBITMAP hBitmap = NULL;
		CComQIPtr< IShellItemImageFactory > pImageFactory( pShellItem );
		if ( pImageFactory != NULL )
			Good( pImageFactory->GetImage( boundsSize, flags, &hBitmap ) );

		return hBitmap;
	}

} //namespace shell
