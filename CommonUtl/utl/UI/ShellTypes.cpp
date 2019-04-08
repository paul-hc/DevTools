
#include "stdafx.h"
#include "ShellTypes.h"
#include <xhash>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr< IShellFolder > GetDesktopFolder( void )
	{
		CComPtr< IShellFolder > pDesktopFolder;
		if ( HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
			return pDesktopFolder;

		return NULL;
	}

	CComPtr< IShellFolder > FindShellFolder( const TCHAR* pDirPath )
	{
		CComPtr< IShellFolder > pDirFolder;

		if ( CComPtr< IShellFolder > pDesktopFolder = GetDesktopFolder() )
		{
			CComHeapPtr< ITEMIDLIST > pidlFolder( static_cast< ITEMIDLIST_RELATIVE* >( pidl::GetRelativeItem( pDesktopFolder, pDirPath ) ) );
			if ( pidlFolder.m_pData != NULL )
				HR_AUDIT( pDesktopFolder->BindToObject( pidlFolder, NULL, IID_PPV_ARGS( &pDirFolder ) ) );
		}

		return pDirFolder;
	}


	CComPtr< IShellItem > FindShellItem( const fs::CPath& fullPath )
	{
		CComPtr< IShellItem > pShellItem;
		if ( HR_OK( ::SHCreateItemFromParsingName( fullPath.GetPtr(), NULL, IID_PPV_ARGS( &pShellItem ) ) ) )
			return pShellItem;
		return NULL;
	}

	CComPtr< IShellFolder2 > ToShellFolder( IShellItem* pFolderItem )
	{
		ASSERT_PTR( pFolderItem );

		CComPtr< IShellFolder2 > pShellFolder;

		CComHeapPtr< ITEMIDLIST_ABSOLUTE > pidlFolder;
		if ( HR_OK( ::SHGetIDListFromObject( pFolderItem, &pidlFolder ) ) )
			if ( CComPtr< IShellFolder > pDesktopFolder = GetDesktopFolder() )
				if ( HR_OK( pDesktopFolder->BindToObject( pidlFolder, NULL, IID_PPV_ARGS( &pShellFolder ) ) ) )
					return pShellFolder;

		return pShellFolder;
	}

	CComPtr< IShellFolder2 > GetParentFolderAndPidl( ITEMID_CHILD** pPidlItem, IShellItem* pShellItem )
	{
		if ( CComQIPtr< IParentAndItem > pParentAndItem = pShellItem )
		{
			CComPtr< IShellFolder > pParentFolder;
			if ( SUCCEEDED( pParentAndItem->GetParentAndItem( NULL, &pParentFolder, pPidlItem ) ) )
				return CComQIPtr< IShellFolder2 >( pParentFolder );
		}

		return NULL;
	}
}


namespace shell
{
	std::tstring GetString( STRRET* pStrRet, PCUITEMID_CHILD pidl /*= NULL*/ )
	{
		ASSERT_PTR( pStrRet );

		CComHeapPtr< TCHAR > text;
		if ( SUCCEEDED( ::StrRetToStr( pStrRet, pidl, &text ) ) )
			return text.m_pData;

		return std::tstring();
	}

	// IShellFolder properties

	std::tstring GetDisplayName( IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF flags )
	{
		STRRET strRet;
		if ( SUCCEEDED( pFolder->GetDisplayNameOf( pidl, flags, &strRet ) ) )
			return GetString( &strRet, pidl );

		return std::tstring();
	}

