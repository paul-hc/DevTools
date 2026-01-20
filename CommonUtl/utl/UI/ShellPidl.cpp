
#include "pch.h"
#include "ShellPidl.h"

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

	bool CPidlAbsolute::CreateFrom( IUnknown* pUnknown )
	{	// most general, works for any compatible interface passed (IShellItem, IShellFolder, IPersistFolder2, etc)
		ASSERT_PTR( pUnknown );
		return HR_OK( ::SHGetIDListFromObject( pUnknown, &Ref() ) );
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


	// CPidlChild implementation

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
}
