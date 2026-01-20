
#include "pch.h"
#include "ShellTypes.h"
#include "utl/Algorithms_fwd.h"
#include "utl/IUnknownImpl.h"
#include "utl/StdHashValue.h"
#include <atlcomtime.h>		// for COleDateTime

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr<IShellFolder> GetDesktopFolder( void )
	{
		CComPtr<IShellFolder> pDesktopFolder;

		if ( !HR_OK( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
			return nullptr;

		return pDesktopFolder;
	}

	CComPtr<IShellFolder> FindShellFolder( const TCHAR* pDirPath )
	{
		CComPtr<IShellFolder> pDirFolder;

		if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
		{
			CComHeapPtr<ITEMIDLIST_RELATIVE> pidlFolder( static_cast<ITEMIDLIST_RELATIVE*>( pidl::CreateRelativePidl( pDesktopFolder, pDirPath ) ) );
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
	// Implements IFileSystemBindData interface, used to setup a bind context for a file that may not exist in the file system.
	//
	class CVirtualFileSysBindData : public utl::IUnknownImpl<IFileSystemBindData>
	{
	protected:
		CVirtualFileSysBindData( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL )
		{
			utl::ZeroStruct( &m_findData );
			m_findData.dwFileAttributes = fileAttribute;
		}
	public:
		static CComPtr<IFileSystemBindData> CreateInstance( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL )		// could be FILE_ATTRIBUTE_DIRECTORY
		{
			CComPtr<IFileSystemBindData> pFileSysBindData;

			pFileSysBindData.Attach( new CVirtualFileSysBindData( fileAttribute ) );
			return pFileSysBindData;
		}

		// IFileSystemBindData interface:
		IFACEMETHODIMP SetFindData( const WIN32_FIND_DATAW* pFindData ) { m_findData = *pFindData; return S_OK; }
		IFACEMETHODIMP GetFindData( OUT WIN32_FIND_DATAW* pDestFindData ) { *pDestFindData = m_findData; return S_OK; }
	private:
		WIN32_FIND_DATA m_findData;
	};


	CComPtr<IBindCtx> CreateFileSysBindContext( DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
	{	// allows parsing of virtual paths, that don't exist physically (e.g. a deleted file), to be validated later on.
		CComPtr<IBindCtx> pBindContext;
		BIND_OPTS bindOpts = { sizeof( bindOpts ), 0, STGM_CREATE, 0 };		// Gemini says that STGM_READ would suffice; the default is STGM_READWRITE

		if ( HR_OK( ::CreateBindCtx( 0, &pBindContext ) ) )
			if ( HR_OK( pBindContext->SetBindOptions( &bindOpts ) ) )
				// create our manual bind data object
				if ( CComPtr<IFileSystemBindData> pFileSysBindData = CVirtualFileSysBindData::CreateInstance( fileAttribute ) )
					// register it so that ::SHParseDisplayName can see the virtual (non-existing) metadata
					if ( HR_OK( pBindContext->RegisterObjectParam( const_cast<LPOLESTR>( STR_FILE_SYS_BIND_DATA ), pFileSysBindData ) ) )
						return pBindContext;

		return nullptr;
	}
}


namespace shell
{
	// shell file info (via SHFILEINFO):

	SFGAOF GetFileSysAttributes( const TCHAR* pFilePath )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ATTRIBUTES ) )
			return 0;

		return fileInfo.dwAttributes;
	}

	int GetFileSysImageIndex( const TCHAR* pFilePath )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX ) )
			return -1;

		return fileInfo.iIcon;
	}

	HICON GetFileSysIcon( const TCHAR* pFilePath, UINT flags /*= SHGFI_SMALLICON*/ )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ICON | flags ) )
			return nullptr;

		return fileInfo.hIcon;
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

	std::tstring GetDisplayName( IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF flags /*= SHGDN_NORMAL*/ )
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
	CComPtr<IShellFolder> _MakeChildPidlArray( std::vector<PITEMID_CHILD>& rPidlItemsArray, const std::vector<std::tstring>& filePaths )
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
				rPidlItemsArray.push_back( ::ILCloneChild( pidlItem ) );			// copy relative pidl to pidlArray (caller must delete them)

			if ( pFirstParentFolder == nullptr )
				pFirstParentFolder = pParentFolder;
		}

		return pFirstParentFolder;
	}
}


namespace shell
{
	namespace pidl
	{
		PIDLIST_RELATIVE CreateRelativePidl( IShellFolder* pParentFolder, const TCHAR* pRelPath )
		{
			ASSERT_PTR( pParentFolder );
			ASSERT( !str::IsEmpty( pRelPath ) );

			TCHAR displayName[ MAX_PATH * 2 ];
			_tcscpy( displayName, pRelPath );

			PIDLIST_RELATIVE childItemPidl = nullptr;
			if ( HR_OK( pParentFolder->ParseDisplayName( nullptr, nullptr, displayName, nullptr, &childItemPidl, nullptr ) ) )
				return childItemPidl;			// allocated, caller must free

			return nullptr;
		}

