#ifndef ShellTypes_h
#define ShellTypes_h
#pragma once

//#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <atlbase.h>
#include "Path.h"


namespace shell
{
	// shell folder
	CComPtr< IShellFolder > GetDesktopFolder( void );
	CComPtr< IShellFolder > FindShellFolder( const TCHAR* pDirPath );

	// shell item
	CComPtr< IShellFolder2 > ToShellFolder( IShellItem* pFolderItem );
	CComPtr< IShellFolder2 > GetParentFolderAndPidl( ITEMID_CHILD** pPidlItem, IShellItem* pShellItem );
}


namespace shell
{
	template< typename PathContainerT >
	CComPtr< IShellFolder > MakeRelativePidlArray( std::vector< PIDLIST_RELATIVE >& rPidlItemsArray, const PathContainerT& filePaths );		// for mixed files having a common ancestor folder; caller must delete the PIDLs

	template< typename ContainerT >
	void ClearOwningPidls( ContainerT& rPidls )			// container of pointers to PIDLs, such as std::vector< LPITEMIDLIST >
	{
		std::for_each( rPidls.begin(), rPidls.end(), &pidl::Delete );
		rPidls.clear();
	}


	namespace pidl
	{
		// MSDN doc - Windows 2000+: CoTaskMemAlloc()/CoTaskMemFree() are preferred over SHAlloc()/ILFree()
		inline LPITEMIDLIST Allocate( size_t size ) { return static_cast< LPITEMIDLIST >( ::CoTaskMemAlloc( static_cast< ULONG >( size ) ) ); }
		inline void Delete( LPITEMIDLIST pidl ) { ::CoTaskMemFree( pidl ); }

		PIDLIST_RELATIVE GetRelativeItem( IShellFolder* pFolder, const TCHAR itemFilename[] );		// itemFilename is normally a fname.ext; also works with a sub-path
		fs::CPath GetAbsolutePath( PIDLIST_ABSOLUTE pidlAbsolute, GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT );
		std::tstring GetName( LPCITEMIDLIST pidl, SIGDN nameType = SIGDN_NORMALDISPLAY );

		inline bool IsNull( LPCITEMIDLIST pidl ) { return NULL == pidl; }
		inline bool IsEmpty( LPCITEMIDLIST pidl ) { return IsNull( pidl ) || 0 == pidl->mkid.cb; }
		size_t GetCount( LPCITEMIDLIST pidl );
		size_t GetByteSize( LPCITEMIDLIST pidl );

		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl );
		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl );

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl );
		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl );

		inline bool IsEqual( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl ) { return ::ILIsEqual( leftPidl, rightPidl ) != FALSE; }
		pred::CompareResult Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder = GetDesktopFolder() );


		namespace impl
		{
			inline BYTE* GetBuffer( LPITEMIDLIST pidl ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast< BYTE* >( pidl ); }
			inline const BYTE* GetBuffer( LPCITEMIDLIST pidl ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast< const BYTE* >( pidl ); }

			inline WORD* GetTerminator( LPITEMIDLIST pidl, size_t size ) { ASSERT( !IsNull( pidl ) ); return reinterpret_cast< WORD* >( GetBuffer( pidl ) + size ); }
			inline void SetTerminator( LPITEMIDLIST pidl, size_t size ) { *GetTerminator( pidl, size ) = 0; }
		}
	}
}


namespace shell
{
	// manages memory for a PIDL used in shell API; similar to CComHeapPtr< ITEMIDLIST > with PIDL specific functionality.
	//
	class CPidl
	{
	public:
		CPidl( void ) : m_pidl( NULL ) {}
		CPidl( CPidl& rRight ) : m_pidl( rRight.Release() ) {}													// copy-move constructor
		explicit CPidl( LPITEMIDLIST pidl ) : m_pidl( pidl ) {}													// takes ownership of pidl
		explicit CPidl( LPCITEMIDLIST rootPidl, LPCITEMIDLIST dirPathPidl, LPCITEMIDLIST childPidl = NULL );	// creates a copy with concatenation
		~CPidl() { Delete(); }

		bool IsNull( void ) const { return NULL == m_pidl; }
		bool IsEmpty( void ) const { return pidl::IsEmpty( m_pidl ); }						// note: desktop PIDL is empty (but not null)
		size_t GetCount( void ) const { return pidl::GetCount( m_pidl ); }
		size_t GetByteSize( void ) const { return pidl::GetByteSize( m_pidl ); }
		size_t GetHash( void ) const;

		LPITEMIDLIST Get( void ) const { return m_pidl; }

		LPITEMIDLIST* operator&()
		{
			Delete();
			return &m_pidl;
		}

		void Reset( LPITEMIDLIST pidl = NULL )
		{
			Delete();
			m_pidl = pidl;
		}

		LPITEMIDLIST Release( void )
		{
			LPITEMIDLIST pidl = m_pidl;
			m_pidl = NULL;
			return pidl;
		}

		void Delete( void )
		{
			if ( m_pidl != NULL )
				pidl::Delete( Release() );
		}

