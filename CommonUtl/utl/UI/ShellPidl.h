#ifndef ShellPidl_h
#define ShellPidl_h
#pragma once

#define STRICT_TYPED_ITEMIDS
#include <shobjidl_core.h>		// for SHGetIDListFromObject()
#include <shlobj_core.h>		// for ILClone(), etc
#include "ShellTypes.h"


namespace shell
{
	// Type-safe PIDL wrapper that manages memory in shell API; similar to CComHeapPtr<ITEMIDLIST> with PIDL specific functionality.
	//
	template< typename ItemIdListT, typename PtrItemIdListT = ItemIdListT __unaligned*, typename PtrConstItemIdListT = const ItemIdListT __unaligned* >
	class CBasePidl : private utl::noncopyable
	{
	protected:
		CBasePidl( void ) : m_pidl( nullptr ) {}
		CBasePidl( CBasePidl& rRight ) : m_pidl( rRight.Release() ) {}		// copy-move constructor
		explicit CBasePidl( PtrItemIdListT pidl ) : m_pidl( pidl ) {}		// take ownership of pidl
	public:
		~CBasePidl() { Reset(); }

		CBasePidl& operator=( CBasePidl& rRight ) { if ( m_pidl != rRight.Get() ) Reset( rRight.Release() ); return *this; }		// move assignment

		void AssignCopy( PtrConstItemIdListT srcPidl ) { Reset( reinterpret_cast<PtrItemIdListT>( ::ILClone( srcPidl ) ) ); }

		bool IsNull( void ) const { return nullptr == m_pidl; }
		bool IsAligned( void ) const { return ::ILIsAligned( m_pidl ); }		// ITEMIDLIST pointer could be aligned/unaligned
		bool HasValue( void ) const { return !pidl::IsEmpty( m_pidl ); }		// neither Null nor Empty

		// Empty referrs to Desktop folder, e.g. for "C:\Users\<UserName>\Desktop"
		bool IsEmpty( void ) const { return pidl::IsEmptyStrict( m_pidl ); }	// Note: desktop PIDL is empty (but not null)
		void SetEmpty( void ) { Reset( pidl::CreateEmptyPidl<PtrItemIdListT>() ); }

		size_t GetItemCount( void ) const { return pidl::GetItemCount( m_pidl ); }
		size_t GetByteSize( void ) const { return pidl::GetByteSize( m_pidl ); }
		size_t GetHash( void ) const { return IsEmpty() ? 0 : utl::HashBytes( GetBuffer(), GetByteSize() ); }

		PtrItemIdListT& Ref( void ) { Reset(); return m_pidl; }
		PtrConstItemIdListT Get( void ) const { return m_pidl; }
		void Set( PtrItemIdListT pidl, utl::Ownership ownership ) { utl::COPY == ownership ? AssignCopy( pidl ) : Reset( pidl ); }

		PtrItemIdListT* operator&() { Reset(); return &m_pidl; }

		void Reset( PtrItemIdListT pidl = nullptr )
		{
			if ( m_pidl != nullptr )
			{
				ASSERT( pidl != m_pidl );		// 2 owners for the same object?
				pidl::Delete( Release() );
			}
			m_pidl = pidl;
		}

		PtrItemIdListT Release( void )
		{
			PtrItemIdListT pidl = m_pidl;
			m_pidl = nullptr;
			return pidl;
		}

		// PIDL compare (native for absolute)
		bool operator==( const CBasePidl& rightPidl ) const { return ::ILIsEqual( AsAbsolute(), rightPidl.AsAbsolute() ); }
		bool operator!=( const CBasePidl& rightPidl ) const { return !operator==( rightPidl ); }

		std::tstring GetName( SIGDN nameType ) const { return pidl::GetName( AsAbsolute(), nameType ); }
		std::tstring GetFnameExt( void ) const { return pidl::GetName( AsAbsolute(), SIGDN_NORMALDISPLAY ); }
		fs::CPath ToPath( GPFIDL_FLAGS optFlags = GPFIDL_DEFAULT ) const { return pidl::GetAbsolutePath( AsAbsolute(), optFlags ); }

