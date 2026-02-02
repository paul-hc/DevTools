
#include "pch.h"
#include "ShellTypes.h"
#include "ShellPidl.h"
#include "utl/Algorithms_fwd.h"
#include "utl/FlagTags.h"
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

	CComPtr<IShellFolder> MakeShellFolder( const TCHAR* pFolderShellPath )
	{
		CComPtr<IShellFolder> pDirFolder;

		if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
		{
			CComHeapPtr<ITEMIDLIST_RELATIVE> folderRelPidl;

			folderRelPidl.Attach( pidl::ParseToRelativePidl( pDesktopFolder, pFolderShellPath ) );
			if ( folderRelPidl.m_pData != nullptr )
				if ( HR_OK( pDesktopFolder->BindToObject( folderRelPidl, nullptr, IID_PPV_ARGS( &pDirFolder ) ) ) )
					return pDirFolder;
		}

		return nullptr;
	}

	PIDLIST_ABSOLUTE MakeFolderPidl( IShellFolder* pFolder )
	{
		ASSERT_PTR( pFolder );
		if ( CComQIPtr<IPersistFolder2> pPersistFolder = pFolder )
		{
			PIDLIST_ABSOLUTE folderPidl;
			if ( HR_OK( pPersistFolder->GetCurFolder( &folderPidl ) ) )
				return folderPidl;		// caller must delete the pidl
		}
		return nullptr;
	}

	std::tstring GetFolderName( IShellFolder* pFolder, SIGDN nameType /*= SIGDN_NORMALDISPLAY*/ )
	{
		CPidlAbsolute folderPidl( MakeFolderPidl( pFolder ) );
		shell::TPath folderShellPath;

		if ( folderPidl.IsNull() )
			return str::GetEmpty();

		return folderPidl.GetName( nameType );
	}

	shell::TPath GetFolderPath( IShellFolder* pFolder )
	{
		CPidlAbsolute folderPidl( MakeFolderPidl( pFolder ) );
		shell::TPath folderShellPath;

		if ( !folderPidl.IsNull() )
			folderShellPath = folderPidl.ToShellPath();

		return folderShellPath;
	}


	CComPtr<IShellItem> MakeShellItem( const TCHAR* pShellPath )
	{
		ASSERT_PTR( pShellPath );

		CComPtr<IShellItem> pShellItem;

		if ( !HR_OK( ::SHCreateItemFromParsingName( pShellPath, nullptr, IID_PPV_ARGS( &pShellItem ) ) ) )
			return nullptr;

		return pShellItem;
	}

	CComPtr<IShellFolder2> ToShellFolder( IShellItem* pFolderItem )
	{
		ASSERT_PTR( pFolderItem );

		CComPtr<IShellFolder2> pShellFolder;

		CComHeapPtr<ITEMIDLIST_ABSOLUTE> folderAbsPidl;
		if ( HR_OK( ::SHGetIDListFromObject( pFolderItem, &folderAbsPidl ) ) )
			if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
				if ( HR_OK( pDesktopFolder->BindToObject( folderAbsPidl, nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) )
					return pShellFolder;

		return pShellFolder;
	}

	CComPtr<IShellFolder2> GetParentFolderAndPidl( OUT PITEMID_CHILD* pPidlItem, IShellItem* pShellItem )
	{
		if ( CComQIPtr<IParentAndItem> pParentAndItem = pShellItem )
		{
			CComPtr<IShellFolder> pParentFolder;
			if ( HR_OK( pParentAndItem->GetParentAndItem( nullptr, &pParentFolder, pPidlItem ) ) )
				return CComQIPtr<IShellFolder2>( pParentFolder );
		}

		return nullptr;
	}

	PIDLIST_ABSOLUTE MakeItemPidl( IShellItem* pShellItem )
	{
		PIDLIST_ABSOLUTE itemPidl;

		if ( !HR_OK( ::SHGetIDListFromObject( pShellItem, &itemPidl ) ) )
			return nullptr;

		return itemPidl;
	}
}


