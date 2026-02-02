#ifndef ShellTypes_h
#define ShellTypes_h
#pragma once

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <atlbase.h>
#include "Path.h"


class CFlagTags;


namespace utl
{
	size_t HashBytes( const void* pFirst, size_t count );	// FWD
}


#define AUTO_BIND_CTX			reinterpret_cast<IBindCtx*>( -1 )
#define AUTO_FOLDER_BIND_CTX	reinterpret_cast<IBindCtx*>( -2 )


namespace shell
{
	// shell folder
	CComPtr<IShellFolder> GetDesktopFolder( void );

	CComPtr<IShellFolder> MakeShellFolder( const TCHAR* pFolderShellPath );
	PIDLIST_ABSOLUTE MakeFolderPidl( IShellFolder* pFolder );
	std::tstring GetFolderName( IShellFolder* pFolder, SIGDN nameType = SIGDN_NORMALDISPLAY );
	shell::TPath GetFolderPath( IShellFolder* pFolder );

	// shell item
	CComPtr<IShellItem> MakeShellItem( const TCHAR* pShellPath );
	CComPtr<IShellFolder2> ToShellFolder( IShellItem* pFolderItem );
	CComPtr<IShellFolder2> GetParentFolderAndPidl( OUT PITEMID_CHILD* pPidlItem, IShellItem* pShellItem );
	PIDLIST_ABSOLUTE MakeItemPidl( IShellItem* pShellItem );


	CComPtr<IBindCtx> CreateFileSysBindContext( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL );			// virtual bind context for FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY

	CComPtr<IBindCtx> AutoFileSysBindContext( const TCHAR* pShellPath, IBindCtx* pBindCtx = AUTO_BIND_CTX );	// adapted to the pShellPath type
}


namespace shell
{
	template< typename PathT >
	bool IsGuidPath( const PathT& shellPath ) { return path::IsValidGuidPath( func::StringOf( shellPath ) ); }

	// shell file info (via SHFILEINFO)
	SFGAOF GetFileSysAttributes( const TCHAR* pFilePath, UINT moreFlags = 0 );
	std::pair<CImageList*, int> GetFileSysImageIndex( const TCHAR* pFilePath, UINT iconFlags = SHGFI_SMALLICON );	// CImageList is a shared shell temporary object (no ownership)
	HICON ExtractFileSysIcon( const TCHAR* pFilePath, UINT flags = SHGFI_SMALLICON );		// returns a copy of the icon (caller must delete it)

	UINT GetSysIconSizeFlag( int iconDimension );

	// shell properties
	std::tstring GetString( STRRET* pStrRet, PCUITEMID_CHILD pidl = nullptr );


	// IShellFolder properties
	std::tstring GetFolderChildDisplayName( IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF flags = SHGDN_NORMAL );

	// IShellFolder2 properties
	std::tstring GetStringDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey );
	CTime GetDateTimeDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey );


	// IShellItem properties
	std::tstring GetItemDisplayName( IShellItem* pItem, SIGDN nameType );
	inline std::tstring GetItemName( IShellItem* pItem ) { return GetItemDisplayName( pItem, SIGDN_NORMALDISPLAY ); }
	inline shell::TPath GetItemShellPath( IShellItem* pItem ) { return GetItemDisplayName( pItem, SIGDN_DESKTOPABSOLUTEPARSING /*SIGDN_FILESYSPATH*/ ); }
	inline fs::CPath GetItemFileSysPath( IShellItem* pItem ) { return GetItemDisplayName( pItem, SIGDN_FILESYSPATH ); }		// useful for CRecycler

	// IShellItem2 properties
	std::tstring GetStringProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	CTime GetDateTimeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	DWORD GetFileAttributesProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );
	ULONGLONG GetFileSizeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey );

#ifdef USE_UT
	const CFlagTags& GetTags_SFGAO_Flags( void );
#endif
}


namespace shell
{
	class CPidlAbsolute;

	PIDLIST_ABSOLUTE MakePidl( const shell::TPath& shellPath, DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL );
	bool MakePidl( OUT CPidlAbsolute& rPidl, const shell::TPath& shellPath, DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL );

	shell::TPath MakeShellPath( PCIDLIST_ABSOLUTE pidlAbs );
	shell::TPath MakeShellPath( const CPidlAbsolute& pidl );


	namespace pidl
	{
		template< typename ConstPidlArrayT, typename SrcPidlT >
		inline ConstPidlArrayT AsPidlArray( const std::vector<SrcPidlT>& srcPidls ) { return reinterpret_cast<ConstPidlArrayT>( utl::Data( srcPidls ) ); }

		// MSDN doc - Windows 2000+: CoTaskMemAlloc()/CoTaskMemFree() are preferred over SHAlloc()/ILFree()
		inline LPITEMIDLIST Allocate( size_t size ) { return static_cast<LPITEMIDLIST>( ::CoTaskMemAlloc( static_cast<ULONG>( size ) ) ); }
		inline void Delete( LPITEMIDLIST pidl ) { ::CoTaskMemFree( pidl ); }

		bool IsSpecialPidl( LPCITEMIDLIST pidl );		// refers to a special shell namespace directory; pidl could be Absolute/Relative

		SFGAOF GetPidlAttributes( PCIDLIST_ABSOLUTE pidl );
		std::pair<CImageList*, int> GetPidlImageIndex( PCIDLIST_ABSOLUTE pidl, UINT iconFlags = SHGFI_SMALLICON );	// CImageList is a shared shell temporary object (no ownership)
		HICON ExtractPidlIcon( PCIDLIST_ABSOLUTE pidl, UINT iconFlags = SHGFI_SMALLICON );		// returns a copy of the icon (caller must delete it)

		template< typename PtrItemIdListT >
		inline PtrItemIdListT CreateEmptyPidl( void )
		{
			PtrItemIdListT pidlEmpty = reinterpret_cast<PtrItemIdListT>( Allocate( sizeof( _SHITEMID::cb ) ) );		// allocate 2 bytes for the null terminator: "_SHITEMID::USHORT cb;"
			if ( pidlEmpty != nullptr )
				pidlEmpty->mkid.cb = 0;		// set size to zero to mark it as empty
			return pidlEmpty;
		}

		// format pidl:
		std::tstring GetName( PCIDLIST_ABSOLUTE pidl, SIGDN nameType = SIGDN_NORMALDISPLAY );
		shell::TPath GetShellPath( PCIDLIST_ABSOLUTE pidlAbsolute );											// shell path: either file-system or GUID - aka SIGDN_DESKTOPABSOLUTEPARSING
		fs::CPath GetAbsolutePath( PCIDLIST_ABSOLUTE pidlAbsolute, GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT );	// file-system path only

		// for strictly file path
		inline PIDLIST_ABSOLUTE CreateFromPath( const TCHAR* pFullPath ) { return ::ILCreateFromPath( pFullPath ); }

		// shell path: FilePath or GuidPath - parse to PIDL (caller must free the pidl) - expands any environment variables:
		PIDLIST_ABSOLUTE ParseToPidl( const TCHAR* pShellPath, IBindCtx* pBindCtx = nullptr );
		PIDLIST_ABSOLUTE ParseToPidlFileSys( const TCHAR* pShellPath, DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL );		// parses inexistent paths via shell::CreateFileSysBindContext()
		PIDLIST_RELATIVE ParseToPidlRelative( const TCHAR* pRelShellPath, IShellFolder* pParentFolder, IBindCtx* pBindCtx = nullptr );
		PIDLIST_ABSOLUTE ParseToPidlAbsolute( const TCHAR* pRelShellPath, IShellFolder* pParentFolder, IBindCtx* pBindCtx = nullptr );

		PIDLIST_RELATIVE ParseToRelativePidl( IShellFolder* pParentFolder, const TCHAR* pRelShellPath, IBindCtx* pBindCtx = nullptr );	// pRelShellPath is normally a fname.ext, but it also works with a deep relative path.
		PITEMID_CHILD ParseToChildPidl( IShellFolder* pParentFolder, const TCHAR* pFilename, IBindCtx* pBindCtx = nullptr );