	std::tstring GetStringDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey )
	{
		CComVariant value;
		if ( SUCCEEDED( pFolder->GetDetailsEx( pidl, &propKey, &value ) ) )
			if ( HR_OK( value.ChangeType( VT_BSTR ) ) )
				return V_BSTR( &value );

		return std::tstring();
	}

	CTime GetDateTimeDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey )
	{
		CComVariant value;
		if ( SUCCEEDED( pFolder->GetDetailsEx( pidl, &propKey, &value ) ) )
		{
			if ( VT_DATE == value.vt )
			{
				COleDateTime dateTime( V_DATE( &value ) );
				SYSTEMTIME sysTime;
				if ( dateTime.GetAsSystemTime( sysTime ) )
					return CTime( sysTime );
			}
		}
		return CTime();
	}


	// IShellItem properties

	std::tstring GetDisplayName( IShellItem* pItem, SIGDN sigdn )
	{
		CComHeapPtr< wchar_t > name;
		if ( SUCCEEDED( pItem->GetDisplayName( sigdn, &name ) ) )
			return name.m_pData;

		return std::tstring();
	}

	std::tstring GetStringProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		CComHeapPtr< wchar_t > value;
		if ( SUCCEEDED( pItem->GetString( propKey, &value ) ) )
			return value.m_pData;

		return std::tstring();
	}

	CTime GetDateTimeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		FILETIME fileTime;
		if ( SUCCEEDED( pItem->GetFileTime( propKey, &fileTime ) ) )
			return CTime( fileTime );

		return CTime();
	}

	DWORD GetFileAttributesProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		DWORD fileAttr;
		if ( SUCCEEDED( pItem->GetUInt32( propKey, &fileAttr ) ) )
			return fileAttr;

		return UINT_MAX;
	}

	ULONGLONG GetFileSizeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		ULONGLONG fileSize;
		if ( SUCCEEDED( pItem->GetUInt64( propKey, &fileSize ) ) )
			return fileSize;

		return 0;
	}
}


namespace shell
{
	// obsolete, kept just for reference on using SHBindToParent
	//
	CComPtr< IShellFolder > _MakeChildPidlArray( std::vector< PITEMID_CHILD >& rPidlItemsArray, const std::vector< std::tstring >& filePaths )
	{
		// MSDN: we assume that all objects are in the same folder
		CComPtr< IShellFolder > pFirstParentFolder;

		ASSERT( !filePaths.empty() );

		rPidlItemsArray.reserve( rPidlItemsArray.size() + filePaths.size() );

		for ( size_t i = 0; i != filePaths.size(); ++i )
		{
			CComHeapPtr< ITEMIDLIST_ABSOLUTE > pidlAbs( static_cast< ITEMIDLIST_ABSOLUTE* >( ::ILCreateFromPath( filePaths[ i ].c_str() ) ) );
				// on 64 bit: the cast prevents warning C4090: 'argument' : different '__unaligned' qualifiers

			CComPtr< IShellFolder > pParentFolder;
			PCUITEMID_CHILD pidlItem;				// pidl relative to parent folder (not allocated)

			if ( HR_OK( ::SHBindToParent( pidlAbs, IID_IShellFolder, (void**)&pParentFolder, &pidlItem ) ) )
				rPidlItemsArray.push_back( pidl::Copy( pidlItem ) );			// copy relative pidl to pidlArray (caller must delete them)

			if ( pFirstParentFolder == NULL )
				pFirstParentFolder = pParentFolder;
		}

		return pFirstParentFolder;
	}


	namespace pidl
	{
		PIDLIST_RELATIVE GetRelativeItem( IShellFolder* pFolder, const TCHAR itemFilename[] )
		{
			ASSERT_PTR( pFolder );
			ASSERT_PTR( !str::IsEmpty( itemFilename ) );

			TCHAR displayName[ MAX_PATH * 2 ];
			_tcscpy( displayName, itemFilename );

			PIDLIST_RELATIVE childItemPidl = NULL;
			if ( HR_OK( pFolder->ParseDisplayName( NULL, NULL, displayName, NULL, &childItemPidl, NULL ) ) )
				return childItemPidl;			// allocated, client must free

			return NULL;
		}

		std::tstring GetName( LPCITEMIDLIST pidl, SIGDN nameType /*= SIGDN_NORMALDISPLAY*/ )
		{
			ASSERT_PTR( pidl );
			std::tstring name;

			TCHAR* pName;
			if ( HR_OK( ::SHGetNameFromIDList( pidl, nameType, &pName ) ) )
			{
				name = pName;
				::CoTaskMemFree( pName );
			}
			return name;
		}

		fs::CPath GetAbsolutePath( PIDLIST_ABSOLUTE pidlAbsolute, GPFIDL_FLAGS optFlags /*= GPFIDL_DEFAULT*/ )
		{
			fs::CPath fullPath;
			if ( !IsNull( pidlAbsolute ) )
			{
				TCHAR buffFullPath[ MAX_PATH * 2 ];
				if ( ::SHGetPathFromIDListEx( pidlAbsolute, buffFullPath, _countof( buffFullPath ), optFlags ) )
					fullPath.Set( buffFullPath );
			}
			else
				ASSERT( false );

			return fullPath;
		}

