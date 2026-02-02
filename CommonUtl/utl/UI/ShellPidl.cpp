
#include "pch.h"
#include "ShellPidl.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// PIDL serialization:

CArchive& operator<<( CArchive& archive, LPCITEMIDLIST pidl )
{
	UINT size = static_cast<UINT>( shell::pidl::GetByteSize( pidl ) );
	ASSERT( ( nullptr == pidl ) == ( 0 == size ) );

	archive.WriteCount( size );
	archive.Write( pidl, size );		// works well for null case (size=0)
	return archive;
}

CArchive& operator>>( CArchive& archive, OUT LPITEMIDLIST& rPidl ) throws_( CArchiveException )
{
	UINT size = static_cast<UINT>( archive.ReadCount() );

	rPidl = nullptr;
	if ( size != 0 )
	{
		CComHeapPtr<ITEMIDLIST> newPidl;
		newPidl.AllocateBytes( size );

		if ( size == archive.Read( newPidl, size ) )
			rPidl = newPidl.Detach();
		else
			throw new CArchiveException( CArchiveException::endOfFile );	// reached end of file while reading an object.
	}

	return archive;
}


namespace shell
{
	// CPidlAbsolute implementation

	CPidlAbsolute::CPidlAbsolute( PIDLIST_ABSOLUTE rootPidl, PCUIDLIST_RELATIVE dirPathPidl, PCUITEMID_CHILD childPidl /*= nullptr*/ )
		: CBasePidl()
	{	// creates a copy with concatenation
		Reset( ::ILCombine( rootPidl, dirPathPidl ) );

		if ( childPidl != nullptr )
			Combine( childPidl );
	}

	CPidlAbsolute& CPidlAbsolute::operator=( const CPidlAbsolute& right )
	{
		if ( right.Get() != Get() )
			Reset( ::ILCloneFull( right.Get() ) );

		return *this;
	}

	bool CPidlAbsolute::CreateFromShellPath( const TCHAR* pShellPath, DWORD fileAttribute /*= 0*/ )
	{	// if fileAttribute is FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY, it parses inexistent file/directory paths
		Reset( pidl::ParseToPidlFileSys( pShellPath, fileAttribute ) );
		return !IsNull();
	}

	bool CPidlAbsolute::CreateFrom( IUnknown* pShellItf )
	{	// most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		ASSERT_PTR( pShellItf );
		return HR_OK( ::SHGetIDListFromObject( pShellItf, &Ref() ) );
	}

	SFGAOF CPidlAbsolute::GetAttributes( void ) const
	{
		ASSERT( !IsNull() );
		return pidl::GetPidlAttributes( Get() );
	}

	std::pair<CImageList*, int> CPidlAbsolute::GetSysImageIndex( UINT iconFlag /*= SHGFI_SMALLICON*/ ) const
	{
		ASSERT( !IsNull() );
		return pidl::GetPidlImageIndex( Get(), iconFlag );
	}

	bool CPidlAbsolute::GetParentPidl( OUT CPidlAbsolute& rParentPidl ) const
	{
		REQUIRE( !IsNull() );

		rParentPidl.AssignCopy( Get() );
		return rParentPidl.RemoveLast();
	}

	PIDLIST_ABSOLUTE CPidlAbsolute::GetParentPidl( void ) const
	{
		CPidlAbsolute parentPidl;

		if ( !GetParentPidl( parentPidl ) )
			return nullptr;

		return parentPidl.Release();		// caller must free the parent PIDL
	}

	bool CPidlAbsolute::GetChildPidl( OUT CPidlChild& rChildPidl ) const
	{
		ASSERT( !ToShellPath().IsGuidPath() );		// child PIDL makes sense only for file-system PIDLs; it doesn't work for special GUID PIDLs

		REQUIRE( !IsNull() );

		rChildPidl.AssignCopy( GetLastItem() );
		return !rChildPidl.IsNull();
	}

	PUITEMID_CHILD CPidlAbsolute::GetChildPidl( void ) const
	{
		CPidlChild childPidl;

		if ( !GetChildPidl( childPidl ) )
			return nullptr;

		return childPidl.Release();			// caller must free the child PIDL
	}

	bool CPidlAbsolute::CreateFromFolder( IShellFolder* pShellFolder )
	{
		CComQIPtr<IPersistFolder2> pFolder( pShellFolder );

		if ( pFolder != nullptr )
			if ( HR_OK( pFolder->GetCurFolder( &Ref() ) ) )
				return true;

		Reset();
		return false;
	}


	// CPidlRelative implementation

	CPidlRelative& CPidlRelative::operator=( const CPidlRelative& right )
	{
		if ( right.Get() != Get() )
			Reset( ::ILClone( right.Get() ) );

		return *this;
	}


	// CPidlChild implementation

	CPidlChild& CPidlChild::operator=( const CPidlChild& right )
	{
		if ( right.Get() != Get() )
			Reset( ::ILCloneChild( right.Get() ) );

		return *this;
	}

	bool CPidlChild::CreateChild( const TCHAR* pFullPath )
	{
		CPidlAbsolute pidlFullPath;

		if ( pidlFullPath.CreateFromPath( pFullPath ) )
		{
			AssignCopy( pidlFullPath.GetLastItem() );
			return true;
		}
		else
			Reset();

		return !IsNull();
	}


	// CFolderRelativePidls implementation

	void CFolderRelativePidls::Clear( void )
	{
		m_ancestorPath.Clear();
		m_relativePaths.clear();

		m_ancestorPath.Clear();
		m_pAncestorFolder = nullptr;
		ClearOwningPidls( m_absolutePidls );
		ClearOwningPidls( m_relativePidls );
	}

