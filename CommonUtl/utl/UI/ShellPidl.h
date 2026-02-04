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
		explicit CBasePidl( PtrItemIdListT pidl ) : m_pidl( pidl ) {}			// take ownership of pidl (MOVE)
	public:
		~CBasePidl() { Reset(); }

		CBasePidl& operator=( CBasePidl& rRight ) { if ( m_pidl != rRight.Get() ) Reset( rRight.Release() ); return *this; }		// move assignment

		void AssignCopy( PtrConstItemIdListT srcPidl ) { Reset( reinterpret_cast<PtrItemIdListT>( ::ILClone( srcPidl ) ) ); }

		bool IsNull( void ) const { return nullptr == m_pidl; }
		bool IsAligned( void ) const { return ::ILIsAligned( m_pidl ); }		// ITEMIDLIST pointer could be aligned/unaligned
		bool HasValue( void ) const { return !pidl::IsEmpty( m_pidl ); }		// neither Null nor Empty
		bool IsSpecialPidl( void ) const { return pidl::IsSpecialPidl( m_pidl ); }

		// Empty referrs to Desktop folder, e.g. for "C:\Users\<UserName>\Desktop"
		bool IsEmpty( void ) const { return pidl::IsEmptyStrict( m_pidl ); }	// Note: desktop PIDL is empty (but not null)
		void SetEmpty( void ) { Reset( pidl::CreateEmptyPidl<PtrItemIdListT>() ); }		// i.e. desktop PIDL

		size_t GetItemCount( void ) const { return pidl::GetItemCount( m_pidl ); }
		size_t GetByteSize( void ) const { return pidl::GetByteSize( m_pidl ); }
		size_t GetHash( void ) const { return IsEmpty() ? 0 : utl::HashBytes( GetBuffer(), GetByteSize() ); }

		PtrItemIdListT& Ref( void ) { Reset(); return m_pidl; }
		PtrConstItemIdListT Get( void ) const { return m_pidl; }

		operator PtrConstItemIdListT( void ) const { return m_pidl; }
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

		shell::TPath ToShellPath( void ) const { return pidl::GetShellPath( AsAbsolute() ); }
		std::tstring GetName( SIGDN nameType ) const { return pidl::GetName( AsAbsolute(), nameType ); }
		std::tstring GetFilename( void ) const { return pidl::GetName( AsAbsolute(), SIGDN_NORMALDISPLAY ); }
		std::tstring GetEditingName( void ) const { return GetName( SIGDN_DESKTOPABSOLUTEEDITING ); }
		std::tstring GetParsingName( void ) const { return GetName( SIGDN_DESKTOPABSOLUTEPARSING ); }	// equivalent to ToShellPath()

		PUIDLIST_RELATIVE GetNextItem( void ) const { return ::ILNext( m_pidl ); }
		PUITEMID_CHILD GetLastItem( void ) const { return ::ILFindLastID( AsRelative() ); }

		void Combine( PCUIDLIST_RELATIVE rightPidlRel ) { Reset( ::ILCombine( Get(), rightPidlRel ) ); }	// works for PCUIDLIST_RELATIVE/PCUITEMID_CHILD
		bool RemoveLast( void ) { return ::ILRemoveLastID( m_pidl ) != FALSE; }		// transforms pidl to parent pidl

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
				: ::SHCreateItemFromParsingName( ToShellPath().GetPtr(), nullptr, IID_PPV_ARGS( &pShellItem ) );

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


	class CPidlChild;


	class CPidlAbsolute : public CBasePidl<ITEMIDLIST_ABSOLUTE, PIDLIST_ABSOLUTE, PCIDLIST_ABSOLUTE>
	{
		typedef CBasePidl<ITEMIDLIST_ABSOLUTE, PIDLIST_ABSOLUTE, PCIDLIST_ABSOLUTE> TBasePidl;

		// hidden base methods
		using TBasePidl::IsEmpty;
		using TBasePidl::SetEmpty;
	public:
		CPidlAbsolute( void ) {}
		CPidlAbsolute( const CPidlAbsolute& right ) : TBasePidl( ::ILCloneFull( right.Get() ) ) {}			// copy constructor
		CPidlAbsolute( const TCHAR* pShellPath, DWORD fileAttribute = 0 ) { CreateFromShellPath( pShellPath, fileAttribute ); }
		CPidlAbsolute( IUnknown* pShellItf ) { CreateFrom( pShellItf ); }
		explicit CPidlAbsolute( PIDLIST_ABSOLUTE pidl ) : TBasePidl( pidl ) {}			// take ownership of pidl (MOVE)

		explicit CPidlAbsolute( PIDLIST_ABSOLUTE rootPidl, PCUIDLIST_RELATIVE dirPathPidl, PCUITEMID_CHILD childPidl = nullptr );	// creates a copy with concatenation

		CPidlAbsolute& operator=( const CPidlAbsolute& right );		// copy assignment

		bool CreateFromShellPath( const TCHAR* pShellPath, DWORD fileAttribute = 0 );	// preferred, more general method; pass FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY for inexistent paths
		bool CreateFromPath( const TCHAR* pExistingShellPath ) { Reset( pidl::CreateFromPath( pExistingShellPath ) ); return !IsNull(); }

		// desktop PIDL is empty (but not null)
		bool IsDesktop( void ) const { return IsEmpty(); }
		void CreateDesktop( void ) { SetEmpty(); }

		SFGAOF GetAttributes( void ) const;
		std::pair<CImageList*, int> GetSysImageIndex( UINT iconFlag = SHGFI_SMALLICON ) const;		// no ownership of the image list

		bool GetParentPidl( OUT CPidlAbsolute& rParentPidl ) const;
		PIDLIST_ABSOLUTE GetParentPidl( void ) const;				// caller must free the parent PIDL

		bool GetChildPidl( OUT CPidlChild& rChildPidl ) const;
		PUITEMID_CHILD GetChildPidl( void ) const;					// caller must free the parent PIDL

		// advanced
		bool CreateFrom( IUnknown* pShellItf );						// most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		bool CreateFromFolder( IShellFolder* pShellFolder );		// ABSOLUTE pidl - superseeded by CreateFrom()
	};


	class CPidlRelative : public CBasePidl<ITEMIDLIST_RELATIVE, PUIDLIST_RELATIVE, PCUIDLIST_RELATIVE>
	{
		typedef CBasePidl<ITEMIDLIST_RELATIVE, PUIDLIST_RELATIVE, PCUIDLIST_RELATIVE> TBasePidl;
	public:
		CPidlRelative( void ) {}
		CPidlRelative( const CPidlRelative& right ) : TBasePidl( ::ILClone( right.Get() ) ) {}	// move constructor
		explicit CPidlRelative( PIDLIST_RELATIVE pidl ) : TBasePidl( pidl ) {}					// take ownership of pidl (MOVE)

		CPidlRelative& operator=( const CPidlRelative& right );		// copy assignment

		std::tstring ToFilename( IShellFolder* pFolder ) const { return shell::GetFolderChildDisplayName( pFolder, AsChild() ); }	// doesn't work for deep relative paths!

		bool CreateRelative( IShellFolder* pFolder, const TCHAR* pRelPath ) { Reset( pidl::ParseToRelativePidl( pFolder, pRelPath ) ); return !IsNull(); }

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
		explicit CPidlChild( PUITEMID_CHILD pidl ) : TBasePidl( pidl ) {}						// take ownership of pidl (MOVE)

		CPidlChild& operator=( const CPidlChild& right );		// copy assignment

		bool CreateChild( IShellFolder* pFolder, const TCHAR* pFilename ) { Reset( pidl::ParseToChildPidl( pFolder, pFilename ) ); return !IsNull(); }
		bool CreateChild( const TCHAR* pFullPath );

		// child PIDL compare
		pred::CompareResult Compare( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder ) const { return pidl::Compare( Get(), rightPidlRel.Get(), pParentFolder ); }
		bool Equals( const CPidlRelative& rightPidlRel, IShellFolder* pParentFolder ) const { return pred::Equal == Compare( rightPidlRel, pParentFolder ); }

		// Note: name conversion methods GetName(), GetFilename(), ToPath() work only for file-system child PIDLs,
		// but not for GUID based child PIDLs, such as a Control Panel applet.
		// Use IShellItem or IShellFolder interfaces for name conversions of the CP applets.
	};


	// binds an IShellFolder and relative PIDLs corresponding to a set of shell paths that share a common ancestor folder
	//
	class CFolderRelativePidls : private utl::noncopyable
	{
	public:
		CFolderRelativePidls( void ) {}
		~CFolderRelativePidls() { Clear(); }

		bool IsEmpty( void ) const { return m_relativePidls.empty(); }
		void Clear( void );

		template< typename PathContainerT >
		bool Build( const PathContainerT& shellPaths )		// e.g. vector<shell::TPath>, vector<std::tstring>, vector<const TCHAR*>
		{
			Clear();
			m_relativePaths.assign( shellPaths.begin(), shellPaths.end() );		// copy as absolute shell paths, later-on strip m_ancestorPath to convert to relative paths
			return MakeRelativePidls();
		}

		const shell::TPath& GetAncestorPath( void ) const { return m_ancestorPath; }
		const std::vector<shell::TRelativePath>& GetRelativePaths( void ) const { return m_relativePaths; }

		const std::vector<PIDLIST_ABSOLUTE>& GetAbsolutePidls( void ) const { return m_absolutePidls; }
		const std::vector<PIDLIST_RELATIVE>& GetRelativePidls( void ) const { return m_relativePidls; }

		IShellFolder* GetAncestorFolder( void ) const { return m_pAncestorFolder; }
		PCUITEMID_CHILD_ARRAY GetChildPidlArray( void ) const { return pidl::AsPidlArray<PCUITEMID_CHILD_ARRAY>( m_relativePidls ); }
		PCIDLIST_ABSOLUTE_ARRAY GetAbsolutePidlArray( void ) const { return pidl::AsPidlArray<PCIDLIST_ABSOLUTE_ARRAY>( m_absolutePidls ); }

		CComPtr<IContextMenu> MakeContextMenu( HWND hWndOwner ) const;
		CComPtr<IShellItemArray> CreateShellItemArray( void ) const;

		// unit testing
		shell::TPath GetRelativePathAt( size_t posRelPidl ) const;
	private:
		bool MakeRelativePidls( void );
	private:
		shell::TPath m_ancestorPath;
		std::vector<shell::TRelativePath> m_relativePaths;

		CComPtr<IShellFolder> m_pAncestorFolder;
		std::vector<PIDLIST_RELATIVE> m_relativePidls;		// with ownership
		std::vector<PIDLIST_ABSOLUTE> m_absolutePidls;		// with ownership
	};
}