		PUIDLIST_RELATIVE GetNextItem( void ) const { return ::ILNext( m_pidl ); }
		PUITEMID_CHILD GetLastItem( void ) const { return ::ILFindLastID( AsRelative() ); }

		void Combine( PCUIDLIST_RELATIVE rightPidlRel ) { Reset( ::ILCombine( Get(), rightPidlRel ) ); }	// works for PCUIDLIST_RELATIVE/PCUITEMID_CHILD
		void RemoveLast( void ) { ::ILRemoveLastID( m_pidl ); }

		bool WriteToStream( OUT IStream* pStream ) const { return HR_OK( ::ILSaveToStream( pStream, m_pidl ) ); }
		bool ReadFromStream( IN IStream* pStream ) { return HR_OK( ::ILLoadFromStreamEx( pStream, &Ref() ) ); }

		CComPtr<IShellFolder> OpenFolder( IShellFolder* pParentFolder = GetDesktopFolder() ) const
		{
			ASSERT_PTR( pParentFolder );
			CComPtr<IShellFolder> pShellFolder;
			return HR_OK( pParentFolder->BindToObject( Get(), nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) ? pShellFolder : nullptr;
		}

		CComPtr<IShellItem> OpenShellItem( IShellFolder* pParentFolder = nullptr ) const
		{
			CComPtr<IShellItem> pShellItem;
			HRESULT hResult = pParentFolder != nullptr
				? ::SHCreateItemWithParent( nullptr, pParentFolder, AsChild(), IID_PPV_ARGS( &pShellItem ) )
				: ::SHCreateItemFromParsingName( ToPath().GetPtr(), nullptr, IID_PPV_ARGS( &pShellItem ) );

			return HR_OK( hResult ) ? pShellItem : nullptr;
		}
	protected:
		PCIDLIST_ABSOLUTE AsAbsolute( void ) const { return reinterpret_cast<PCIDLIST_ABSOLUTE>( m_pidl ); }
		PCUIDLIST_RELATIVE AsRelative( void ) const { return reinterpret_cast<PCUIDLIST_RELATIVE>( m_pidl ); }
		PCUITEMID_CHILD AsChild( void ) const { return reinterpret_cast<PCUITEMID_CHILD>( m_pidl ); }
	private:
		const BYTE* GetBuffer( void ) const { return pidl::impl::GetBuffer( m_pidl ); }
	private:
		PtrItemIdListT m_pidl;
	};


	class CPidlAbsolute : public CBasePidl<ITEMIDLIST_ABSOLUTE, PIDLIST_ABSOLUTE, PCIDLIST_ABSOLUTE>
	{
		typedef CBasePidl<ITEMIDLIST_ABSOLUTE, PIDLIST_ABSOLUTE, PCIDLIST_ABSOLUTE> TBasePidl;
	public:
		CPidlAbsolute( void ) {}
		CPidlAbsolute( CPidlAbsolute& rRight ) : TBasePidl( rRight ) {}				// MOVE constructor
		explicit CPidlAbsolute( PIDLIST_ABSOLUTE pidl, utl::Ownership ownership = utl::MOVE ) { Set( pidl, ownership ); }			// take ownership of pidl by default
		explicit CPidlAbsolute( PIDLIST_ABSOLUTE rootPidl, PCUIDLIST_RELATIVE dirPathPidl, PCUITEMID_CHILD childPidl = nullptr );	// creates a copy with concatenation

		bool CreateFromPath( const TCHAR* pFullPath ) { Reset( ::ILCreateFromPath( pFullPath ) ); return !IsNull(); }

		// advanced
		bool CreateFrom( IUnknown* pUnknown );					// most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		bool CreateFromFolder( IShellFolder* pShellFolder );	// ABSOLUTE pidl - superseeded by CreateFrom()
	};


	class CPidlAbsCp : public CPidlAbsolute		// copyable PIDL
	{
	public:
		CPidlAbsCp( void ) {}
		CPidlAbsCp( const CPidlAbsCp& right ) : CPidlAbsolute( ::ILCloneFull( right.Get() ) ) {}	// COPY constructor
		explicit CPidlAbsCp( PIDLIST_ABSOLUTE pidl, utl::Ownership ownership = utl::MOVE ) : CPidlAbsolute( pidl, ownership ) {}	// optionally MOVE/COPY the pidl

