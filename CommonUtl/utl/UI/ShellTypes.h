#ifndef ShellTypes_h
#define ShellTypes_h
#pragma once

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <atlbase.h>
#include "Path.h"


namespace utl
{
	size_t HashBytes( const void* pFirst, size_t count );	// FWD
}


namespace shell
{
	// shell folder
	CComPtr<IShellFolder> GetDesktopFolder( void );
	CComPtr<IShellFolder> FindShellFolder( const TCHAR* pDirPath );

	// shell item
	CComPtr<IShellItem> FindShellItem( const fs::CPath& fullPath );
	CComPtr<IShellFolder2> ToShellFolder( IShellItem* pFolderItem );
	CComPtr<IShellFolder2> GetParentFolderAndPidl( ITEMID_CHILD** pPidlItem, IShellItem* pShellItem );

	CComPtr<IBindCtx> CreateFileSysBindContext( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL );		// virtual bind context for FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY
}


namespace shell
{
	// shell file info (via SHFILEINFO)
	SFGAOF GetFileSysAttributes( const TCHAR* pFilePath );
	int GetFileSysImageIndex( const TCHAR* pFilePath );
	HICON GetFileSysIcon( const TCHAR* pFilePath, UINT flags = SHGFI_SMALLICON );		// returns a copy of the icon (caller must delete it)


	// shell properties
	std::tstring GetString( STRRET* pStrRet, PCUITEMID_CHILD pidl = nullptr );


	// IShellFolder properties
	std::tstring GetDisplayName( IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF flags = SHGDN_NORMAL );

	// IShellFolder2 properties
	std::tstring GetStringDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey );
	CTime GetDateTimeDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey );


	// IShellItem properties
	std::tstring GetDisplayName( IShellItem* pItem, SIGDN sigdn );
	inline std::tstring GetName( IShellItem* pItem ) { return GetDisplayName( pItem, SIGDN_NORMALDISPLAY ); }
	inline fs::CPath GetFilePath( IShellItem* pItem ) { return GetDisplayName( pItem, SIGDN_FILESYSPATH ); }
	inline fs::CPath GetRecycledPath( IShellItem* pRecycledItem ) { return GetFilePath( pRecycledItem ); }

	inline bool IsGuidPath( const std::tstring& strPath ) { return path::IsValidGuidPath( strPath ); }

	// IShellItem2 properties
	std::tstring GetStringProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	CTime GetDateTimeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	DWORD GetFileAttributesProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	ULONGLONG GetFileSizeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
}


namespace shell
{
	namespace pidl
	{
		// MSDN doc - Windows 2000+: CoTaskMemAlloc()/CoTaskMemFree() are preferred over SHAlloc()/ILFree()
		inline LPITEMIDLIST Allocate( size_t size ) { return static_cast<LPITEMIDLIST>( ::CoTaskMemAlloc( static_cast<ULONG>( size ) ) ); }
		inline void Delete( LPITEMIDLIST pidl ) { ::CoTaskMemFree( pidl ); }

		template< typename PtrItemIdListT >
		inline PtrItemIdListT CreateEmptyPidl( void )
		{
			PtrItemIdListT pidlEmpty = reinterpret_cast<PtrItemIdListT>( Allocate( sizeof(_SHITEMID::cb) ) );		// allocate 2 bytes for the null terminator: "_SHITEMID::USHORT cb;"
			if ( pidlEmpty != nullptr )
				pidlEmpty->mkid.cb = 0;		// set size to zero to mark it as empty
			return pidlEmpty;
		}

		PIDLIST_RELATIVE CreateRelativePidl( IShellFolder* pParentFolder, const TCHAR* pRelPath );		// pRelPath is normally a fname.ext; also works with a relative path (sub-path)
		inline PUITEMID_CHILD CreateChilePidl( IShellFolder* pParentFolder, const TCHAR* pFnameExt ) { return reinterpret_cast<PUITEMID_CHILD>( CreateRelativePidl( pParentFolder, pFnameExt ) ); }

		// format pidl:
		fs::CPath GetAbsolutePath( PCIDLIST_ABSOLUTE pidlAbsolute, GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT );
		std::tstring GetName( PCIDLIST_ABSOLUTE pidl, SIGDN nameType = SIGDN_NORMALDISPLAY );

