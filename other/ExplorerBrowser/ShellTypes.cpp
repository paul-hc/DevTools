
#include "pch.h"
#include "ShellTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CPidl::CPidl( LPCITEMIDLIST rootPidl, LPCITEMIDLIST dirPathPidl, LPCITEMIDLIST childPidl /*= NULL*/ )
	{
		AssignCopy( rootPidl );
		Concatenate( dirPathPidl );

		if ( childPidl != NULL )
			ConcatenateChild( childPidl );
	}

	size_t CPidl::GetHash( void ) const
	{
		if ( IsEmpty() )
			return 0;

		const BYTE* pBytes = GetBuffer();

		return utl::HashValue( pBytes, GetByteSize() );
	}

	void CPidl::Concatenate( LPCITEMIDLIST rightPidl )
	{
		if ( IsNull() )
			AssignCopy( rightPidl );
		else if ( !Pidl_IsNull( rightPidl ) )
		{
			size_t oldSize = GetByteSize() - sizeof( WORD );
			size_t rightSize = Pidl_GetByteSize( rightPidl );

			LPITEMIDLIST newPidl = Pidl_Allocate( oldSize + rightSize );
			if ( newPidl != NULL )
			{
				::CopyMemory( newPidl, m_pidl, oldSize );
				::CopyMemory( ( (LPBYTE)newPidl ) + oldSize, rightPidl, rightSize );
			}
			Reset( newPidl );
		}
	}

	void CPidl::ConcatenateChild( LPCITEMIDLIST childPidl )
	{
		if ( IsNull() )
			AssignCopy( childPidl );
		else if ( !Pidl_IsNull( childPidl ) )
		{
			size_t oldSize = GetByteSize() - sizeof( WORD );
			size_t childSize = childPidl->mkid.cb;

			LPITEMIDLIST pidlNew = Pidl_Allocate( oldSize + childSize + sizeof( WORD ) );
			if ( pidlNew != NULL )
			{
				::CopyMemory( pidlNew, m_pidl, oldSize );
				::CopyMemory( ((LPBYTE)pidlNew) + oldSize, childPidl, childSize );
				Pidl_SetTerminator( pidlNew, oldSize + childSize );
			}
			Reset( pidlNew );
		}
	}

	void CPidl::RemoveLast( void )
	{
		if ( LPITEMIDLIST lastPidl = const_cast<LPITEMIDLIST>( Pidl_GetLastItem( m_pidl ) ) )
			lastPidl->mkid.cb = 0;
	}


	// CPidl advanced

	bool CPidl::CreateFromPath( const TCHAR fullPath[] )
	{
		ASSERT( !str::IsEmpty( fullPath ) );
		Reset( ::ILCreateFromPath( fullPath ) );
		return !IsNull();
	}

	bool CPidl::CreateAbsolute( const TCHAR fullPath[] )			// obsolete? same as CreateFromPath()?
	{
		CComPtr<IShellFolder> pDesktopFolder;
		if ( HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
			if ( HR_OK( pDesktopFolder->ParseDisplayName( NULL, NULL, const_cast<TCHAR*>( fullPath ), NULL, &m_pidl, NULL ) ) )
				return true;

		Delete();
		return false;
	}

	bool CPidl::CreateChild( IShellFolder* pFolder, const TCHAR itemFilename[] )
	{
		Reset( Pidl_GetChildItem( pFolder, itemFilename ) );
		return !IsNull();
	}

	bool CPidl::CreateFrom( IUnknown* pUnknown )
	{
		ASSERT_PTR( pUnknown );

		return HR_OK( ::SHGetIDListFromObject( pUnknown, &*this ) );
	}

	bool CPidl::CreateFromFolder( IShellFolder* pShellFolder )
	{
		CComQIPtr<IPersistFolder2> pFolder( pShellFolder );
		if ( pFolder != NULL )
			if ( HR_OK( pFolder->GetCurFolder( &*this ) ) )
				return true;

		Delete();
		return false;
	}

	CComPtr<IShellItem> CPidl::FindItem( IShellFolder* pParentFolder ) const
	{
		CComPtr<IShellItem> pShellItem;

		if ( pParentFolder != NULL )
		{
			if ( HR_OK( ::SHCreateItemWithParent( NULL, pParentFolder, m_pidl, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}
		else
		{
			if ( HR_OK( ::SHCreateItemFromParsingName( GetFullPath().c_str(), NULL, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}

		return NULL;
	}

	CComPtr<IShellFolder> CPidl::FindFolder( IShellFolder* pParentFolder /*= GetDesktopFolder()*/ ) const
	{
		ASSERT_PTR( pParentFolder );

		CComPtr<IShellFolder> pShellFolder;
		if ( HR_OK( pParentFolder->BindToObject( m_pidl, NULL, IID_PPV_ARGS( &pShellFolder ) ) ) )
			return pShellFolder;

		return NULL;
	}

	std::tstring CPidl::GetFullPath( GPFIDL_FLAGS optFlags /*= GPFIDL_DEFAULT*/ ) const
	{
		std::tstring fullPath;
		if ( !IsNull() )
		{
			TCHAR buffFullPath[ MAX_PATH * 2 ];
			if ( ::SHGetPathFromIDListEx( m_pidl, buffFullPath, _countof( buffFullPath ), optFlags ) )
				fullPath = buffFullPath;
		}
		else
			ASSERT( false );

		return fullPath;
	}

	std::tstring CPidl::GetName( SIGDN nameType /*= SIGDN_NORMALDISPLAY*/ ) const
	{
		std::tstring name;

		TCHAR* pName;
		if ( HR_OK( ::SHGetNameFromIDList( m_pidl, nameType, &pName ) ) )
		{
			name = pName;
			::CoTaskMemFree( pName );
		}
		return name;
	}

	bool CPidl::WriteToStream( IStream* pStream ) const
	{
		ULONG bytesWritten = 0;
		UINT size = static_cast<UINT>( GetByteSize() );

		if ( HR_OK( pStream->Write( &size, sizeof( size ), &bytesWritten ) ) )
			if ( HR_OK( pStream->Write( Get(), size, &bytesWritten ) ) )
				return true;

		return false;
	}

	bool CPidl::ReadFromStream( IStream* pStream )
	{
		ULONG bytesRead = 0;
		UINT size = 0;

		if ( HR_OK( pStream->Read( &size, sizeof( size ), &bytesRead ) ) )
		{
			LPITEMIDLIST pidl = (LPITEMIDLIST)_alloca( size + 1 );   // +1 because of possible NULL pidl; alloc fails otherwise
			if ( !HR_OK( pStream->Read( pidl, size, &bytesRead ) ) )
				return false;

			AssignCopy( 0 == bytesRead ? NULL : pidl );
		}

		return size == bytesRead;
	}


	// CPidl static interface

	size_t CPidl::Pidl_GetCount( LPCITEMIDLIST pidl )
	{
		UINT count = 0;

		if ( pidl != NULL )
			for ( ; pidl->mkid.cb > 0; pidl = Pidl_GetNextItem( pidl ) )
				++count;

		return count;
	}

	size_t CPidl::Pidl_GetByteSize( LPCITEMIDLIST pidl )
	{
		size_t size = 0;

		if ( pidl != NULL )
		{
			for ( ; pidl->mkid.cb > 0; pidl = Pidl_GetNextItem( pidl ) )
				size += pidl->mkid.cb;

			size += sizeof( WORD );		// add the size of the NULL terminating ITEMIDLIST
		}
		return size;
	}

	LPCITEMIDLIST CPidl::Pidl_GetNextItem( LPCITEMIDLIST pidl )
	{
		LPITEMIDLIST nextPidl = NULL;
		if ( pidl != NULL )
			nextPidl = (LPITEMIDLIST)( Pidl_GetBuffer( pidl ) + pidl->mkid.cb );

		return nextPidl;
	}

	LPCITEMIDLIST CPidl::Pidl_GetLastItem( LPCITEMIDLIST pidl )
	{
		// Get the PIDL of the last item in the list
		LPCITEMIDLIST lastPidl = NULL;
		if ( pidl != NULL )
			for ( ; pidl->mkid.cb > 0; pidl = Pidl_GetNextItem( pidl ) )
				lastPidl = pidl;

		return lastPidl;
	}

	LPITEMIDLIST CPidl::Pidl_Copy( LPCITEMIDLIST pidl )
	{
		LPITEMIDLIST targetPidl = NULL;

		if ( pidl != NULL )
		{
			size_t size = Pidl_GetByteSize( pidl );

			targetPidl = Pidl_Allocate( size );				// allocate the new pidl
			if ( targetPidl != NULL )
				::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
		}

		return targetPidl;
	}

	LPITEMIDLIST CPidl::Pidl_CopyFirstItem( LPCITEMIDLIST pidl )
	{
		LPITEMIDLIST targetPidl = NULL;

		if ( pidl != NULL )
		{
			size_t size = pidl->mkid.cb + sizeof( WORD );

			targetPidl = Pidl_Allocate( size );				// allocate the new pidl
			if ( targetPidl != NULL )
			{
				::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
				*(WORD*) ( ( (BYTE*)targetPidl ) + targetPidl->mkid.cb ) = 0;		// add terminator
			}
		}
		return targetPidl;
	}

	LPITEMIDLIST CPidl::Pidl_GetChildItem( IShellFolder* pFolder, const TCHAR itemFilename[] )
	{
		ASSERT_PTR( pFolder );
		ASSERT_PTR( !str::IsEmpty( itemFilename ) );

		TCHAR displayName[ MAX_PATH * 2 ];
		_tcscpy( displayName, itemFilename );

		LPITEMIDLIST childItemPidl = NULL;
		if ( HR_OK( pFolder->ParseDisplayName( NULL, NULL, displayName, NULL, &childItemPidl, NULL ) ) )
			return childItemPidl;

		return NULL;
	}

	pred::CompareResult CPidl::Pidl_Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder /*= GetDesktopFolder()*/ )
	{
		ASSERT_PTR( pParentFolder );

		HRESULT hResult = pParentFolder->CompareIDs( SHCIDS_CANONICALONLY, leftPidl, rightPidl );
		return pred::ToCompareResult( (short)HRESULT_CODE( hResult ) );
	}

	CComPtr<IShellFolder> CPidl::GetDesktopFolder( void )
	{
		CComPtr<IShellFolder> pDesktopFolder;
		if ( HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
			return pDesktopFolder;

		return NULL;
	}
}
