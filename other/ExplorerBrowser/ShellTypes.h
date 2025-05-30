#ifndef ShellTypes_h
#define ShellTypes_h
#pragma once

#include <shlobj.h>


namespace shell
{
	// Manages memory for a PIDL used in shell API. Similar to CComHeapPtr<ITEMIDLIST> with PIDL specific functionality.

	class CPidl
	{
	public:
		CPidl( void ) : m_pidl( NULL ) {}
		explicit CPidl( LPITEMIDLIST pidl ) : m_pidl( pidl ) {}													// takes ownership of pidl
		explicit CPidl( LPCITEMIDLIST rootPidl, LPCITEMIDLIST dirPathPidl, LPCITEMIDLIST childPidl = NULL );	// creates a copy with concatenation
		~CPidl() { Delete(); }

		bool IsNull( void ) const { return NULL == m_pidl; }
		bool IsEmpty( void ) const { return Pidl_IsEmpty( m_pidl ); }						// note: desktop PIDL is empty (but not null)
		size_t GetCount( void ) const { return Pidl_GetCount( m_pidl ); }
		size_t GetByteSize( void ) const { return Pidl_GetByteSize( m_pidl ); }
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
				Pidl_Delete( Release() );
		}

		void AssignCopy( LPCITEMIDLIST pidl ) { Reset( Pidl_Copy( pidl ) ); }

		LPCITEMIDLIST GetNextItem( void ) const { return Pidl_GetNextItem( m_pidl ); }
		LPCITEMIDLIST GetLastItem( void ) const { return Pidl_GetLastItem( m_pidl ); }

		void Concatenate( LPCITEMIDLIST rightPidl );
		void ConcatenateChild( LPCITEMIDLIST childPidl );
		void RemoveLast( void );

		// absolute PIDL compare
		bool operator==( const CPidl& rightPidl ) const { return Pidl_IsEqual( m_pidl, rightPidl.m_pidl ); }
		bool operator!=( const CPidl& rightPidl ) const { return !operator==( rightPidl ); }

		// relative PIDL compare
		pred::CompareResult Compare( const CPidl& rightRelativePidl, IShellFolder* pParentFolder = GetDesktopFolder() ) { return Pidl_Compare( m_pidl, rightRelativePidl.m_pidl, pParentFolder ); }

		// advanced
		bool CreateFromPath( const TCHAR fullPath[] );								// ABSOLUTE pidl
		bool CreateAbsolute( const TCHAR fullPath[] );								// ABSOLUTE pidl, synonym to CreateFromPath()
		bool CreateChild( IShellFolder* pFolder, const TCHAR itemFilename[] );		// RELATIVE or CHILD, depending on itemFilename being a sub-path or fname.ext

		bool CreateFrom( IUnknown* pUnknown );										// ABSOLUTE pidl - most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		bool CreateFromFolder( IShellFolder* pShellFolder );						// ABSOLUTE pidl

		CComPtr<IShellItem> FindItem( IShellFolder* pParentFolder = NULL ) const;
		CComPtr<IShellFolder> FindFolder( IShellFolder* pParentFolder = GetDesktopFolder() ) const;

		std::tstring GetFullPath( GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT ) const;
		std::tstring GetName( SIGDN nameType = SIGDN_NORMALDISPLAY ) const;

		bool WriteToStream( IStream* pStream ) const;
		bool ReadFromStream( IStream* pStream );

		// MSDN doc - Windows 2000+: CoTaskMemAlloc()/CoTaskMemFree() are preferred over SHAlloc()/ILFree()
		static LPITEMIDLIST Pidl_Allocate( size_t size ) { return static_cast<LPITEMIDLIST>( ::CoTaskMemAlloc( static_cast<ULONG>( size ) ) ); }
		static void Pidl_Delete( LPITEMIDLIST pidl ) { ::CoTaskMemFree( pidl ); }

		static bool Pidl_IsNull( LPCITEMIDLIST pidl ) { return NULL == pidl; }
		static bool Pidl_IsEmpty( LPCITEMIDLIST pidl ) { return Pidl_IsNull( pidl ) || 0 == pidl->mkid.cb; }
		static size_t Pidl_GetCount( LPCITEMIDLIST pidl );
		static size_t Pidl_GetByteSize( LPCITEMIDLIST pidl );

		static LPCITEMIDLIST Pidl_GetLastItem( LPCITEMIDLIST pidl );
		static LPCITEMIDLIST Pidl_GetNextItem( LPCITEMIDLIST pidl );

		static LPITEMIDLIST Pidl_Copy( LPCITEMIDLIST pidl );
		static LPITEMIDLIST Pidl_CopyFirstItem( LPCITEMIDLIST pidl );

		static LPITEMIDLIST Pidl_GetChildItem( IShellFolder* pFolder, const TCHAR itemFilename[] );		// itemFilename is normally a fname.ext; also works with a sub-path

		static bool Pidl_IsEqual( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl ) { return ::ILIsEqual( leftPidl, rightPidl ) != FALSE; }
		static pred::CompareResult Pidl_Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder = GetDesktopFolder() );

		static CComPtr<IShellFolder> GetDesktopFolder( void );
	private:
		const BYTE* GetBuffer( void ) const { return Pidl_GetBuffer( m_pidl ); }
		BYTE* GetBuffer( void ) { return Pidl_GetBuffer( m_pidl ); }

		static BYTE* Pidl_GetBuffer( LPITEMIDLIST pidl ) { ASSERT( !Pidl_IsNull( pidl ) ); return reinterpret_cast<BYTE*>( pidl ); }
		static const BYTE* Pidl_GetBuffer( LPCITEMIDLIST pidl ) { ASSERT( !Pidl_IsNull( pidl ) ); return reinterpret_cast<const BYTE*>( pidl ); }

		static WORD* Pidl_GetTerminator( LPITEMIDLIST pidl, size_t size ) { ASSERT( !Pidl_IsNull( pidl ) ); return reinterpret_cast<WORD*>( Pidl_GetBuffer( pidl ) + size ); }
		static void Pidl_SetTerminator( LPITEMIDLIST pidl, size_t size ) { *Pidl_GetTerminator( pidl, size ) = 0; }
	private:
		LPITEMIDLIST m_pidl;
	};
}


#endif // ShellTypes_h