		CPidlAbsCp& operator=( const CPidlAbsCp& right )	// COPY assignment
		{
			Reset( ::ILCloneFull( right.Get() ) );
			return *this;
		}
	};


	class CPidlRelative : public CBasePidl<ITEMIDLIST_RELATIVE, PUIDLIST_RELATIVE, PCUIDLIST_RELATIVE>
	{
		typedef CBasePidl<ITEMIDLIST_RELATIVE, PUIDLIST_RELATIVE, PCUIDLIST_RELATIVE> TBasePidl;
	public:
		CPidlRelative( void ) {}
		CPidlRelative( CPidlRelative& rRight ) : TBasePidl( rRight ) {}				// copy-move constructor
		explicit CPidlRelative( PIDLIST_RELATIVE pidl, utl::Ownership ownership = utl::MOVE ) { Set( pidl, ownership ); }		// take ownership of pidl by default

		fs::TRelativePath ToRelativePath( IShellFolder* pFolder ) const { return shell::GetDisplayName( pFolder, AsChild() ); }

		bool CreateRelative( IShellFolder* pFolder, const TCHAR* pRelPath ) { Reset( pidl::CreateRelativePidl( pFolder, pRelPath ) ); return !IsNull(); }

		// relative PIDL compare
		pred::CompareResult Compare( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder = GetDesktopFolder() ) const
		{
			return pidl::Compare( Get(), rightPidlRel.Get(), pParentFolder );
		}

		bool Equals( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder = GetDesktopFolder() ) const
		{
			return pred::Equal == Compare( rightPidlRel, pParentFolder );
		}
	};


	class CPidlChild : public CBasePidl<ITEMID_CHILD, PUITEMID_CHILD, PCUITEMID_CHILD>
	{
		typedef CBasePidl<ITEMID_CHILD, PUITEMID_CHILD, PCUITEMID_CHILD> TBasePidl;

		// hidden base methods
		using TBasePidl::Combine;
		using TBasePidl::RemoveLast;
	public:
		CPidlChild( void ) {}
		CPidlChild( const CPidlChild& right ) : TBasePidl( ::ILCloneChild( right.Get() ) ) {}	// COPY constructor
		explicit CPidlChild( PUITEMID_CHILD pidl, utl::Ownership ownership = utl::MOVE ) { Set( pidl, ownership ); }	// take ownership of pidl by default

		bool CreateChild( IShellFolder* pFolder, const TCHAR* pFnameExt ) { Reset( pidl::CreateChilePidl( pFolder, pFnameExt ) ); return !IsNull(); }
		bool CreateChild( const TCHAR* pFullPath );

		// COPY assignment
		CPidlChild& operator=( const CPidlChild& right ) { Reset( ::ILCloneChild( right.Get() ) ); return *this; }

		// child PIDL compare
		pred::CompareResult Compare( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder ) const { return pidl::Compare( Get(), rightPidlRel.Get(), pParentFolder ); }
		bool Equals( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder ) const { return pred::Equal == Compare( rightPidlRel, pParentFolder ); }
	};
}


// PIDL serialization:


class CArchive;


CArchive& operator<<( CArchive& archive, LPCITEMIDLIST pidl );
CArchive& operator>>( CArchive& archive, OUT LPITEMIDLIST& rPidl ) throws_( CArchiveException );


template< typename ItemIdListT, typename PtrItemIdListT, typename PtrConstItemIdListT >
inline CArchive& operator<<( CArchive& archive, const shell::CBasePidl<ItemIdListT, PtrItemIdListT, PtrConstItemIdListT>& pidl )
{
	return archive << pidl.Get();
}

template< typename ItemIdListT, typename PtrItemIdListT, typename PtrConstItemIdListT >
inline CArchive& operator>>( CArchive& archive, OUT shell::CBasePidl<ItemIdListT, PtrItemIdListT, PtrConstItemIdListT>& rPidl )
{
	return archive >> reinterpret_cast<LPITEMIDLIST&>( rPidl.Ref() );
}


#endif // ShellPidl_h