namespace shell
{
	// Implements IFileSystemBindData interface, used to setup a bind context for a file that may not exist in the file system.
	//
	class CProxyFileSysBindData : public utl::IUnknownImpl<IFileSystemBindData>
	{
	protected:
		CProxyFileSysBindData( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL )
		{
			utl::ZeroStruct( &m_findData );
			m_findData.dwFileAttributes = fileAttribute;
		}
	public:
		static CComPtr<IFileSystemBindData> CreateInstance( DWORD fileAttribute = FILE_ATTRIBUTE_NORMAL )		// could be FILE_ATTRIBUTE_DIRECTORY
		{
			CComPtr<IFileSystemBindData> pFileSysBindData;

			pFileSysBindData.Attach( new CProxyFileSysBindData( fileAttribute ) );
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
				if ( CComPtr<IFileSystemBindData> pFileSysBindData = CProxyFileSysBindData::CreateInstance( fileAttribute ) )
					// register it so that ::SHParseDisplayName can see the virtual (non-existing) metadata
					if ( HR_OK( pBindContext->RegisterObjectParam( const_cast<LPOLESTR>( STR_FILE_SYS_BIND_DATA ), pFileSysBindData ) ) )
						return pBindContext;

		return nullptr;
	}

	CComPtr<IBindCtx> AutoFileSysBindContext( const TCHAR* pShellPath, IBindCtx* pBindCtx /*= nullptr*/ )
	{
		ASSERT_PTR( pShellPath );

		if ( AUTO_BIND_CTX == pBindCtx || AUTO_FOLDER_BIND_CTX == pBindCtx )
			if ( path::IsGuidPath( pShellPath )			// parsing GUID paths to PIDL require a bind context
				 || !fs::FileExist( pShellPath ) )		// inexistent file/directory paths require a proxy bind context
				return CreateFileSysBindContext( AUTO_FOLDER_BIND_CTX == pBindCtx ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL );
			else
				return nullptr;

		return pBindCtx;
	}
}


namespace shell
{
	// shell file info (via SHFILEINFO):

	SFGAOF GetFileSysAttributes( const TCHAR* pFilePath, UINT moreFlags /*= 0*/ )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ATTRIBUTES | moreFlags ) )
			return 0;

		return fileInfo.dwAttributes;
	}

	std::pair<CImageList*, int> GetFileSysImageIndex( const TCHAR* pFilePath, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );
		std::pair<CImageList*, int> imagePair( nullptr, -1 );

		if ( HIMAGELIST hSysImageList = (HIMAGELIST)::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX | iconFlags ) )
		{
			ENSURE( fileInfo.iIcon >= 0 );
			imagePair.first = CImageList::FromHandle( hSysImageList );
			imagePair.second = fileInfo.iIcon;
		}

		return imagePair;
	}

	HICON ExtractFileSysIcon( const TCHAR* pFilePath, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pFilePath, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ICON | iconFlags ) )
			return nullptr;

		return fileInfo.hIcon;
	}

	UINT GetSysIconSizeFlag( int iconDimension )
	{
		switch ( ui::LookupIconStdSize( iconDimension, SmallIcon ) )
		{
			case SmallIcon:
				return SHGFI_SMALLICON;
			case LargeIcon:
				return SHGFI_LARGEICON;
			default:
				return SHGFI_SHELLICONSIZE;
		}
	}
}


namespace shell
{
	std::tstring GetString( STRRET* pStrRet, PCUITEMID_CHILD pidl /*= nullptr*/ )
	{
		ASSERT_PTR( pStrRet );

		CComHeapPtr<TCHAR> text;
		if ( !HR_OK( ::StrRetToStr( pStrRet, pidl, &text ) ) )
			return str::GetEmpty();

		return text.m_pData;
	}