		inline bool IsNull( LPCITEMIDLIST pidl ) { return nullptr == pidl; }
		inline bool IsEmpty( LPCITEMIDLIST pidl ) { return IsNull( pidl ) || 0 == pidl->mkid.cb; }
		inline bool IsEmptyStrict( LPCITEMIDLIST pidl ) { return 0 == pidl->mkid.cb; }
		size_t GetItemCount( LPCITEMIDLIST pidl );
		size_t GetByteSize( LPCITEMIDLIST pidl );

		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl );	// duplicate of ::ILNext()
		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl );	// duplicate of ::ILFindLastID()

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl );			// duplicate of ::ILClone(), ::ILCloneFull(), ::ILCloneChild()
		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl );	// duplicate of ::ILCloneFirst()

		bool IsEqual( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl );	// works for null pidls as well
		pred::CompareResult Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder = GetDesktopFolder() );

		// common ancestor PIDL
		PIDLIST_ABSOLUTE GetCommonAncestor( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl );		// find the common ancestor pidl of two absolute PIDLs
		PIDLIST_ABSOLUTE ExtractCommonAncestorPidl( PCIDLIST_ABSOLUTE_ARRAY pAbsPidls, size_t count );		// find the common ancestor pidl of multiple absolute PIDLs

		template< typename PidlAbsoluteT >
		PIDLIST_ABSOLUTE ExtractCommonAncestorPidl( const std::vector<PidlAbsoluteT>& absPidls )
		{
			return ExtractCommonAncestorPidl( AsPidlArray<PCIDLIST_ABSOLUTE_ARRAY>( absPidls ), absPidls.size() );
		}

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
	struct DeleteComHeap		// works with TCHAR*, PIDLIST_ABSOLUTE, PIDLIST_RELATIVE, PITEMID_CHILD, etc
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
	template< typename PathContainerT >		// e.g. vector<shell::TPath>, vector<std::tstring>, vector<const TCHAR*>
	void CreateAbsolutePidls( OUT std::vector<PIDLIST_ABSOLUTE>& rOutPidls, const PathContainerT& shellPaths )
	{
		REQUIRE( rOutPidls.empty() );

		for ( typename PathContainerT::const_iterator itShellPath = shellPaths.begin(); itShellPath != shellPaths.end(); ++itShellPath )
			if ( PIDLIST_ABSOLUTE pidlAbs = pidl::ParseToPidlFileSys( str::traits::GetCharPtr( *itShellPath ) ) )
				rOutPidls.push_back( pidlAbs );

		// Note: for input virtual folders (like Control Panel applets), it only succeeds with a custom bind context via CreateFileSysBindContext!
		// That's why we call ParseToPidlFileSys() with FILE_ATTRIBUTE_NORMAL, so it succeeds in parsing the GUID paths.
	}


	template< typename ContainerT >
	void ClearOwningPidls( IN OUT ContainerT& rPidls )			// container of pointers to PIDLs, such as std::vector<LPITEMIDLIST>
	{
		std::for_each( rPidls.begin(), rPidls.end(), func::TDeletePidl() );
		rPidls.clear();
	}

	template< typename ShellItemContainerT >
	void QueryShellPaths( OUT std::vector<shell::TPath>& rShellPaths, const ShellItemContainerT& shellItems )
	{
		rShellPaths.reserve( rShellPaths.size() + shellItems.size() );
		utl::transform( shellItems, rShellPaths, &shell::GetItemShellPath );		// SIGDN_DESKTOPABSOLUTEPARSING
	}


	// IShellItemArray to path conversion

	void QueryShellItemArrayPaths( OUT std::vector<shell::TPath>& rFilePaths, IShellItemArray* pSrcShellItemArray );

	template< typename ShellItemContainerT >
	CComPtr<IShellItemArray> MakeShellItemArray( const ShellItemContainerT& shellItems )
	{
		std::vector<PIDLIST_ABSOLUTE> itemAbsPidls;
		utl::transform( shellItems, itemAbsPidls, &shell::MakeItemPidl );

		CComPtr<IShellItemArray> pShellItemArray;
		HR_AUDIT( ::SHCreateShellItemArrayFromIDLists( static_cast<UINT>( itemAbsPidls.size() ), pidl::AsPidlArray<PCIDLIST_ABSOLUTE_ARRAY>( itemAbsPidls ), &pShellItemArray ) );

		ClearOwningPidls( itemAbsPidls );
		return pShellItemArray;
	}
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