		/* Pidl name formatting examples:
			Pidl on "C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice.png":
				SIGDN_NORMALDISPLAY:			"Dice.png"
				SIGDN_PARENTRELATIVEPARSING:	"Dice.png"
				SIGDN_DESKTOPABSOLUTEPARSING:	"C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice.png"
				SIGDN_PARENTRELATIVEEDITING:	"Dice.png"
				SIGDN_DESKTOPABSOLUTEEDITING:	"C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice.png"
				SIGDN_URL:						"file:///C:/dev/code/DevTools/TestDataUtl/images/Dice.png"
				SIGDN_PARENTRELATIVEFORADDRESSBAR:	"Dice.png"
				SIGDN_PARENTRELATIVE:			"Dice.png"
				SIGDN_PARENTRELATIVEFORUI:		"Dice.png"

			Pidl on "Control Panel > Region":
				SIGDN_NORMALDISPLAY:			"Region"
				SIGDN_PARENTRELATIVEPARSING:	"::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
				SIGDN_DESKTOPABSOLUTEPARSING:	"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
				SIGDN_PARENTRELATIVEEDITING:	"Region"
				SIGDN_DESKTOPABSOLUTEEDITING:	"Control Panel\\All Control Panel Items\\Region"
				SIGDN_URL:						""	hResult=E_NOTIMPL
				SIGDN_PARENTRELATIVEFORADDRESSBAR: "Region"
				SIGDN_PARENTRELATIVE:			"Region"
				SIGDN_PARENTRELATIVEFORUI:		"Region"
		*/

		std::tstring GetName( PCIDLIST_ABSOLUTE pidl, SIGDN nameType /*= SIGDN_NORMALDISPLAY*/ )
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

		fs::CPath GetAbsolutePath( PCIDLIST_ABSOLUTE pidlAbsolute, GPFIDL_FLAGS optFlags /*= GPFIDL_DEFAULT*/ )
		{
			fs::CPath fullPath;

			if ( !IsNull( pidlAbsolute ) )
			{
				TCHAR buffer[ MAX_PATH * 2 ];

				if ( ::SHGetPathFromIDListEx( pidlAbsolute, ARRAY_SPAN( buffer ), optFlags ) )
					fullPath.Set( buffer );
			}
			else
				ASSERT( false );

			return fullPath;
		}


		// This performs the same function as SHSimpleIDListFromPath(), which is deprecated.
		//
		// pFullPathOrName: could be either a file/folder path, or a display name formatted with SIGDN_DESKTOPABSOLUTEPARSING.
		//	Examples:
		//		"C:\dev\code\DevTools\TestDataUtl\images\Dice.png"
		//		"::{26EE0668-A00A-44D7-9371-BEB064C98683}\0\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
		//			- display name formatted with SIGDN_DESKTOPABSOLUTEPARSING,
		//			  corresponding to "Control Panel\All Control Panel Items\Region" (SIGDN_DESKTOPABSOLUTEEDITING).
		// pBindCtx: pass the bind context created with shell::CreateFileSysBindContext() to parse virtual path items (non-existent in the file system).
		//
		PIDLIST_ABSOLUTE ParseToPidl( const TCHAR* pPathOrName, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			PIDLIST_ABSOLUTE pidlAbs = nullptr;

			// This will parse non-existent files and Control Panel apps correctly.
			// For non-existent files/folders pass a pBindCtx created with CreateFileSysBindContext()
			if ( !HR_OK( ::SHParseDisplayName( pPathOrName, pBindCtx, &pidlAbs, 0, nullptr ) ) )
				return nullptr;

			return pidlAbs;
		}

		PIDLIST_RELATIVE ParseToPidlRelative( const TCHAR* pRelPathOrName, IShellFolder* pParentFolder, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			PIDLIST_RELATIVE pidlRel = nullptr;

			if ( nullptr == pParentFolder )
				return reinterpret_cast<PIDLIST_RELATIVE>( ParseToPidl( pRelPathOrName, pBindCtx ) );

			if ( !HR_OK( pParentFolder->ParseDisplayName( nullptr, pBindCtx, const_cast<LPWSTR>( pRelPathOrName ), nullptr, &pidlRel, nullptr ) ) )
				return nullptr;

			return pidlRel;
		}