	// IShellFolder properties

	std::tstring GetFolderChildDisplayName( IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF flags /*= SHGDN_NORMAL*/ )
	{
		STRRET strRet;
		if ( !HR_OK( pFolder->GetDisplayNameOf( pidl, flags, &strRet ) ) )
			return str::GetEmpty();

		return GetString( &strRet, pidl );
	}

	std::tstring GetStringDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey )
	{
		CComVariant value;
		if ( HR_OK( pFolder->GetDetailsEx( pidl, &propKey, &value ) ) )
			if ( HR_OK( value.ChangeType( VT_BSTR ) ) )
				return V_BSTR( &value );

		return str::GetEmpty();
	}

	CTime GetDateTimeDetail( IShellFolder2* pFolder, PCUITEMID_CHILD pidl, const PROPERTYKEY& propKey )
	{
		CComVariant value;
		if ( HR_OK( pFolder->GetDetailsEx( pidl, &propKey, &value ) ) )
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

	std::tstring GetItemDisplayName( IShellItem* pItem, SIGDN nameType )
	{
		CComHeapPtr<wchar_t> name;
		if ( !HR_OK( pItem->GetDisplayName( nameType, &name ) ) )
			return str::GetEmpty();

		return name.m_pData;
	}


	std::tstring GetStringProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		CComHeapPtr<wchar_t> value;
		if ( !HR_OK( pItem->GetString( propKey, &value ) ) )
			return str::GetEmpty();

		return value.m_pData;
	}

	CTime GetDateTimeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		FILETIME fileTime;
		if ( !HR_OK( pItem->GetFileTime( propKey, &fileTime ) ) )
			return CTime();

		return CTime( fileTime );
	}

	DWORD GetFileAttributesProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		DWORD fileAttr;
		if ( !HR_OK( pItem->GetUInt32( propKey, &fileAttr ) ) )
			return UINT_MAX;

		return fileAttr;
	}

	ULONGLONG GetFileSizeProperty( IShellItem2* pItem, const PROPERTYKEY& propKey )
	{
		ULONGLONG fileSize;
		if ( !HR_OK( pItem->GetUInt64( propKey, &fileSize ) ) )
			return 0;

		return fileSize;
	}


	void QueryShellItemArrayPaths( OUT std::vector<shell::TPath>& rFilePaths, IShellItemArray* pSrcShellItemArray )
	{
		ASSERT_PTR( pSrcShellItemArray );

		DWORD itemCount;
		if ( HR_OK( pSrcShellItemArray->GetCount( &itemCount ) ) )
			for ( DWORD i = 0; i != itemCount; ++i )
			{
				CComPtr<IShellItem> pShellItem;
				if ( HR_OK( pSrcShellItemArray->GetItemAt( i, &pShellItem ) ) )
					rFilePaths.push_back( shell::GetItemShellPath( pShellItem ) );
			}
	}