		size_t GetCount( LPCITEMIDLIST pidl )
		{
			UINT count = 0;

			if ( pidl != NULL )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					++count;

			return count;
		}

		size_t GetByteSize( LPCITEMIDLIST pidl )
		{
			size_t size = 0;

			if ( pidl != NULL )
			{
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					size += pidl->mkid.cb;

				size += sizeof( WORD );		// add the size of the NULL terminating ITEMIDLIST
			}
			return size;
		}

		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST nextPidl = NULL;
			if ( pidl != NULL )
				nextPidl = (LPITEMIDLIST)( impl::GetBuffer( pidl ) + pidl->mkid.cb );

			return nextPidl;
		}

		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl )
		{
			// Get the PIDL of the last item in the list
			LPCITEMIDLIST lastPidl = NULL;
			if ( pidl != NULL )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					lastPidl = pidl;

			return lastPidl;
		}

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST targetPidl = NULL;

			if ( pidl != NULL )
			{
				size_t size = GetByteSize( pidl );

				targetPidl = Allocate( size );				// allocate the new pidl
				if ( targetPidl != NULL )
					::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
			}

			return targetPidl;
		}

		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST targetPidl = NULL;

			if ( pidl != NULL )
			{
				size_t size = pidl->mkid.cb + sizeof( WORD );

				targetPidl = Allocate( size );				// allocate the new pidl
				if ( targetPidl != NULL )
				{
					::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
					*(WORD*) ( ( (BYTE*)targetPidl ) + targetPidl->mkid.cb ) = 0;		// add terminator
				}
			}
			return targetPidl;
		}

		pred::CompareResult Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder /*= GetDesktopFolder()*/ )
		{
			ASSERT_PTR( pParentFolder );

			HRESULT hResult = pParentFolder->CompareIDs( SHCIDS_CANONICALONLY, leftPidl, rightPidl );
			return pred::ToCompareResult( (short)HRESULT_CODE( hResult ) );
		}
	}
}


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
		else if ( !pidl::IsNull( rightPidl ) )
		{
			size_t oldSize = GetByteSize() - sizeof( WORD );
			size_t rightSize = pidl::GetByteSize( rightPidl );

			LPITEMIDLIST newPidl = pidl::Allocate( oldSize + rightSize );
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
		else if ( !pidl::IsNull( childPidl ) )
		{
			size_t oldSize = GetByteSize() - sizeof( WORD );
			size_t childSize = childPidl->mkid.cb;

			LPITEMIDLIST pidlNew = pidl::Allocate( oldSize + childSize + sizeof( WORD ) );
			if ( pidlNew != NULL )
			{
				::CopyMemory( pidlNew, m_pidl, oldSize );
				::CopyMemory( ((LPBYTE)pidlNew) + oldSize, childPidl, childSize );
				pidl::impl::SetTerminator( pidlNew, oldSize + childSize );
			}
			Reset( pidlNew );
		}
	}

	void CPidl::RemoveLast( void )
	{
		if ( LPITEMIDLIST lastPidl = const_cast< LPITEMIDLIST >( pidl::GetLastItem( m_pidl ) ) )
			lastPidl->mkid.cb = 0;
	}


	// CPidl advanced

	bool CPidl::CreateAbsolute( const TCHAR fullPath[] )
	{
		ASSERT( !str::IsEmpty( fullPath ) );
		Reset( ::ILCreateFromPath( fullPath ) );
		return !IsNull();

		/*	equivalent implementation:
		if ( HR_OK( GetDesktopFolder()->ParseDisplayName( NULL, NULL, const_cast< TCHAR* >( fullPath ), NULL, &m_pidl, NULL ) ) )
			return true;
		Delete();
		return false; */
	}

	bool CPidl::CreateRelative( IShellFolder* pFolder, const TCHAR itemFilename[] )
	{
		Reset( pidl::GetRelativeItem( pFolder, itemFilename ) );
		return !IsNull();
	}

	bool CPidl::CreateFrom( IUnknown* pUnknown )
	{
		ASSERT_PTR( pUnknown );

		return HR_OK( ::SHGetIDListFromObject( pUnknown, &*this ) );
	}

	bool CPidl::CreateFromFolder( IShellFolder* pShellFolder )
	{
		CComQIPtr< IPersistFolder2 > pFolder( pShellFolder );
		if ( pFolder != NULL )
			if ( HR_OK( pFolder->GetCurFolder( &*this ) ) )
				return true;

		Delete();
		return false;
	}

	CComPtr< IShellItem > CPidl::FindItem( IShellFolder* pParentFolder ) const
	{
		CComPtr< IShellItem > pShellItem;

		if ( pParentFolder != NULL )
		{
			if ( HR_OK( ::SHCreateItemWithParent( NULL, pParentFolder, m_pidl, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}
		else
		{
			if ( HR_OK( ::SHCreateItemFromParsingName( GetAbsolutePath().GetPtr(), NULL, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}

		return NULL;
	}

	CComPtr< IShellFolder > CPidl::FindFolder( IShellFolder* pParentFolder /*= GetDesktopFolder()*/ ) const
	{
		ASSERT_PTR( pParentFolder );

		CComPtr< IShellFolder > pShellFolder;
		if ( HR_OK( pParentFolder->BindToObject( m_pidl, NULL, IID_PPV_ARGS( &pShellFolder ) ) ) )
			return pShellFolder;

		return NULL;
	}

	bool CPidl::WriteToStream( IStream* pStream ) const
	{
		ULONG bytesWritten = 0;
		UINT size = static_cast< UINT >( GetByteSize() );

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
}


namespace shell
{
	std::tstring FormatByteSize( UINT64 fileSize )
	{
		TCHAR buffer[ 32 ];
		::StrFormatByteSize64( fileSize, buffer, COUNT_OF( buffer ) );
		return buffer;
	}

	std::tstring FormatKiloByteSize( UINT64 fileSize )
	{
		TCHAR buffer[ 32 ];
		::StrFormatKBSize( fileSize, buffer, COUNT_OF( buffer ) );
		return buffer;
	}


	CImageList* GetSysImageList( ui::GlyphGauge glyphGauge )
	{
		return CSysImageLists::Instance().Get( glyphGauge );
	}

	int GetFileSysImageIndex( const TCHAR* filePath )
	{
		SHFILEINFO fileInfo;
		::SHGetFileInfo( filePath, NULL, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX );
		return fileInfo.iIcon;
	}

	HICON GetFileSysIcon( const TCHAR* filePath, UINT flags /*= SHGFI_SMALLICON*/ )
	{
		SHFILEINFO fileInfo;
		::SHGetFileInfo( filePath, NULL, &fileInfo, sizeof( fileInfo ), SHGFI_ICON | flags );
		return fileInfo.hIcon;
	}


	// CSysImageLists implementation

	CSysImageLists::~CSysImageLists()
	{
		// release image-lists since they're owned by Explorer
		m_imageLists[ ui::SmallGlyph ].Detach();
		m_imageLists[ ui::LargeGlyph ].Detach();
	}

	CSysImageLists& CSysImageLists::Instance( void )
	{
		static CSysImageLists s_sysImages;
		return s_sysImages;
	}

	CImageList* CSysImageLists::Get( ui::GlyphGauge glyphGauge )
	{
		CImageList* pImageList = &m_imageLists[ glyphGauge ];

		if ( NULL == pImageList->GetSafeHandle() )
		{
			SHFILEINFO fileInfo;
			UINT flags = SHGFI_SYSICONINDEX;
			SetFlag( flags, ui::LargeGlyph == glyphGauge ? SHGFI_LARGEICON : SHGFI_SMALLICON );

			if ( HIMAGELIST hSysImageList = (HIMAGELIST)::SHGetFileInfo( _T("C:\\"), NULL, &fileInfo, sizeof( fileInfo ), flags ) )
				pImageList->Attach( hSysImageList );
		}

		return pImageList;
	}
}


namespace shell
{
	pred::CompareResult ExplorerCompare( const wchar_t* pLeftPath, const wchar_t* pRightPath )
	{
		if ( path::EquivalentPtr( pLeftPath, pRightPath ) )
			return pred::Equal;

		return pred::ToCompareResult( ::StrCmpLogicalW( pLeftPath, pRightPath ) );
	}
}
