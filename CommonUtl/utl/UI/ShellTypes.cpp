
#include "stdafx.h"
#include "ShellTypes.h"
#include "utl/StdHashValue.h"
#include <atlcomtime.h>		// for COleDateTime

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// verbatim from <src/mfc/afximpl.h>
class AFX_COM
{
public:
	HRESULT CreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv );
	HRESULT GetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv );
};


namespace shell
{
	bool ResolveShortcut( fs::CPath& rDestPath, const TCHAR* pShortcutLnkPath, CWnd* pWnd /*= nullptr*/ )
	{
		if ( nullptr == pWnd )
			pWnd = AfxGetMainWnd();

		SHFILEINFO linkInfo;

		if ( 0 == ::SHGetFileInfo( pShortcutLnkPath, 0, &linkInfo, sizeof( linkInfo ), SHGFI_ATTRIBUTES ) ||
			 !::HasFlag( linkInfo.dwAttributes, SFGAO_LINK ) ||
			 nullptr == pWnd->GetSafeHwnd() )
			return false;

		AFX_COM com;
		CComPtr<IShellLink> pShellLink;

		if ( !HR_OK( com.CreateInstance( CLSID_ShellLink, nullptr, IID_IShellLink, (LPVOID*)&pShellLink ) ) || pShellLink == nullptr )
			return false;

		if ( CComQIPtr<IPersistFile> pPersistFile = pShellLink.p )
			if ( HR_OK( pPersistFile->Load( pShortcutLnkPath, STGM_READ ) ) )
				if ( HR_OK( pShellLink->Resolve( pWnd->GetSafeHwnd(), SLR_ANY_MATCH ) ) )		// resolve the link; this may post UI to find the link
				{
					TCHAR destPath[ MAX_PATH ];

					pShellLink->GetPath( ARRAY_SPAN( destPath ), nullptr, 0 );
					rDestPath.Set( destPath );
					return true;
				}

		return false;
	}
}