		void AssignCopy( LPCITEMIDLIST pidl ) { Reset( pidl::Copy( pidl ) ); }

		LPCITEMIDLIST GetNextItem( void ) const { return pidl::GetNextItem( m_pidl ); }
		LPCITEMIDLIST GetLastItem( void ) const { return pidl::GetLastItem( m_pidl ); }

		void Concatenate( LPCITEMIDLIST rightPidl );
		void ConcatenateChild( LPCITEMIDLIST childPidl );
		void RemoveLast( void );

		// absolute PIDL compare
		bool operator==( const CPidl& rightPidl ) const { return pidl::IsEqual( m_pidl, rightPidl.m_pidl ); }
		bool operator!=( const CPidl& rightPidl ) const { return !operator==( rightPidl ); }

		// relative PIDL compare
		pred::CompareResult Compare( const CPidl& rightRelativePidl, IShellFolder* pParentFolder = GetDesktopFolder() ) { return pidl::Compare( m_pidl, rightRelativePidl.m_pidl, pParentFolder ); }

		// advanced
		bool CreateAbsolute( const TCHAR fullPath[] );								// ABSOLUTE pidl
		bool CreateRelative( IShellFolder* pFolder, const TCHAR itemFilename[] );	// RELATIVE/CHILD pidl, depending on itemFilename being a sub-path or fname.ext

		bool CreateFrom( IUnknown* pUnknown );										// ABSOLUTE pidl - most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		bool CreateFromFolder( IShellFolder* pShellFolder );						// ABSOLUTE pidl - superseeded by CreateFrom()

		CComPtr< IShellItem > FindItem( IShellFolder* pParentFolder = NULL ) const;			// pass pParentFolder if PIDL is relative/child
		CComPtr< IShellFolder > FindFolder( IShellFolder* pParentFolder = GetDesktopFolder() ) const;

		fs::CPath GetAbsolutePath( GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT ) const { return pidl::GetAbsolutePath( m_pidl, optFlags ); }
		std::tstring GetName( SIGDN nameType = SIGDN_NORMALDISPLAY ) const { return pidl::GetName( m_pidl, nameType ); }

		bool WriteToStream( IStream* pStream ) const;
		bool ReadFromStream( IStream* pStream );
	private:
		const BYTE* GetBuffer( void ) const { return pidl::impl::GetBuffer( m_pidl ); }
		BYTE* GetBuffer( void ) { return pidl::impl::GetBuffer( m_pidl ); }
	private:
		LPITEMIDLIST m_pidl;
	};
}


#include "Image_fwd.h"


namespace shell
{
	std::tstring FormatByteSize( UINT64 fileSize );
	std::tstring FormatKiloByteSize( UINT64 fileSize );

	CImageList* GetSysImageList( ui::GlyphGauge glyphGauge );

	int GetFileSysImageIndex( const TCHAR* filePath );
	HICON GetFileSysIcon( const TCHAR* filePath );			// returns a copy of the icon (client must delete it)


	class CSysImageLists
	{
		CSysImageLists( void ) {}
		~CSysImageLists();
	public:
		static CSysImageLists& Instance( void );

		CImageList* Get( ui::GlyphGauge glyphGauge );
	private:
		CImageList m_imageLists[ ui::_GlyphGaugeCount ];
	};
}


namespace shell
{
	// template code

	template< typename PathContainerT >
	CComPtr< IShellFolder > MakeRelativePidlArray( std::vector< PIDLIST_RELATIVE >& rPidlItemsArray, const PathContainerT& filePaths )
	{	// for mixed files having a common ancestor folder
		CComPtr< IShellFolder > pCommonFolder;
		fs::CPath commonDirPath = path::ExtractCommonParentPath( filePaths );

		rPidlItemsArray.reserve( filePaths.size() );
		if ( !commonDirPath.IsEmpty() )
		{
			pCommonFolder = FindShellFolder( commonDirPath.GetPtr() );
			if ( pCommonFolder != NULL )
				for ( typename PathContainerT::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
				{
					std::tstring relativePath = func::StringOf( *itFilePath );
					path::StripPrefix( relativePath, commonDirPath.GetPtr() );

					if ( PIDLIST_RELATIVE pidlRelative = pidl::GetRelativeItem( pCommonFolder, relativePath.c_str() ) )
						rPidlItemsArray.push_back( pidlRelative );			// caller must free the pidl
				}
		}

		return pCommonFolder;
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

	typedef LessValue< CompareExplorerPath > TLess_ExplorerPath;
}


namespace shell
{
	// path sort for std::tstring, fs::CPath, fs::CFlexPath

	template< typename IteratorT >
	inline void SortPaths( IteratorT itFirst, IteratorT itLast, bool ascending = true )
	{
		std::sort( itFirst, itLast, pred::OrderByValue< pred::CompareExplorerPath >( ascending ) );
	}

	template< typename ContainerT >
	inline void SortPaths( ContainerT& rStrPaths, bool ascending = true )
	{
		shell::SortPaths( rStrPaths.begin(), rStrPaths.end(), ascending );
	}
}


#endif // ShellTypes_h