		PIDLIST_ABSOLUTE ParseToPidlAbsolute( const TCHAR* pRelPathOrName, IShellFolder* pParentFolder, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			CComHeapPtr<ITEMIDLIST_RELATIVE> pidlRel;

			pidlRel.Attach( ParseToPidlRelative( pRelPathOrName, pParentFolder, pBindCtx ) );
			if ( pidlRel != nullptr )
			{
				// convert to absolute pidl:
				CComHeapPtr<ITEMIDLIST_ABSOLUTE> parentPidlAbs;
				if ( HR_OK( ::SHGetIDListFromObject( pParentFolder, &parentPidlAbs ) ) )
				{
					PIDLIST_ABSOLUTE pidlAbs = ::ILCombine( parentPidlAbs, pidlRel.m_pData );
					return pidlAbs;
				}
			}

			return nullptr;
		}


		size_t GetItemCount( LPCITEMIDLIST pidl )
		{
			UINT count = 0;

			if ( pidl != nullptr )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					++count;

			return count;
		}

		size_t GetByteSize( LPCITEMIDLIST pidl )
		{	// AKA ::ILGetSize for non-null pidls
			size_t size = 0;

			if ( pidl != nullptr )
			{
			#if (1)
				size = ::ILGetSize( reinterpret_cast<PCUIDLIST_RELATIVE>( pidl ) );
			#else
				// manual computation
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					size += pidl->mkid.cb;

				size += sizeof( WORD );		// add the size of the NULL terminating ITEMIDLIST
			#endif
			}
			return size;
		}

		LPCITEMIDLIST GetNextItem( LPCITEMIDLIST pidl )
		{	// AKA ::ILNext()
			LPITEMIDLIST nextPidl = nullptr;
			if ( pidl != nullptr )
				nextPidl = (LPITEMIDLIST)( impl::GetBuffer( pidl ) + pidl->mkid.cb );

			return nextPidl;
		}

		LPCITEMIDLIST GetLastItem( LPCITEMIDLIST pidl )
		{	// AKA ::ILFindLastID()
			// Get the PIDL of the last item in the list
			LPCITEMIDLIST lastPidl = nullptr;
			if ( pidl != nullptr )
				for ( ; pidl->mkid.cb > 0; pidl = GetNextItem( pidl ) )
					lastPidl = pidl;

			return lastPidl;
		}

		LPITEMIDLIST Copy( LPCITEMIDLIST pidl )
		{	// AKA ::ILClone(), ::ILCloneFull(), ::ILCloneChild()
			LPITEMIDLIST targetPidl = nullptr;

			if ( pidl != nullptr )
			{
				size_t size = GetByteSize( pidl );

				targetPidl = pidl::Allocate( size );			// allocate the new pidl
				if ( targetPidl != nullptr )
					::CopyMemory( targetPidl, pidl, size );		// copy the source to the target
			}

			return targetPidl;
		}

		LPITEMIDLIST CopyFirstItem( LPCITEMIDLIST pidl )
		{	// AKA ::ILCloneFirst()
			LPITEMIDLIST targetPidl = nullptr;

			if ( pidl != nullptr )
			{
				size_t size = pidl->mkid.cb + sizeof( WORD );

				targetPidl = Allocate( size );					// allocate the new pidl
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
	bool WriteToStream( IStream* pStream, IN LPCITEMIDLIST pidl )
	{	// superseeded by ::ILSaveToStream()
		UINT size = static_cast<UINT>( shell::pidl::GetByteSize( pidl ) );
		ULONG bytesWritten = 0;

		if ( HR_OK( pStream->Write( &size, sizeof( size ), &bytesWritten ) ) )
			if ( nullptr == pidl || HR_OK( pStream->Write( pidl, size, &bytesWritten ) ) )
				return true;

		return false;
	}

	bool ReadFromStream( IStream* pStream, OUT LPITEMIDLIST* pPidl )
	{	// superseeded by ::ILLoadFromStreamEx()
		UINT size = 0;
		ULONG bytesRead = 0;

		*pPidl = nullptr;
		if ( HR_OK( pStream->Read( &size, sizeof( size ), &bytesRead ) ) )
			if ( size != 0 )
			{
				CComHeapPtr<ITEMIDLIST> newPidl;
				newPidl.AllocateBytes( size );

				if ( !HR_OK( pStream->Read( newPidl, size, &bytesRead ) ) )
					return false;

				*pPidl = newPidl.Detach();
			}

		return size == bytesRead;
	}
}


namespace shell
{
	std::tstring FormatByteSize( UINT64 fileSize )
	{
		TCHAR buffer[ 32 ];
		::StrFormatByteSize64( fileSize, ARRAY_SPAN( buffer ) );
		return buffer;
	}

	std::tstring FormatKiloByteSize( UINT64 fileSize )
	{
		TCHAR buffer[ 32 ];
		::StrFormatKBSize( fileSize, ARRAY_SPAN( buffer ) );
		return buffer;
	}


	CImageList* GetSysImageList( ui::GlyphGauge glyphGauge )
	{
		return CSysImageLists::Instance().Get( glyphGauge );
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