	bool CFolderRelativePidls::MakeRelativePidls( void )
	{
		const std::vector<shell::TPath>& shellPaths = m_relativePaths;		// initially the absolute shell paths are stored

		if ( shellPaths.empty() )
			return false;		// no input

		shell::CreateAbsolutePidls( m_absolutePidls, shellPaths );

		m_ancestorPath = path::ExtractCommonParentPath( shellPaths );

		CPidlAbsolute ancestorPidl( m_ancestorPath.GetPtr() );

		if ( ancestorPidl.IsNull() )
			return false;		// couldn't find a valid coomon ancestor for input shell paths

		if ( !HR_OK( ::SHBindToObject( nullptr, ancestorPidl, nullptr, IID_PPV_ARGS( &m_pAncestorFolder ) ) ) )		// bind to the ancestor IShellFolder
			return false;		// couldn't bind to the ancestor IShellFolder

		CComPtr<IBindCtx> pBindCtx;

		if ( shell::IsGuidPath( m_ancestorPath ) || utl::Any( shellPaths, pred::IsGuidPath() ) )
			pBindCtx = shell::CreateFileSysBindContext( FILE_ATTRIBUTE_NORMAL );		// required for parsing GUID paths

		// strip ancestor dir and extract relative PIDLs
		for ( std::vector<shell::TPath>::iterator itRelPath = m_relativePaths.begin(); itRelPath != m_relativePaths.end(); ++itRelPath )
		{
			path::StripPrefix( itRelPath->Ref(), m_ancestorPath.GetPtr() );

			if ( PIDLIST_RELATIVE relPidl = shell::pidl::ParseToRelativePidl( m_pAncestorFolder, itRelPath->GetPtr(), pBindCtx ) )
				m_relativePidls.push_back( relPidl );
		}

		// caller can use m_pAncestorFolder and m_relativePidls, e.g. for querying IContextMenu, ::SHCreateShellItemArrayFromIDLists, etc.
		ENSURE( !IsEmpty() );
		return true;
	}

	shell::TPath CFolderRelativePidls::GetRelativePathAt( size_t posRelPidl ) const
	{
		ASSERT( posRelPidl < m_relativePidls.size() );
		ASSERT_PTR( m_pAncestorFolder );

		PCUITEMID_CHILD childPidl = reinterpret_cast<PCUITEMID_CHILD>( m_relativePidls[ posRelPidl ] );
		shell::TPath leafRelativePath = shell::GetFolderChildDisplayName( m_pAncestorFolder, childPidl, SHGDN_FORPARSING );		// returns full path if SHGDN_INFOLDER is not specified

		leafRelativePath.StripPrefix( m_ancestorPath );
		return leafRelativePath;
	}

	CComPtr<IContextMenu> CFolderRelativePidls::MakeContextMenu( HWND hWndOwner ) const
	{
		CComPtr<IContextMenu> pCtxMenu;

		if ( m_pAncestorFolder != nullptr )
		{
			ASSERT( !IsEmpty() );
			if ( !HR_OK( m_pAncestorFolder->GetUIObjectOf( hWndOwner, static_cast<unsigned int>( m_relativePidls.size() ), GetChildPidlArray(),
														   __uuidof( IContextMenu ), nullptr, (void**)&pCtxMenu ) ) )
				return nullptr;
		}

		return pCtxMenu;
	}

	CComPtr<IShellItemArray> CFolderRelativePidls::CreateShellItemArray( void ) const
	{
		CComPtr<IShellItemArray> pShellItemArray;

		if ( !HR_OK( ::SHCreateShellItemArrayFromIDLists( static_cast<unsigned int>( m_relativePidls.size() ), GetAbsolutePidlArray(), &pShellItemArray ) ) )
			return nullptr;

		return pShellItemArray;
	}
}


// CPathPidlItem implementation

CPathPidlItem::CPathPidlItem( bool useDirPath /*= false*/ )
	: CPathItemBase( shell::TPath() )
	, m_fileAttribute( 0 )
{
	SetUseDirPath( useDirPath );
}

void CPathPidlItem::SetShellPath( const shell::TPath& shellPath ) override
{
	SetFilePath( shellPath );

	if ( shellPath.IsGuidPath() )
		m_specialPidl.Reset( shell::MakePidl( shellPath, m_fileAttribute ) );
	else
		m_specialPidl.Reset();
}

void CPathPidlItem::SetPidl( const shell::CPidlAbsolute& pidl )
{
	if ( pidl.IsNull() )
	{
		m_specialPidl.Reset();
		SetFilePath( shell::TPath() );
		return;
	}
	else if ( pidl.IsSpecialPidl() )
		m_specialPidl = pidl;

	SetFilePath( pidl.ToShellPath() );		// file path could be eithera file-system path or a GUID path
}

bool CPathPidlItem::ObjectExist( void ) const
{
	if ( IsFilePath() )
		return GetFilePath().GetExpanded().FileExist();
	else if ( IsSpecialPidl() )
		return true;

	return false;
}

std::tstring CPathPidlItem::FormatPhysical( void ) const
{
	if ( IsFilePath() )
		return GetFilePath().GetExpanded().Get();
	else if ( IsSpecialPidl() )
		return GetFilePath().Get();		// i.e. m_specialPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING )

	return str::GetEmpty();
}

bool CPathPidlItem::ParsePhysical( const std::tstring& shellPath )
{
	if ( !path::IsValidPath( shellPath ) && !path::IsValidGuidPath( shellPath ) )
	{
		TRACE( _T("! CPathPidlItem::ParsePhysical(): ignoring input PIDL string '%s' since it's not parsable\n"), shellPath.c_str() );
		return false;
	}

	SetShellPath( shellPath );
	return true;
}
