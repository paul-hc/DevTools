
#include "pch.h"
#include "ShellTypes.h"
#include "ShellPidl.h"
#include "utl/Algorithms_fwd.h"
#include "utl/AppTools.h"		// for IsRunningUnderWow64()
#include "utl/ResourcePool.h"	// for app::GetSharedResources
#include "utl/FlagTags.h"
#include "utl/IUnknownImpl.h"
#include "utl/StdHashValue.h"
#include <atlcomtime.h>			// for COleDateTime

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	bool ShellFolderExist( const TCHAR* pFolderShellPath )
	{
		if ( SFGAOF shAttribs = GetShellAttributes( pFolderShellPath ) )
		{
			//TRACE( _T( " - ShellFolderExist(): '%s'  SFGAOF={%s}\n" ), pFolderShellPath, GetTags_SFGAO_Flags().FormatKey( shAttribs ).c_str() );
			if ( HasFlag( shAttribs, SFGAO_FOLDER ) )		// file system directory or special shell namespace folder (e.g., Control Panel, Recycle Bin)
				return true;
		}

		return false;
	}

	bool ShellItemExist( const TCHAR* pShellPath )
	{
		if ( SFGAOF shAttribs = GetShellAttributes( pShellPath ) )
		{
			//TRACE( _T( " - ShellItemExist(): '%s'  SFGAOF={%s}\n" ), pShellPath, GetTags_SFGAO_Flags().FormatKey( shAttribs ).c_str() );
			return true;
		}

		return false;
	}

	DWORD GetShellFileAttributes( const TCHAR* pShellPath )
	{
		if ( SFGAOF shAttribs = GetShellAttributes( pShellPath ) )
			if ( HasFlag( shAttribs, SFGAO_FILESYSTEM ) )
				return ::GetFileAttributes( pShellPath );
			else
				return shell::ToFileAttributes( shAttribs );

		return INVALID_FILE_ATTRIBUTES;
	}


	bool ShellFolderExist( PCIDLIST_ABSOLUTE pidl )
	{
		if ( SFGAOF shAttribs = pidl::GetPidlShellAttributes( pidl ) )
			if ( HasFlag( shAttribs, SFGAO_FOLDER ) )		// file system directory or special shell namespace folder (e.g., Control Panel, Recycle Bin)
				return true;

		return false;
	}

	bool ShellItemExist( PCIDLIST_ABSOLUTE pidl )
	{
		if ( SFGAOF shAttribs = pidl::GetPidlShellAttributes( pidl ) )
			return true;

		return false;
	}

	DWORD GetShellFileAttributes( PCIDLIST_ABSOLUTE pidl )
	{
		DWORD fileAttributes = INVALID_FILE_ATTRIBUTES;

		if ( SFGAOF shAttribs = pidl::GetPidlShellAttributes( pidl ) )
			if ( HasFlag( shAttribs, SFGAO_FILESYSTEM ) )
				fileAttributes = ::GetFileAttributes( pidl::GetShellPath( pidl ).GetPtr() );
			else
				fileAttributes = shell::ToFileAttributes( shAttribs );

		return fileAttributes;
	}


	const CFlagTags& GetTags_SFGAO_Flags( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
		#ifdef _DEBUG
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
		#else
			NULL_TAG
		#endif
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}


	// CPatternParts implementation

	CPatternParts::CPatternParts( fs::SplitMode splitMode )
		: m_splitMode( splitMode )
		, m_result( InvalidPattern )
		, m_isFileSystem( false )
	{
	}

	ShellPatternResult CPatternParts::Split( const shell::TPatternPath& searchShellPath )
	{
		m_result = InvalidPattern;
		m_isFileSystem = false;

		bool hasWildcard = searchShellPath.HasWildcards();
		SFGAOF shAttribs = hasWildcard ? 0 : GetShellAttributes( searchShellPath.GetPtr() );

		if ( shAttribs != 0 )
		{
			if ( HasFlag( shAttribs, SFGAO_FOLDER ) )
			{	// pattern is directory/folder
				m_path = searchShellPath;
				m_wildSpec.clear();
				m_result = ValidFolder;
			}
			else if ( HasFlag( shAttribs, SFGAO_CANLINK ) )
			{	// pattern is file/applet
				if ( fs::SearchMode == m_splitMode )
				{
					m_path = searchShellPath.GetParentPath();
					m_wildSpec = searchShellPath.GetFilename();
				}
				else
				{
					ASSERT( fs::BrowseMode == m_splitMode );
					m_path = searchShellPath;
					m_wildSpec.clear();
				}
				m_result = ValidItem;
			}

			m_isFileSystem = HasFlag( shAttribs, SFGAO_FILESYSTEM );
		}
		else if ( hasWildcard )
		{
			m_path = searchShellPath.GetParentPath();
			m_wildSpec = searchShellPath.GetFilename();
		}
		else
		{
			m_path = searchShellPath;
			m_wildSpec.clear();
		}

		if ( InvalidPattern == m_result && 0 == shAttribs )
		{
			shAttribs = GetShellAttributes( m_path.GetPtr() );
			if ( shAttribs != 0 )
			{
				m_result = HasFlag( shAttribs, SFGAO_FOLDER ) ? ValidFolder : ValidItem;
				m_isFileSystem = HasFlag( shAttribs, SFGAO_FILESYSTEM );
			}
		}

		return m_result;
	}

	ShellPatternResult CPatternParts::SplitPattern( OUT shell::TPath* pPath, OUT OPTIONAL std::tstring* pWildSpec, const shell::TPatternPath& searchShellPath, fs::SplitMode splitMode )
	{
		CPatternParts parts( splitMode );

		parts.Split( searchShellPath );

		pPath->Swap( parts.m_path );
		if ( pWildSpec != nullptr )
			pWildSpec->swap( parts.m_wildSpec );

		return parts.m_result;
	}
}


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
		CPidlAbsolute folderPidl( pFolderShellPath );

		if ( !folderPidl.IsNull() )
			return MakeShellFolder( folderPidl );

		return nullptr;
	}

	CComPtr<IShellFolder> MakeShellFolder( PCIDLIST_ABSOLUTE folderPidl )
	{
		ASSERT_PTR( folderPidl );
		CComPtr<IShellFolder> pShellFolder;

		if ( !HR_OK( ::SHBindToObject( nullptr, folderPidl, nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) )
			return nullptr;

		return pShellFolder;
	}

	CComPtr<IShellFolder> MakeShellFolder_DesktopRel( const TCHAR* pFolderShellPath )
	{
		CComPtr<IShellFolder> pDirFolder;

		if ( CComPtr<IShellFolder> pDesktopFolder = GetDesktopFolder() )
		{
			CComHeapPtr<ITEMIDLIST_RELATIVE> folderPidlRel;

			folderPidlRel.Attach( pidl::ParseToRelativePidl( pDesktopFolder, pFolderShellPath ) );
			if ( folderPidlRel.m_pData != nullptr )
				if ( HR_OK( pDesktopFolder->BindToObject( folderPidlRel, nullptr, IID_PPV_ARGS( &pDirFolder ) ) ) )
					return pDirFolder;
		}

		return nullptr;
	}

	CComPtr<IShellFolder2> ToShellFolder( IUnknown* pShellItf )
	{
		ASSERT_PTR( pShellItf );

		CComPtr<IShellFolder2> pShellFolder;
		CComHeapPtr<ITEMIDLIST_ABSOLUTE> folderPidlAbs;

		if ( HR_OK( ::SHGetIDListFromObject( pShellItf, &folderPidlAbs ) ) )
			if ( HR_OK( ::SHBindToObject( nullptr, folderPidlAbs, nullptr, IID_PPV_ARGS( &pShellFolder ) ) ) )		// bind to the IShellFolder2
				return pShellFolder;

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

	SFGAOF GetItemAttributes( IShellFolder* pFolder, PCUITEMID_CHILD childPidl, SFGAOF desiredAttrMask )
	{
		ASSERT( pFolder != nullptr && childPidl != nullptr );

		SFGAOF shAttribs = desiredAttrMask;

		if ( !HR_OK( pFolder->GetAttributesOf( 1, (PCUITEMID_CHILD_ARRAY)&childPidl, &shAttribs ) ) )		// shAttribs: both IN and OUT
			return 0;

		return shAttribs;
	}


	CComPtr<IShellItem> MakeShellItem( const TCHAR* pShellPath )
	{
		ASSERT_PTR( pShellPath );

		CComPtr<IShellItem> pShellItem;

		if ( !HR_OK( ::SHCreateItemFromParsingName( pShellPath, CShared::AutoGuidPathBindCtx( pShellPath ), IID_PPV_ARGS( &pShellItem ) ) ) )
			return nullptr;

		return pShellItem;
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
	bool IsRunningUnderWow64( void )
	{
		return CAppTools::Instance()->IsRunningUnderWow64();
	}


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
		IFACEMETHODIMP SetFindData( const WIN32_FIND_DATA* pFindData ) { m_findData = *pFindData; return S_OK; }
		IFACEMETHODIMP GetFindData( OUT WIN32_FIND_DATA* pDestFindData ) { *pDestFindData = m_findData; return S_OK; }
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


	// CShared implementation

	IBindCtx* CShared::GetFileBindCtx( void )
	{
		static CComPtr<IBindCtx> s_pFileBindCtx;
		if ( nullptr == s_pFileBindCtx )
		{	// lazy init
			s_pFileBindCtx = CreateFileSysBindContext( FILE_ATTRIBUTE_NORMAL );
			app::GetSharedResources().AddComPtr( s_pFileBindCtx );			// will release the CComPtr singleton in ExitInstance()
		}
		return s_pFileBindCtx;
	}

	IBindCtx* CShared::GetDirectoryBindCtx( void )
	{
		static CComPtr<IBindCtx> s_pDirectoryBindCtx;
		if ( nullptr == s_pDirectoryBindCtx )
		{	// lazy init
			s_pDirectoryBindCtx = CreateFileSysBindContext( FILE_ATTRIBUTE_DIRECTORY );
			app::GetSharedResources().AddComPtr( s_pDirectoryBindCtx );		// will release the CComPtr singleton in ExitInstance()
		}
		return s_pDirectoryBindCtx;
	}

	/*
		File System Redirection (WOW64):
		When a 32-bit application runs on a 64-bit OS, Windows uses a File System Redirector to map calls intended for %windir%\System32 to %windir%\SysWOW64.

		Control Panel Applets: many applets (like Region) are implemented via .cpl or .dll files located in the System32 directory.
		The Failure: When SHCreateItemFromParsingName() is called in 32-bit mode, the Shell attempts to resolve the underlying file.
		Because of redirection, it looks in SysWOW64.
		If the specific 64-bit applet's parsing logic isn't mirrored or correctly registered in the 32-bit hive/folder, the parsing fails.

		By passing an IBindCtx with STR_FILE_SYS_BIND_DATA, we are providing the Shell with pre-verified file metadata (via a WIN32_FIND_DATA structure).
		This tells the Shell: "I already know this item exists; here are its attributes."
		This allows the parser to bypass the standard disk-probing phase (which is where the redirection/missing-file error occurs),
		and proceed directly to creating the IShellItem based on the provided data.
	*/
	IBindCtx* CShared::GetWow64BindCtx( void )
	{	// shared bind context to be used for parsing GUID paths when 32 bit process runs under WOW64 redirector
		if ( IsRunningUnderWow64() )
			return GetFileBindCtx();

		return nullptr;
	}

	IBindCtx* CShared::GetFileSysBindCtx( DWORD fileAttribute )
	{
		if ( FILE_ATTRIBUTE_NORMAL == fileAttribute )
			return GetFileBindCtx();

		if ( FILE_ATTRIBUTE_DIRECTORY == fileAttribute )
			return GetDirectoryBindCtx();

		return nullptr;
	}

	IBindCtx* CShared::AutoProxyFilePathBindCtx( const TCHAR* pShellPath )
	{
		ASSERT_PTR( pShellPath );

		if ( IBindCtx* pWow64BindCtx = GetWow64BindCtx() )
			return pWow64BindCtx;

		if ( !IsGuidPath( pShellPath ) && !fs::FileExist( pShellPath ) )		// inexistent file/directory paths require a proxy bind context
			return GetFileBindCtx();

		return nullptr;
	}

	IBindCtx* CShared::AutoGuidPathBindCtx( const TCHAR* pShellPath )
	{
		if ( IsRunningUnderWow64() )
			if ( IsGuidPath( pShellPath ) )
				return GetFileBindCtx();		// STR_FILE_SYS_BIND_DATA bind context is required to workaround WOW64 redirector parsing errors (missing WOW64 registry keys, etc)

		return nullptr;
	}
}


namespace shell
{
	// shell file info (via SHFILEINFO):

	DWORD ToFileAttributes( SFGAOF shAttribs )
	{
		if ( 0 == shAttribs )
			return INVALID_FILE_ATTRIBUTES;

		DWORD fileAttributes = HasFlag( shAttribs, SFGAO_FOLDER ) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

		SetFlag( fileAttributes, FILE_ATTRIBUTE_READONLY, HasFlag( shAttribs, SFGAO_READONLY ) );
		SetFlag( fileAttributes, FILE_ATTRIBUTE_HIDDEN, HasFlag( shAttribs, SFGAO_HIDDEN ) );
		SetFlag( fileAttributes, FILE_ATTRIBUTE_SYSTEM, HasFlag( shAttribs, SFGAO_SYSTEM ) );

		return fileAttributes;
	}

	SFGAOF GetShellAttributes( const TCHAR* pShellPath )
	{
		if ( path::HasEnvironVar( pShellPath ) )
			return GetShellAttributes( shell::TPath::Expand( pShellPath ).GetPtr() );		// recurse with expaned path

		if ( IsGuidPath( pShellPath ) )
		{
			CPidlAbsolute pidl( pShellPath );
			return !pidl.IsNull() ? pidl.GetShellAttributes() : 0;
		}

		return GetRawShellAttributes( pShellPath );
	}

	SFGAOF GetRawShellAttributes( const TCHAR* pPathOrPidl, UINT moreFlags /*= 0*/ )
	{
		ASSERT_PTR( pPathOrPidl );

		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pPathOrPidl, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ATTRIBUTES | moreFlags ) )
			return 0;

		return static_cast<SFGAOF>( fileInfo.dwAttributes );
	}


	std::pair<CImageList*, int> GetShellSysImageIndex( const TCHAR* pShellPath, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		if ( path::HasEnvironVar( pShellPath ) )
			return GetShellSysImageIndex( shell::TPath::Expand( pShellPath ).GetPtr(), iconFlags );		// recurse with expaned path

		if ( IsGuidPath( pShellPath ) )
		{
			CPidlAbsolute pidl( pShellPath );
			return !pidl.IsNull() ? pidl::GetPidlImageIndex( pidl, iconFlags ) : std::pair<CImageList*, int>( nullptr, -1 );
		}

		return GetFileSysImageIndex( pShellPath, iconFlags );
	}

	std::pair<CImageList*, int> GetFileSysImageIndex( const TCHAR* pPathOrPidl, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		ASSERT_PTR( pPathOrPidl );

		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );
		std::pair<CImageList*, int> imagePair( nullptr, -1 );

		if ( HIMAGELIST hSysImageList = (HIMAGELIST)::SHGetFileInfo( pPathOrPidl, 0, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX | iconFlags ) )
		{
			ENSURE( fileInfo.iIcon >= 0 );
			imagePair.first = CImageList::FromHandle( hSysImageList );
			imagePair.second = fileInfo.iIcon;
		}

		return imagePair;
	}

	HICON ExtractShellIcon( const TCHAR* pShellPath, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		if ( path::HasEnvironVar( pShellPath ) )
			return ExtractShellIcon( shell::TPath::Expand( pShellPath ).GetPtr(), iconFlags );		// recurse with expaned path

		if ( IsGuidPath( pShellPath ) )
		{
			CPidlAbsolute pidl( pShellPath );
			return !pidl.IsNull() ? pidl::ExtractPidlIcon( pidl, iconFlags ) : nullptr;
		}

		return ExtractFileSysIcon( pShellPath, iconFlags );
	}

	HICON ExtractFileSysIcon( const TCHAR* pPathOrPidl, UINT iconFlags /*= SHGFI_SMALLICON*/ )
	{
		ASSERT_PTR( pPathOrPidl );

		SHFILEINFO fileInfo;
		utl::ZeroStruct( &fileInfo );

		if ( 0 == ::SHGetFileInfo( pPathOrPidl, 0, &fileInfo, sizeof( fileInfo ), SHGFI_ICON | iconFlags ) )
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

	bool GetString( OUT std::tstring& rText, STRRET* pStrRet, PCUITEMID_CHILD pidl /*= nullptr*/ )
	{
		ASSERT_PTR( pStrRet );

		CComHeapPtr<TCHAR> text;
		if ( !HR_OK( ::StrRetToStr( pStrRet, pidl, &text ) ) )
		{
			rText.clear();
			return false;
		}

		rText = text.m_pData;
		return true;
	}


	// IShellFolder properties

	std::tstring GetFolderChildName( IShellFolder* pParentFolder, PCUITEMID_CHILD pidl, SHGDNF flags /*= SHGDN_NORMAL*/ )
	{
		STRRET strRet;
		if ( !HR_OK( pParentFolder->GetDisplayNameOf( pidl, flags, &strRet ) ) )
			return str::GetEmpty();

		return GetString( &strRet, pidl );
	}

	bool GetFolderChildName( OUT std::tstring& rDisplayName, IShellFolder* pParentFolder, PCUITEMID_CHILD pidl, SHGDNF flags )
	{
		STRRET strRet;
		if ( !HR_OK( pParentFolder->GetDisplayNameOf( pidl, flags, &strRet ) ) )
		{
			rDisplayName.clear();
			return false;
		}

		return GetString( rDisplayName, &strRet, pidl );
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


	// IShellItemArray utils

	void QueryShellItemArrayPaths( OUT std::vector<shell::TPath>& rShellPaths, IN IShellItemArray* pItemArray )
	{
		ASSERT_PTR( pItemArray );
		rShellPaths.clear();

		DWORD itemCount;
		if ( HR_OK( pItemArray->GetCount( &itemCount ) ) )
			for ( DWORD i = 0; i != itemCount; ++i )
			{
				CComPtr<IShellItem> pItem;

				if ( HR_OK( pItemArray->GetItemAt( i, &pItem ) ) )
					rShellPaths.push_back( shell::GetItemShellPath( pItem ) );
			}
	}

	void QueryShellItemArrayEnumPaths( OUT std::vector<shell::TPath>& rShellPaths, IN IShellItemArray* pItemArray )
	{
		rShellPaths.clear();

		if ( CComPtr<IEnumShellItems> pEnumItems = GetEnumShellItemArray( pItemArray ) )
			QueryEnumShellItemsPaths( rShellPaths, pEnumItems );
	}

	bool GetFirstItemShellPath( OUT shell::TPath& rShellPath, IShellItemArray* pItemArray )
	{
		ASSERT_PTR( pItemArray );

		DWORD itemCount = 0;
		if ( HR_OK( pItemArray->GetCount( &itemCount ) ) && itemCount != 0 )
		{
			CComPtr<IShellItem> pFirstItem;
			if ( HR_OK( pItemArray->GetItemAt( 0, &pFirstItem ) ) )
			{
				rShellPath = shell::GetItemShellPath( pFirstItem );
				return true;
			}
		}
		return false;
	}

	CComPtr<IEnumShellItems> GetEnumShellItemArray( IShellItemArray* pItemArray )
	{
		ASSERT_PTR( pItemArray );

		CComPtr<IEnumShellItems> pEnumItems;

		if ( !HR_OK( pItemArray->EnumItems( &pEnumItems ) ) )
			return nullptr;

		return pEnumItems;
	}

	void QueryEnumShellItemsPaths( OUT std::vector<shell::TPath>& rShellPaths, IEnumShellItems* pEnumItems )
	{
		ASSERT_PTR( pEnumItems );
		rShellPaths.clear();

		ULONG fetchedCount;
		for ( CComPtr<IShellItem> pItem; S_OK == pEnumItems->Next( 1, &pItem, &fetchedCount ) && fetchedCount != 0; pItem = nullptr )
			rShellPaths.push_back( shell::GetItemShellPath( pItem ) );
	}
}


namespace shell
{
	PIDLIST_ABSOLUTE MakePidl( const shell::TPath& shellPath, DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
	{
		// parses inexistent paths if fileAttribute = FILE_ATTRIBUTE_NORMAL or FILE_ATTRIBUTE_DIRECTORY
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
		bool IsSpecialPidl( PCIDLIST_ABSOLUTE pidl )
		{
			ASSERT_PTR( pidl );

			if ( SFGAOF shAttribs = GetPidlShellAttributes( pidl ) )
				if ( !HasFlag( shAttribs, SFGAO_FILESYSTEM ) && HasFlag( shAttribs, SFGAO_FOLDER | SFGAO_CANLINK ) )
				{	// PIDL refers to a special shell namespace folder (e.g., Control Panel, Recycle Bin), or a Control Panel applet.
					//	SFGAO_FOLDER:	"::{26EE0668-A00A-44D7-9371-BEB064C98683}\0" for PIDL to folder "Control Panel\All Control Panel Items"
					//	SFGAO_CANLINK:	"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}" for Region applet in Control Panel
					ASSERT( IsGuidPath( pidl::GetShellPath( pidl ) ) );
					return true;
				}

			return false;
		}


		SFGAOF GetPidlShellAttributes( PCIDLIST_ABSOLUTE pidl )
		{
			return shell::GetRawShellAttributes( (LPCTSTR)pidl, SHGFI_PIDL );
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

			PIDLIST_RELATIVE childItemPidl = nullptr;
			if ( !HR_OK( pParentFolder->ParseDisplayName( nullptr, pBindCtx != nullptr ? pBindCtx : CShared::AutoProxyFilePathBindCtx( pRelShellPath ),
														  const_cast<LPWSTR>( pRelShellPath ), nullptr, &childItemPidl, nullptr ) ) )
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

			if ( pidlAbsolute != nullptr )
			{
				TCHAR buffer[ MAX_PATH * 2 ];

				if ( ::SHGetPathFromIDListEx( pidlAbsolute, ARRAY_SPAN( buffer ), optFlags ) )
					fullPath.Set( buffer );
			}
			else
				ASSERT( false );

			return fullPath;
		}


		PIDLIST_ABSOLUTE CreateFromPath( const TCHAR* pExistingShellPath )
		{
			if ( path::HasEnvironVar( pExistingShellPath ) )
				return CreateFromPath( shell::TPath::Expand( pExistingShellPath ).GetPtr() );	// recurse with expaned path

			PIDLIST_ABSOLUTE pidlAbs = ::ILCreateFromPath( pExistingShellPath );

			// Handle for uniform behaviour on 32/64 builds:
			//	64-bit build: succeeds for any existing file-system path or GUID path.
			//	32-bit build: succeeds for any existing file-system path, but it fails for GUID path due to WOW64 redirector errors.

		#if !defined(_WIN64)
			if ( nullptr == pidlAbs )		// possibly a WOW64 redirector error?
				pidlAbs = ParseToPidl( pExistingShellPath, CShared::AutoGuidPathBindCtx( pExistingShellPath ) );		// use custom bind context only for GUID paths
		#endif

			return pidlAbs;
		}

		// Similar with CreateFromPath(), but also works for non-existen file system paths.
		// This performs the same function as ::SHSimpleIDListFromPath(), which is deprecated.
		//
		// pShellPath: could be either a file/folder path, or a GuidPath (display name formatted with SIGDN_DESKTOPABSOLUTEPARSING).
		//	Examples:
		//		"C:\dev\code\DevTools\TestDataUtl\images\Dice.png"
		//		"::{26EE0668-A00A-44D7-9371-BEB064C98683}\0\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
		//			- display name formatted with SIGDN_DESKTOPABSOLUTEPARSING,
		//			  corresponding to "Control Panel\All Control Panel Items\Region" (SIGDN_DESKTOPABSOLUTEEDITING).
		// pBindCtx: pass the bind context shell::CShared::GetFileBindCtx() to parse inexistent file system paths.
		//
		PIDLIST_ABSOLUTE ParseToPidl( const TCHAR* pShellPath, IBindCtx* pBindCtx /*= nullptr*/ )
		{
			if ( path::HasEnvironVar( pShellPath ) )
				return ParseToPidl( shell::TPath::Expand( pShellPath ).GetPtr(), pBindCtx );	// recurse with expaned path

			PIDLIST_ABSOLUTE pidlAbs = nullptr;

			// This can parse non-existent files and Control Panel apps correctly with a proper pBindCtx.
			if ( !HR_OK( ::SHParseDisplayName( pShellPath, pBindCtx, &pidlAbs, 0, nullptr ) ) )
				return nullptr;

			return pidlAbs;
		}

		PIDLIST_ABSOLUTE ParseToPidlFileSys( const TCHAR* pShellPath, DWORD fileAttribute /*= FILE_ATTRIBUTE_NORMAL*/ )
		{	// parses inexistent paths via shell::CreateFileSysBindContext()
			if ( 0 == fileAttribute )
				if ( IsGuidPath( pShellPath )				// parsing GUID paths to PIDL require a bind context
					 || !fs::FileExist( pShellPath ) )		// inexistent file/directory paths require a proxy bind context
					fileAttribute = FILE_ATTRIBUTE_NORMAL;

			return shell::pidl::ParseToPidl( pShellPath, CShared::GetFileSysBindCtx( fileAttribute ) );
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