#ifdef USE_UT

	const CFlagTags& GetTags_SFGAO_Flags( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FLAG_TAG( SFGAO_CANCOPY ) },
			{ FLAG_TAG( SFGAO_CANMOVE ) },
			{ FLAG_TAG( SFGAO_CANLINK ) },
			{ FLAG_TAG( SFGAO_STORAGE ) },
			{ FLAG_TAG( SFGAO_CANRENAME ) },
			{ FLAG_TAG( SFGAO_CANDELETE ) },
			{ FLAG_TAG( SFGAO_HASPROPSHEET ) },
			{ FLAG_TAG( SFGAO_DROPTARGET ) },
			{ FLAG_TAG( SFGAO_PLACEHOLDER ) },
			{ FLAG_TAG( SFGAO_SYSTEM ) },
			{ FLAG_TAG( SFGAO_ENCRYPTED ) },
			{ FLAG_TAG( SFGAO_ISSLOW ) },
			{ FLAG_TAG( SFGAO_GHOSTED ) },
			{ FLAG_TAG( SFGAO_LINK ) },
			{ FLAG_TAG( SFGAO_SHARE ) },
			{ FLAG_TAG( SFGAO_READONLY ) },
			{ FLAG_TAG( SFGAO_HIDDEN ) },
			{ FLAG_TAG( SFGAO_FILESYSANCESTOR ) },
			{ FLAG_TAG( SFGAO_FOLDER ) },
			{ FLAG_TAG( SFGAO_FILESYSTEM ) },
			{ FLAG_TAG( SFGAO_HASSUBFOLDER ) },
			{ FLAG_TAG( SFGAO_CONTENTSMASK ) },
			{ FLAG_TAG( SFGAO_VALIDATE ) },
			{ FLAG_TAG( SFGAO_REMOVABLE ) },
			{ FLAG_TAG( SFGAO_COMPRESSED ) },
			{ FLAG_TAG( SFGAO_BROWSABLE ) },
			{ FLAG_TAG( SFGAO_NONENUMERATED ) },
			{ FLAG_TAG( SFGAO_NEWCONTENT ) },
			{ FLAG_TAG( SFGAO_CANMONIKER ) },
			{ FLAG_TAG( SFGAO_HASSTORAGE ) },
			{ FLAG_TAG( SFGAO_STREAM ) },
			{ FLAG_TAG( SFGAO_STORAGEANCESTOR ) }
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}

#endif
}