namespace shell
{
	// IContextMenu helpers on multiple selected files/items

	template< typename PathContainerT >
	CComPtr<IContextMenu> MakeFilePathsContextMenu( const PathContainerT& shellPaths, HWND hWndOwner )
	{	// for mixed file system or virtual folder paths having a common ancestor folder (at least Desktop folder)
		shell::CFolderRelativePidls multiPidls;
		return multiPidls.Build( shellPaths ) ? multiPidls.MakeContextMenu( hWndOwner ) : nullptr;
	}

	template< typename ShellItemContainerT >
	CComPtr<IContextMenu> MakeItemsContextMenu( const ShellItemContainerT& shellItems, HWND hWndOwner )
	{
		std::vector<shell::TPath> shellPaths;
		shell::QueryShellPaths( shellPaths, shellItems );

		return shell::MakeFilePathsContextMenu( shellPaths, hWndOwner );
	}
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


#include "utl/PathItemBase.h"


class CPathPidlItem : public CPathItemBase
{
	// hidden base methods
	using CPathItemBase::SetFilePath;		// call SetShellPath() instead
public:
	CPathPidlItem( bool useDirPath = false );

	void SetUseDirPath( bool useDirPath ) { m_fileAttribute = useDirPath ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL; }

	bool IsEmpty( void ) const { return GetFilePath().IsEmpty() && m_specialPidl.IsNull(); }
	bool IsFilePath( void ) const { return !GetFilePath().IsEmpty() && m_specialPidl.IsNull(); }
	bool IsSpecialPidl( void ) const { return !m_specialPidl.IsNull(); }

	const shell::TPath& GetShellPath( void ) const { return GetFilePath(); }
	const shell::CPidlAbsolute& GetPidl( void ) const { return m_specialPidl; }

	void SetShellPath( const shell::TPath& shellPath ) override;
	void SetPidl( const shell::CPidlAbsolute& pidl );

	bool ObjectExist( void ) const;

	// I/O that uses SIGDN_DESKTOPABSOLUTEPARSING for pidl
	std::tstring FormatPhysical( void ) const;
	bool ParsePhysical( const std::tstring& shellPath );
private:
	DWORD m_fileAttribute;					// for non-existent fise-system paths
	shell::CPidlAbsolute m_specialPidl;		// mutually exclusive with CPathItemBase::m_filePath, only for known folder PIDLs
};


#endif // ShellPidl_h