		// parse to absolute pidl:
		PIDLIST_ABSOLUTE ParseToPidl( const TCHAR* pPathOrName, IBindCtx* pBindCtx = nullptr );
		PIDLIST_RELATIVE ParseToPidlRelative( const TCHAR* pRelPathOrName, IShellFolder* pParentFolder, IBindCtx* pBindCtx = nullptr );
		PIDLIST_ABSOLUTE ParseToPidlAbsolute( const TCHAR* pRelPathOrName, IShellFolder* pParentFolder, IBindCtx* pBindCtx = nullptr );

		inline bool IsNull( LPCITEMIDLIST pidl ) { return nullptr == pidl; }
		inline bool IsEmpty( LPCITEMIDLIST pidl ) { return IsNull( pidl ) || 0 == pidl->mkid.cb; }
		inline bool IsEmptyStrict( LPCITEMIDLIST pidl ) { return 0 == pidl->mkid.cb; }
		size_t GetItemCount( LPCITEMIDLIST pidl );
		size_t GetByteSize( LPCITEMIDLIST pidl );

		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl );	// duplicate of ::ILNext()
		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl );	// duplicate of ::ILFindLastID()

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl );			// duplicate of ::ILClone(), ::ILCloneFull(), ::ILCloneChild()
		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl );	// duplicate of ::ILCloneFirst()

		inline bool IsEqual( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl ) { return ::ILIsEqual( leftPidl, rightPidl ) != FALSE; }
		pred::CompareResult Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder = GetDesktopFolder() );


		namespace impl
		{
			inline BYTE* GetBuffer( LPITEMIDLIST pidl ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast<BYTE*>( pidl ); }
			inline const BYTE* GetBuffer( LPCITEMIDLIST pidl ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast<const BYTE*>( pidl ); }

			inline WORD* GetTerminator( LPITEMIDLIST pidl, size_t size ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast<WORD*>( GetBuffer( pidl ) + size ); }
			inline void SetTerminator( LPITEMIDLIST pidl, size_t size ) { *GetTerminator( pidl, size ) = 0; }
		}
	}
}


namespace func
{
	struct DeleteComHeap		// works with PIDLIST_ABSOLUTE, PIDLIST_RELATIVE, PITEMID_CHILD, etc
	{
		void operator()( void* pHeapPtr ) const
		{
			::CoTaskMemFree( pHeapPtr );
		}
	};

	typedef DeleteComHeap TDeletePidl;
}


namespace shell
{
	template< typename PathContainerT >
	CComPtr<IShellFolder> MakeRelativePidlArray( std::vector<PIDLIST_RELATIVE>& rPidlItemsArray, const PathContainerT& filePaths );		// for mixed files having a common ancestor folder; caller must delete the PIDLs

	template< typename ShellItemContainerT >		// ShellItemContainerT examples: std::vector<CComPtrIShellItem2>, std::vector<IShellItem*>, etc
	void MakeAbsolutePidlArray( std::vector<PIDLIST_ABSOLUTE>& rPidlVector, const ShellItemContainerT& shellItems );

	template< typename ContainerT >
	void ClearOwningPidls( ContainerT& rPidls )			// container of pointers to PIDLs, such as std::vector<LPITEMIDLIST>
	{
		std::for_each( rPidls.begin(), rPidls.end(), func::TDeletePidl() );
		rPidls.clear();
	}

	template< typename ShellItemContainerT >
	void QueryFilePaths( std::vector<fs::CPath>& rFilePaths, const ShellItemContainerT& shellItems, SIGDN sigdn = SIGDN_FILESYSPATH )
	{
		rFilePaths.reserve( rFilePaths.size() + shellItems.size() );

		for ( typename ShellItemContainerT::const_iterator itShellItem = shellItems.begin(); itShellItem != shellItems.end(); ++itShellItem )
			rFilePaths.push_back( shell::GetDisplayName( *itShellItem, sigdn ) );
	}

	template< typename ShellItemContainerT >
	CComPtr<IShellItemArray> MakeShellItemArray( const ShellItemContainerT& shellItems );
}


#include "Image_fwd.h"


namespace shell
{
	std::tstring FormatByteSize( UINT64 fileSize );
	std::tstring FormatKiloByteSize( UINT64 fileSize );

	CImageList* GetSysImageList( ui::GlyphGauge glyphGauge );


	class CSysImageLists
	{
		CSysImageLists( void ) {}
		~CSysImageLists();
	public:
		static CSysImageLists& Instance( void );