namespace shell
{
	PIDLIST_ABSOLUTE MakePidl( const shell::TPath& shellPath, DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
	{
		// parses inexistent paths via shell::CreateFileSysBindContext() if fileAttribute = FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY
		return shell::pidl::ParseToPidlFileSys( shellPath.GetPtr(), fileAttribute  );
	}

	bool MakePidl( OUT CPidlAbsolute& rPidl, const shell::TPath& shellPath, DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
	{
		rPidl.Reset( MakePidl( shellPath, fileAttribute ) );
		return !rPidl.IsNull();
	}

	shell::TPath MakeShellPath( PCIDLIST_ABSOLUTE pidlAbs )
	{
		return shell::pidl::GetName( pidlAbs, SIGDN_DESKTOPABSOLUTEPARSING );		// compatible for both file-system and GUID paths
	}

	shell::TPath MakeShellPath( const CPidlAbsolute& pidl )
	{
		return shell::pidl::GetName( pidl, SIGDN_DESKTOPABSOLUTEPARSING );			// compatible for both file-system and GUID paths
	}


	namespace pidl
	{
		bool IsSpecialPidl( LPCITEMIDLIST pidl )
		{
			ASSERT_PTR( pidl );
			SHFILEINFO fileInfo;
			utl::ZeroStruct( &fileInfo );

			if ( ::SHGetFileInfo( (LPCWSTR)pidl, 0, &fileInfo, sizeof( fileInfo ), SHGFI_PIDL | SHGFI_ATTRIBUTES ) != 0 )
				if ( !HasFlag( fileInfo.dwAttributes, SFGAO_FILESYSTEM ) )		// PIDL refers to a virtual object OR a special shell namespace directory (e.g., Control Panel, Recycle Bin)
				{
					std::tstring parsingName = shell::pidl::GetName( reinterpret_cast<PCIDLIST_ABSOLUTE>( pidl ), SIGDN_DESKTOPABSOLUTEPARSING );

					// example: parsingName="::{26EE0668-A00A-44D7-9371-BEB064C98683}\0" for PIDL "Control Panel\\All Control Panel Items"
					if ( path::IsValidGuidPath( parsingName ) )
						return true;
				}

			return false;
		}


		SFGAOF GetPidlAttributes( PCIDLIST_ABSOLUTE pidl )
		{
			return shell::GetFileSysAttributes( (LPCTSTR)pidl, SHGFI_PIDL );
		}

		std::pair<CImageList*, int> GetPidlImageIndex( PCIDLIST_ABSOLUTE pidl, UINT iconFlags /*= SHGFI_SMALLICON*/ )
		{	// CImageList is a shared shell temporary object (no ownership)
			return shell::GetFileSysImageIndex( (LPCTSTR)pidl, SHGFI_PIDL | iconFlags );
		}

		HICON ExtractPidlIcon( PCIDLIST_ABSOLUTE pidl, UINT iconFlags /*= SHGFI_SMALLICON*/ )
		{	// returns a copy of the icon (caller must delete it)
			return shell::ExtractFileSysIcon( (LPCTSTR)pidl, SHGFI_PIDL | iconFlags );
		}


		PIDLIST_RELATIVE ParseToRelativePidl( IShellFolder* pParentFolder, const TCHAR* pRelShellPath, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			ASSERT_PTR( pParentFolder );
			ASSERT( !str::IsEmpty( pRelShellPath ) );

			CComPtr<IBindCtx> pBindContext = AutoFileSysBindContext( pRelShellPath, pBindCtx );

			PIDLIST_RELATIVE childItemPidl = nullptr;
			if ( !HR_OK( pParentFolder->ParseDisplayName( nullptr, pBindContext, const_cast<LPWSTR>( pRelShellPath ), nullptr, &childItemPidl, nullptr ) ) )
				return nullptr;

			return childItemPidl;			// allocated, caller must free
		}

		PITEMID_CHILD ParseToChildPidl( IShellFolder* pParentFolder, const TCHAR* pFilename, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			return reinterpret_cast<PITEMID_CHILD>( ParseToRelativePidl( pParentFolder, pFilename, pBindCtx ) );
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

			CComHeapPtr<TCHAR> pName;
			if ( HR_OK( ::SHGetNameFromIDList( pidl, nameType, &pName ) ) )
				name = pName.m_pData;

			return name;
		}

		shell::TPath GetShellPath( PCIDLIST_ABSOLUTE pidlAbsolute )
		{
			return shell::pidl::GetName( pidlAbsolute, SIGDN_DESKTOPABSOLUTEPARSING );		// compatible for both file-system and GUID paths
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


		// This performs the same function as ::SHSimpleIDListFromPath(), which is deprecated.
		//
		// pShellPath: could be either a file/folder path, or a GuidPath (display name formatted with SIGDN_DESKTOPABSOLUTEPARSING).
		//	Examples:
		//		"C:\dev\code\DevTools\TestDataUtl\images\Dice.png"
		//		"::{26EE0668-A00A-44D7-9371-BEB064C98683}\0\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
		//			- display name formatted with SIGDN_DESKTOPABSOLUTEPARSING,
		//			  corresponding to "Control Panel\All Control Panel Items\Region" (SIGDN_DESKTOPABSOLUTEEDITING).
		// pBindCtx: pass the bind context created with shell::CreateFileSysBindContext() to parse virtual path items (non-existent in the file system).
		//
		PIDLIST_ABSOLUTE ParseToPidl( const TCHAR* pShellPath, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			PIDLIST_ABSOLUTE pidlAbs = nullptr;
			shell::TPath expandedShellPath( pShellPath );

			if ( expandedShellPath.HasEnvironVar() )
				expandedShellPath.Expand();

			// This can parse non-existent files and Control Panel apps correctly.
			// For non-existent files/folders pass a pBindCtx created with CreateFileSysBindContext()
			if ( !HR_OK( ::SHParseDisplayName( expandedShellPath.GetPtr(), pBindCtx, &pidlAbs, 0, nullptr ) ) )
				return nullptr;

			return pidlAbs;
		}

		PIDLIST_ABSOLUTE ParseToPidlFileSys( const TCHAR* pShellPath, DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
		{	// parses inexistent paths via shell::CreateFileSysBindContext()
			return shell::pidl::ParseToPidl( pShellPath, fileAttribute != 0 ? shell::CreateFileSysBindContext( fileAttribute ) : nullptr );
		}

		PIDLIST_RELATIVE ParseToPidlRelative( const TCHAR* pRelShellPath, IShellFolder* pParentFolder, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			PIDLIST_RELATIVE pidlRel = nullptr;

			if ( nullptr == pParentFolder )
				return reinterpret_cast<PIDLIST_RELATIVE>( ParseToPidl( pRelShellPath, pBindCtx ) );

			if ( !HR_OK( pParentFolder->ParseDisplayName( nullptr, pBindCtx, const_cast<LPWSTR>( pRelShellPath ), nullptr, &pidlRel, nullptr ) ) )
				return nullptr;

			return pidlRel;
		}

		PIDLIST_ABSOLUTE ParseToPidlAbsolute( const TCHAR* pRelShellPath, IShellFolder* pParentFolder, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			CComHeapPtr<ITEMIDLIST_RELATIVE> pidlRel;

			pidlRel.Attach( ParseToPidlRelative( pRelShellPath, pParentFolder, pBindCtx ) );
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

		bool IsEqual( PCIDLIST_ABSOLUTE leftPidl, PCIDLIST_ABSOLUTE rightPidl )
		{
			if ( nullptr == leftPidl )
				return nullptr == rightPidl;
			else if ( nullptr == rightPidl )
				return nullptr == leftPidl;

			return ::ILIsEqual( leftPidl, rightPidl ) != FALSE;
		}

		pred::CompareResult Compare( PCUIDLIST_RELATIVE leftPidl, PCUIDLIST_RELATIVE rightPidl, IShellFolder* pParentFolder /*= GetDesktopFolder()*/ )
		{
			ASSERT_PTR( pParentFolder );

			HRESULT hResult = pParentFolder->CompareIDs( SHCIDS_CANONICALONLY, leftPidl, rightPidl );
			return pred::ToCompareResult( (short)HRESULT_CODE( hResult ) );
		}


		// common ancestor pidl:

		PIDLIST_ABSOLUTE GetCommonAncestor( PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2 )
		{	// find the common ancestor pidl of two absolute PIDLs
			if ( nullptr == pidl1 || nullptr == pidl2 )
				return nullptr;

			// start a copy of the full path of the first PIDL
			PIDLIST_ABSOLUTE ancestorPidl = ::ILCloneFull( pidl1 );

			// keep removing the last part of the 'common' path until it is a parent of pidl2
			while ( ancestorPidl != nullptr && ancestorPidl->mkid.cb != 0 )
				if ( ::ILIsParent( ancestorPidl, pidl2, FALSE ) )	// first parameter is a parent of the second?
					break;			// found a common ancestor
				else if ( !::ILRemoveLastID( ancestorPidl ) )
					break;			// reached the root Desktop (empty) pidl

			return ancestorPidl;	// special case: if they share no parent, they share the Desktop (empty PIDL)
		}

		PIDLIST_ABSOLUTE ExtractCommonAncestorPidl( PCIDLIST_ABSOLUTE_ARRAY pAbsPidls, size_t count )
		{	// find the common ancestor pidl of multiple absolute PIDLs; caller must delete it
			if ( 0 == count )
				return nullptr;

			ASSERT_PTR( pAbsPidls );
			ASSERT( 0 == std::count( pAbsPidls, pAbsPidls + count, nullptr ) );		// no PIDL should be null

			CPidlAbsolute ancestorPidl;

			// make ancestor the direct parent
			ancestorPidl.AssignCopy( pAbsPidls[0] );
			ancestorPidl.RemoveLast();					// make ancestor the parent

			for ( size_t i = 1; !ancestorPidl.IsNull() && i != count; ++i )
				ancestorPidl.Reset( GetCommonAncestor( ancestorPidl, pAbsPidls[i] ) );

			return ancestorPidl.Release();
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