namespace shell
{
	CComPtr<IShellFolder> GetDesktopFolder( void )
	{
		CComPtr<IShellFolder> pDesktopFolder;
		if ( HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
			return pDesktopFolder;

		return nullptr;
	}

	CComPtr<IShellFolder> FindShellFolder( const TCHAR* pDirPath )
	{
		CComPtr<IShellFolder> pDirFolder;

		if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
		{
			CComHeapPtr<ITEMIDLIST> pidlFolder( static_cast<ITEMIDLIST_RELATIVE*>( pidl::GetRelativeItem( pDesktopFolder, pDirPath ) ) );
			if ( pidlFolder.m_pData != nullptr )
				HR_AUDIT( pDesktopFolder->BindToObject( pidlFolder, nullptr, IID_PPV_ARGS( &pDirFolder ) ) );
		}

		return pDirFolder;
	}


	CComPtr<IShellItem> FindShellItem( const fs::CPath& fullPath )
	{
		CComPtr<IShellItem> pShellItem;
		if ( HR_OK( ::SHCreateItemFromParsingName( fullPath.GetPtr(), nullptr, IID_PPV_ARGS( &pShellItem ) ) ) )
			return pShellItem;
		return nullptr;
	}

	CComPtr<IShellFolder2> ToShellFolder( IShellItem* pFolderItem )
	{
		ASSERT_PTR( pFolderItem );

		CComPtr<IShellFolder2> pShellFolder;

		CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlFolder;
		if ( HR_OK( ::SHGetIDListFromObject( pFolderItem, &pidlFolder ) ) )
			if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
				if ( HR_OK( pDesktopFolder->BindToObject( pidlFolder, nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) )
					return pShellFolder;

		return pShellFolder;
	}

	CComPtr<IShellFolder2> GetParentFolderAndPidl( ITEMID_CHILD** pPidlItem, IShellItem* pShellItem )
	{
		if ( CComQIPtr<IParentAndItem> pParentAndItem = pShellItem )
		{
			CComPtr<IShellFolder> pParentFolder;
			if ( SUCCEEDED( pParentAndItem->GetParentAndItem( nullptr, &pParentFolder, pPidlItem ) ) )
				return CComQIPtr<IShellFolder2>( pParentFolder );
		}

		return nullptr;
	}
}


namespace shell
{
	std::tstring GetString( STRRET* pStrRet, PCUITEMID_CHILD pidl /*= nullptr*/ )
	{
		ASSERT_PTR( pStrRet );

		CComHeapPtr<TCHAR> text;
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
		CComHeapPtr<wchar_t> name;
		if ( SUCCEEDED( pItem->GetDisplayName( sigdn, &name ) ) )
			return name.m_pData;

		return std::tstring();
	}

	std::tstring GetStringProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		CComHeapPtr<wchar_t> value;
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
	CComPtr<IShellFolder> _MakeChildPidlArray( std::vector< PITEMID_CHILD >& rPidlItemsArray, const std::vector< std::tstring >& filePaths )
	{
		// MSDN: we assume that all objects are in the same folder
		CComPtr<IShellFolder> pFirstParentFolder;

		ASSERT( !filePaths.empty() );

		rPidlItemsArray.reserve( rPidlItemsArray.size() + filePaths.size() );

		for ( size_t i = 0; i != filePaths.size(); ++i )
		{
			CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlAbs( static_cast<ITEMIDLIST_ABSOLUTE*>( ::ILCreateFromPath( filePaths[ i ].c_str() ) ) );
				// on 64 bit: the cast prevents warning C4090: 'argument' : different '__unaligned' qualifiers

			CComPtr<IShellFolder> pParentFolder;
			PCUITEMID_CHILD pidlItem;				// pidl relative to parent folder (not allocated)

			if ( HR_OK( ::SHBindToParent( pidlAbs, IID_IShellFolder, (void**)&pParentFolder, &pidlItem ) ) )
				rPidlItemsArray.push_back( pidl::Copy( pidlItem ) );			// copy relative pidl to pidlArray (caller must delete them)

			if ( pFirstParentFolder == nullptr )
				pFirstParentFolder = pParentFolder;
		}

		return pFirstParentFolder;
	}


	namespace pidl
	{
		PIDLIST_RELATIVE GetRelativeItem( IShellFolder* pFolder, const TCHAR itemFilename[] )
		{
			ASSERT_PTR( pFolder );
			ASSERT( !str::IsEmpty( itemFilename ) );

			TCHAR displayName[ MAX_PATH * 2 ];
			_tcscpy( displayName, itemFilename );

			PIDLIST_RELATIVE childItemPidl = nullptr;
			if ( HR_OK( pFolder->ParseDisplayName( nullptr, nullptr, displayName, nullptr, &childItemPidl, nullptr ) ) )
				return childItemPidl;			// allocated, client must free

			return nullptr;
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

			if ( pidl != nullptr )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					++count;

			return count;
		}

		size_t GetByteSize( LPCITEMIDLIST pidl )
		{
			size_t size = 0;

			if ( pidl != nullptr )
			{
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					size += pidl->mkid.cb;

				size += sizeof( WORD );		// add the size of the NULL terminating ITEMIDLIST
			}
			return size;
		}

		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST nextPidl = nullptr;
			if ( pidl != nullptr )
				nextPidl = (LPITEMIDLIST)( impl::GetBuffer( pidl ) + pidl->mkid.cb );

			return nextPidl;
		}

		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl )
		{
			// Get the PIDL of the last item in the list
			LPCITEMIDLIST lastPidl = nullptr;
			if ( pidl != nullptr )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					lastPidl = pidl;

			return lastPidl;
		}

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST targetPidl = nullptr;

			if ( pidl != nullptr )
			{
				size_t size = GetByteSize( pidl );

				targetPidl = Allocate( size );				// allocate the new pidl
				if ( targetPidl != nullptr )
					::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
			}

			return targetPidl;
		}

		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl )
		{
			LPITEMIDLIST targetPidl = nullptr;

			if ( pidl != nullptr )
			{
				size_t size = pidl->mkid.cb + sizeof( WORD );

				targetPidl = Allocate( size );				// allocate the new pidl
				if ( targetPidl != nullptr )
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
	CPidl::CPidl( LPCITEMIDLIST rootPidl, LPCITEMIDLIST dirPathPidl, LPCITEMIDLIST childPidl /*= nullptr*/ )
	{
		AssignCopy( rootPidl );
		Concatenate( dirPathPidl );

		if ( childPidl != nullptr )
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
			if ( newPidl != nullptr )
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
			if ( pidlNew != nullptr )
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
		if ( LPITEMIDLIST lastPidl = const_cast<LPITEMIDLIST>( pidl::GetLastItem( m_pidl ) ) )
			lastPidl->mkid.cb = 0;
	}


	// CPidl advanced

	bool CPidl::CreateAbsolute( const TCHAR fullPath[] )
	{
		ASSERT( !str::IsEmpty( fullPath ) );
		Reset( ::ILCreateFromPath( fullPath ) );
		return !IsNull();

		/*	equivalent implementation:
		if ( HR_OK( GetDesktopFolder()->ParseDisplayName( nullptr, nullptr, const_cast<TCHAR* >( fullPath ), nullptr, &m_pidl, nullptr ) ) )
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
		CComQIPtr<IPersistFolder2> pFolder( pShellFolder );
		if ( pFolder != nullptr )
			if ( HR_OK( pFolder->GetCurFolder( &*this ) ) )
				return true;

		Delete();
		return false;
	}

	CComPtr<IShellItem> CPidl::FindItem( IShellFolder* pParentFolder ) const
	{
		CComPtr<IShellItem> pShellItem;

		if ( pParentFolder != nullptr )
		{
			if ( HR_OK( ::SHCreateItemWithParent( nullptr, pParentFolder, m_pidl, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}
		else
		{
			if ( HR_OK( ::SHCreateItemFromParsingName( GetAbsolutePath().GetPtr(), nullptr, IID_PPV_ARGS( &pShellItem ) ) ) )
				return pShellItem;
		}

		return nullptr;
	}

	CComPtr<IShellFolder> CPidl::FindFolder( IShellFolder* pParentFolder /*= GetDesktopFolder()*/ ) const
	{
		ASSERT_PTR( pParentFolder );

		CComPtr<IShellFolder> pShellFolder;
		if ( HR_OK( pParentFolder->BindToObject( m_pidl, nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) )
			return pShellFolder;

		return nullptr;
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

			AssignCopy( 0 == bytesRead ? nullptr : pidl );
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
		::SHGetFileInfo( filePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX );
		return fileInfo.iIcon;
	}

	HICON GetFileSysIcon( const TCHAR* filePath, UINT flags /*= SHGFI_SMALLICON*/ )
	{
		SHFILEINFO fileInfo;
		::SHGetFileInfo( filePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ICON | flags );
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

		if ( nullptr == pImageList->GetSafeHandle() )
		{
			SHFILEINFO fileInfo;
			UINT flags = SHGFI_SYSICONINDEX;
			SetFlag( flags, ui::LargeGlyph == glyphGauge ? SHGFI_LARGEICON : SHGFI_SMALLICON );

			if ( HIMAGELIST hSysImageList = (HIMAGELIST)::SHGetFileInfo( _T("C:\\"), 0, &fileInfo, sizeof( fileInfo ), flags ) )
				pImageList->Attach( hSysImageList );
		}

		return pImageList;
	}
}


namespace shell
{
	pred::CompareResult ExplorerCompare( const wchar_t* pLeftPath, const wchar_t* pRightPath )
	{
		if ( path::Equivalent( pLeftPath, pRightPath ) )
			return pred::Equal;

		return pred::ToCompareResult( ::StrCmpLogicalW( pLeftPath, pRightPath ) );
	}
}