		CImageList* Get( ui::GlyphGauge glyphGauge );
	private:
		CImageList m_imageLists[ ui::_GlyphGaugeCount ];	// image-lists are owned by Explorer!
	};
}


namespace shell
{
	// template code

	template< typename PathContainerT >
	CComPtr<IShellFolder> MakeRelativePidlArray( std::vector<PIDLIST_RELATIVE>& rPidlItemsArray, const PathContainerT& filePaths )
	{	// for mixed files having a common ancestor folder
		CComPtr<IShellFolder> pCommonFolder;
		fs::CPath commonDirPath = path::ExtractCommonParentPath( filePaths );

		rPidlItemsArray.reserve( filePaths.size() );
		if ( !commonDirPath.IsEmpty() )
		{
			pCommonFolder = FindShellFolder( commonDirPath.GetPtr() );
			if ( pCommonFolder != nullptr )
				for ( typename PathContainerT::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
				{
					std::tstring relativePath = func::StringOf( *itFilePath );
					path::StripPrefix( relativePath, commonDirPath.GetPtr() );

					if ( PIDLIST_RELATIVE pidlRelative = pidl::CreateRelativePidl( pCommonFolder, relativePath.c_str() ) )
						rPidlItemsArray.push_back( pidlRelative );			// caller must free the pidl
				}
		}

		return pCommonFolder;
	}

	template< typename ShellItemContainerT >		// ShellItemContainerT examples: std::vector<CComPtrIShellItem2>, std::vector<IShellItem*>, etc
	void MakeAbsolutePidlArray( std::vector<PIDLIST_ABSOLUTE>& rPidlVector, const ShellItemContainerT& shellItems )
	{
		REQUIRE( rPidlVector.empty() );

		// caller must delete the allocated PIDLs:
		//	utl::for_each( pidlVector, func::DeleteComHeap() );

		rPidlVector.reserve( shellItems.size() );

		for ( typename ShellItemContainerT::const_iterator itShellItem = shellItems.begin(); itShellItem != shellItems.end(); ++itShellItem )
		{
			ASSERT_PTR( *itShellItem );

			PIDLIST_ABSOLUTE pidl;
			ASSERT( HR_OK( ::SHGetIDListFromObject( *itShellItem, &pidl ) ) );
			ASSERT_PTR( pidl );
			rPidlVector.push_back( pidl );
		}
	}

	template< typename ShellItemContainerT >
	CComPtr<IShellItemArray> MakeShellItemArray( const ShellItemContainerT& shellItems )
	{
		std::vector<PIDLIST_ABSOLUTE> pidlVector;
		MakeAbsolutePidlArray( pidlVector, shellItems );

		CComPtr<IShellItemArray> pShellItemArray;
		ASSERT( HR_OK( ::SHCreateShellItemArrayFromIDLists( static_cast<UINT>( pidlVector.size() ), (PCIDLIST_ABSOLUTE_ARRAY)&pidlVector.front(), &pShellItemArray ) ) );

		ClearOwningPidls( pidlVector );
		return pShellItemArray;
	}
}


namespace shell
{
	// for sorting in Explorer.exe order (which varies with Windows version), based on ::StrCmpLogicalW
	// note: it doesn't translate '/' to '\\'

	pred::CompareResult ExplorerCompare( const wchar_t* pLeftPath, const wchar_t* pRightPath );
}


namespace pred
{
	struct CompareExplorerPath
	{
		pred::CompareResult operator()( const wchar_t* pLeftPath, const wchar_t* pRightPath ) const
		{
			return shell::ExplorerCompare( pLeftPath, pRightPath );
		}

		pred::CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			return operator()( leftPath.GetPtr(), rightPath.GetPtr() );
		}
	};

	typedef LessValue<CompareExplorerPath> TLess_ExplorerPath;
}


namespace shell
{
	// path sort for std::tstring, fs::CPath, fs::CFlexPath

	template< typename IteratorT >
	inline void SortPaths( IteratorT itFirst, IteratorT itLast, bool ascending = true )
	{
		std::sort( itFirst, itLast, pred::OrderByValue<pred::CompareExplorerPath>( ascending ) );
	}

	template< typename ContainerT >
	inline void SortPaths( ContainerT& rStrPaths, bool ascending = true )
	{
		shell::SortPaths( rStrPaths.begin(), rStrPaths.end(), ascending );
	}
}


#endif // ShellTypes_h
