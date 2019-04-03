#ifndef Recycler_h
#define Recycler_h
#pragma once

#include <shlobj.h>
#include "ShellTypes.h"


namespace shell
{
	class CRecycler
	{
	public:
		CRecycler( void ) : m_pRecyclerItem( GetRecycleBinShellItem() ) { ASSERT_PTR( m_pRecyclerItem ); }

		static CComPtr< IShellItem2 > GetRecycleBinShellItem( void );
		static CComPtr< IShellFolder2 > GetRecycleBinFolder( void );

		IShellItem2* GetItem( void ) const { return m_pRecyclerItem; }
		IShellFolder2* GetFolder( void ) const { return ToShellFolder( m_pRecyclerItem ); }

		CComPtr< IEnumShellItems > GetEnumItems( void ) const;

		// caller must release the IShellItem2 item interfaces returned
		void QueryRecycledItems( std::vector< IShellItem2* >& rRecycledItems, const TCHAR* pOrigPrefixOrSpec, path::SpecMatch minMatch = path::Match_Any ) const;
		void QueryMultiRecycledItems( std::vector< IShellItem2* >& rRecycledItems, const std::vector< fs::CPath >& delFilePaths ) const;		// returns 1 recycled item per file-path (or NULL if not recycled)
		IShellItem2* FindRecycledItem( const fs::CPath& delFilePath ) const;

		bool UndeleteFile( const fs::CPath& delFilePath, CWnd* pWndOwner );
		size_t UndeleteMultiFiles( const std::vector< fs::CPath >& delFilePaths, CWnd* pWndOwner, std::vector< fs::CPath >* pErrorFilePaths = NULL );

		static bool UndeleteItem( IShellItem2* pRecycledItem, CWnd* pWndOwner );

		static bool EmptyRecycleBin( HWND hWndOwner, const TCHAR* pRootPath = NULL, DWORD flags = 0 ) { return HR_OK( ::SHEmptyRecycleBin( hWndOwner, pRootPath, flags ) ); }
	public:
		static fs::CPath GetOriginalFilePath( IShellItem* pRecycledItem ) { ASSERT_PTR( pRecycledItem ); return shell::GetDisplayName( pRecycledItem, SIGDN_NORMALDISPLAY ); }
		static CTime GetDateDeleted( IShellItem2* pRecycledItem ) { ASSERT_PTR( pRecycledItem ); return shell::GetDateTimeProperty( pRecycledItem, PK_DateDeleted ); }

		static path::SpecMatch OriginalPathMatchesPrefix( IShellItem* pRecycledItem, const TCHAR* pOrigPrefixOrSpec );

		static void QueryAvailableDrives( std::vector< std::tstring >& rDriveRootPaths );
		static void QueryDrivesWithRecycledItems( std::vector< std::tstring >& rDriveRootPaths );		// drive root paths that have deleted files in Recycle Bin

		// pRootPath could be any directory path
		static bool QueryRecycleBin( const TCHAR* pRootPath, size_t& rItemCount, ULONGLONG* pTotalSize = NULL );
		static size_t FindRecycledItemCount( const TCHAR* pRootPath );
	private:
		CComPtr< IShellItem2 > m_pRecyclerItem;
	public:
		static const PROPERTYKEY PK_Name;
		static const PROPERTYKEY PK_Type;
		static const PROPERTYKEY PK_Size;
		static const PROPERTYKEY PK_WriteTime;
		static const PROPERTYKEY PK_CreateTime;
		static const PROPERTYKEY PK_AccessTime;
		static const PROPERTYKEY PK_Attributes;

		static const PROPERTYKEY PK_OriginalLocation;
		static const PROPERTYKEY PK_DateDeleted;
	};
}


#endif // Recycler_h
