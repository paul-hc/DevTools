
#include "pch.h"
#include "Shortcut.h"
#include "ComUtils.h"
#include "utl/Algorithms_fwd.h"
#include "utl/EnumTags.h"
#include "utl/FlagTags.h"
#include "utl/FmtUtils.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/SerializeStdTypes.h"
#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	bool IsValidShortcutFile( const TCHAR* pLnkPath )
	{
		return ::HasFlag( shell::GetFileSysAttributes( pLnkPath ), SFGAO_LINK );
	}

	CComPtr<IShellLink> CreateShellLink( void )
	{
		CComPtr<IShellLink> pShellLink;

		if ( HR_OK( pShellLink.CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER ) ) )	// create a shell link object
			return pShellLink;

		return nullptr;
	}

	CComPtr<IShellLink> LoadLinkFromFile( const TCHAR* pLnkPath )
	{
		if ( IsValidShortcutFile( pLnkPath ) )
			if ( CComPtr<IShellLink> pShellLink = CreateShellLink() )
				if ( CComQIPtr<IPersistFile> pPersistFile = pShellLink.p )
					if ( HR_OK( pPersistFile->Load( pLnkPath, STGM_READ ) ) )								// load it from .lnk file
						return pShellLink;

		return nullptr;
	}

	bool SaveLinkToFile( const TCHAR* pDestLnkPath, IShellLink* pShellLink )
	{
		ASSERT_PTR( pDestLnkPath );
		ASSERT_PTR( pShellLink );

		if ( CComQIPtr<IPersistFile> pPersistFile = pShellLink )
			if ( HR_OK( pPersistFile->Save( pDestLnkPath, TRUE ) ) )			// save the link to .lnk file
				return true;

		return false;
	}


	fs::CPath GetLinkTargetPath( IShellLink* pShellLink, DWORD pFlags /*= 0*/, OUT WIN32_FIND_DATA* pFindData /*= nullptr*/ )
	{
		TCHAR targetPath[ MAX_PATH ];

		pShellLink->GetPath( ARRAY_SPAN( targetPath ), pFindData, pFlags );
		return fs::CPath( targetPath );
	}

	fs::CPath GetShortcutTargetPath( const TCHAR* pLnkPath, OUT std::tstring* pArguments /*= nullptr*/, OUT WIN32_FIND_DATA* pFindData /*= nullptr*/ )
	{
		fs::CPath targetPath;

		if ( CComPtr<IShellLink> pShellLink = LoadLinkFromFile( pLnkPath ) )
		{
			TCHAR buffer[ INFOTIPSIZE ];

			if ( HR_OK( pShellLink->GetPath( ARRAY_SPAN( buffer ), pFindData, 0 ) ) )
			{
				targetPath.Set( buffer );

				if ( pArguments != nullptr )
					if ( HR_OK( pShellLink->GetArguments( ARRAY_SPAN( buffer ) ) ) )
						*pArguments = buffer;
			}
		}

		return targetPath;
	}


	bool ResolveLink( OUT fs::CPath& rTargetPath, IShellLink* pShellLink, CWnd* pWnd /*= nullptr*/, DWORD rFlags /*= 0*/ )
	{
		if ( nullptr == pWnd )
			pWnd = ::AfxGetMainWnd();

		if ( pWnd != nullptr )
			rFlags |= SLR_NO_UI;

		if ( HR_OK( pShellLink->Resolve( pWnd->GetSafeHwnd(), rFlags ) ) )		// resolve the link; this may post UI to find the link
		{
			TCHAR destPath[ MAX_PATH ];

			pShellLink->GetPath( ARRAY_SPAN( destPath ), nullptr, 0 );
			rTargetPath.Set( destPath );
			return true;
		}

		return false;
	}

	bool ResolveShortcut( OUT fs::CPath& rTargetPath, const TCHAR* pLnkPath, CWnd* pWnd /*= nullptr*/ )
	{
		CComPtr<IShellLink> pShellLink = LoadLinkFromFile( pLnkPath );

		return pShellLink != nullptr && ResolveLink( rTargetPath, pShellLink, pWnd );
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

	const CFlagTags& GetTags_ShellLinkDataFlags( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FLAG_TAG( SLDF_HAS_ID_LIST ) },			// Shell link saved with ID list
			{ FLAG_TAG( SLDF_HAS_LINK_INFO ) },			// Shell link saved with LinkInfo
			{ FLAG_TAG( SLDF_HAS_NAME ) },
			{ FLAG_TAG( SLDF_HAS_RELPATH ) },
			{ FLAG_TAG( SLDF_HAS_WORKINGDIR ) },
			{ FLAG_TAG( SLDF_HAS_ARGS ) },
			{ FLAG_TAG( SLDF_HAS_ICONLOCATION ) },
			{ FLAG_TAG( SLDF_UNICODE ) },				// the strings are unicode
			{ FLAG_TAG( SLDF_FORCE_NO_LINKINFO ) },		// disable LINKINFO tracking information (used to track network drives and compute UNC paths if one exists)
			{ FLAG_TAG( SLDF_HAS_EXP_SZ ) },			// the link contains expandable env strings
			{ FLAG_TAG( SLDF_RUN_IN_SEPARATE ) },		// "Run in separate memory space": Run the 16-bit target exe in a separate VDM/WOW
		#if (NTDDI_VERSION < NTDDI_VISTA)
			{ FLAG_TAG( SLDF_HAS_LOGO3ID ) },			// not used anymore
		#endif
			{ FLAG_TAG( SLDF_HAS_DARWINID ) },			// MSI (Darwin) link that can be installed on demand
			{ FLAG_TAG( SLDF_RUNAS_USER ) },			// "Run as Administrator": Run target as a different user
			{ FLAG_TAG( SLDF_HAS_EXP_ICON_SZ ) },		// contains expandable env string for icon path
		#if (NTDDI_VERSION >= NTDDI_WINXP)
			{ FLAG_TAG( SLDF_NO_PIDL_ALIAS ) },         // disable IDList alias mapping when parsing the IDList from the path
			{ FLAG_TAG( SLDF_FORCE_UNCNAME ) },         // make GetPath() prefer the UNC name to the local name
			{ FLAG_TAG( SLDF_RUN_WITH_SHIMLAYER ) },    // activate target of this link with shim layer active
		#if (NTDDI_VERSION >= NTDDI_VISTA)
			{ FLAG_TAG( SLDF_FORCE_NO_LINKTRACK ) },			// disable ObjectID tracking information
			{ FLAG_TAG( SLDF_ENABLE_TARGET_METADATA ) },		// enable caching of target metadata into link
			{ FLAG_TAG( SLDF_DISABLE_LINK_PATH_TRACKING ) },	// disable EXP_SZ_LINK_SIG tracking
			{ FLAG_TAG( SLDF_DISABLE_KNOWNFOLDER_RELATIVE_TRACKING ) },	// disable KnownFolder tracking information (EXP_KNOWN_FOLDER)
		#if (NTDDI_VERSION >= NTDDI_WIN7)
			{ FLAG_TAG( SLDF_NO_KF_ALIAS ) },				// disable Known Folder alias mapping when loading the IDList during deserialization
			{ FLAG_TAG( SLDF_ALLOW_LINK_TO_LINK ) },		// allows this link to point to another shell link - must only be used when it is not possible to create cycles
			{ FLAG_TAG( SLDF_UNALIAS_ON_SAVE ) },			// unalias the IDList when saving
			{ FLAG_TAG( SLDF_PREFER_ENVIRONMENT_PATH ) },	// the IDList is not persisted, instead it is recalculated from the path with environmental variables at load time
			{ FLAG_TAG( SLDF_KEEP_LOCAL_IDLIST_FOR_UNC_TARGET ) },	// if target is a UNC location on a local machine, keep the local target in addition to the remote one
		#if (NTDDI_VERSION >= NTDDI_WIN8)
			{ FLAG_TAG( SLDF_PERSIST_VOLUME_ID_RELATIVE ) },// persist target idlist in its volume ID-relative form to avoid dependency on drive letters
		#endif
		#endif
		#endif
			{ FLAG_TAG( (DWORD)SLDF_RESERVED ) }			// Reserved-- so we can use the low word as an index value in the future
		#endif
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}

#endif //USE_UT


	// CShortcut implementation

	CShortcut::CShortcut( IN IShellLink* pShellLink )
	{
		ASSERT_PTR( pShellLink );
		Reset( pShellLink );
	}

	void CShortcut::Reset( IN IShellLink* pShellLink /*= nullptr*/ )
	{
		*this = CShortcut();
		if ( nullptr == pShellLink )
			return;

		if ( CComQIPtr<IShellLinkDataList> pDataList = pShellLink )
			if ( !HR_OK( pDataList->GetFlags( &m_linkDataFlags ) ) )
				m_linkDataFlags = 0;

		int bufferSize = INFOTIPSIZE;			// 1024: max size for GetArguments()
		std::vector<TCHAR> buffer( bufferSize );
		TCHAR* pBuffer = utl::Data( buffer );

		if ( HR_OK( pShellLink->GetPath( pBuffer, bufferSize, nullptr, SLGP_RAWPATH ) ) )		// possible to err with E_NOTIMPL, or INPLACE_S_TRUNCATED?!
			m_targetPath.Set( pBuffer );
		else if ( HR_OK( pShellLink->GetPath( pBuffer, bufferSize, nullptr, 0 ) ) )
			m_targetPath.Set( pBuffer );

		HR_AUDIT( pShellLink->GetIDList( &m_targetPidl.Ref() ) );

		if ( HR_OK( pShellLink->GetWorkingDirectory( pBuffer, bufferSize ) ) )
			m_workDirPath.Set( pBuffer );

		if ( HR_OK( pShellLink->GetArguments( pBuffer, bufferSize ) ) )
			m_arguments = pBuffer;

		if ( HR_OK( pShellLink->GetDescription( pBuffer, bufferSize ) ) )
			m_description = pBuffer;

		if ( HR_OK( pShellLink->GetIconLocation( pBuffer, bufferSize, &m_iconLocation.m_index ) ) )
			m_iconLocation.m_path.Set( pBuffer );

		if ( !HR_OK( pShellLink->GetHotkey( &m_hotKey ) ) )
			m_hotKey = 0;

		if ( !HR_OK( pShellLink->GetShowCmd( &m_showCmd ) ) )
			m_showCmd = 0;
	}

	bool CShortcut::WriteToLink( OUT IShellLink* pDestShellLink ) const
	{
		bool succeeded = true;

		if ( HasFlag( m_changedFields, LinkDataFlags ) )
			if ( CComQIPtr<IShellLinkDataList> pDataList = pDestShellLink )
				succeeded &= HR_OK( pDataList->SetFlags( m_linkDataFlags ) );

		if ( HasFlag( m_changedFields, TargetPath ) )
			succeeded &= HR_OK( pDestShellLink->SetPath( m_targetPath.GetPtr() ) );

		if ( HasFlag( m_changedFields, TargetPidl ) )
			succeeded &= HR_OK( pDestShellLink->SetIDList( m_targetPidl.Get() ) );

		if ( HasFlag( m_changedFields, WorkDirPath ) )
			succeeded &= HR_OK( pDestShellLink->SetWorkingDirectory( m_workDirPath.GetPtr() ) );

		if ( HasFlag( m_changedFields, Arguments ) )
			succeeded &= HR_OK( pDestShellLink->SetArguments( m_arguments.c_str() ) );

		if ( HasFlag( m_changedFields, Description ) )
			succeeded &= HR_OK( pDestShellLink->SetDescription( m_description.c_str() ) );

		if ( HasFlag( m_changedFields, IconLocation ) )
			succeeded &= HR_OK( pDestShellLink->SetIconLocation( m_iconLocation.m_path.GetPtr(), m_iconLocation.m_index ) );

		if ( HasFlag( m_changedFields, HotKey ) )
			succeeded &= HR_OK( pDestShellLink->SetHotkey( m_hotKey ) );

		if ( HasFlag( m_changedFields, ShowCmd ) )
			succeeded &= HR_OK( pDestShellLink->SetShowCmd( m_showCmd ) );

		return succeeded;
	}

	bool CShortcut::IsTargetValid( void ) const
	{
		if ( IsTargetEmpty() )
			return false;

		if ( !m_targetPath.IsEmpty() )
			return env::HasAnyVariable( m_targetPath.Get() ) ? m_targetPath.GetExpanded().FileExist() : m_targetPath.FileExist();

		return true;
	}

	bool CShortcut::operator==( const CShortcut& right ) const
	{
		return
			m_targetPath.Get() == right.m_targetPath.Get() &&		// case-sensitive compare
			m_targetPidl == right.m_targetPidl &&
			m_workDirPath.Get() == right.m_workDirPath.Get() &&		// case-sensitive compare
			m_arguments == right.m_arguments &&
			m_description == right.m_description &&
			m_hotKey == right.m_hotKey &&
			m_iconLocation == right.m_iconLocation &&
			m_showCmd == right.m_showCmd &&
			m_linkDataFlags == right.m_linkDataFlags;
	}

	bool CShortcut::SetTargetPidl( PIDLIST_ABSOLUTE pidl, utl::Ownership ownership /*= utl::MOVE*/ )
	{
		if ( utl::COPY == ownership )
			if ( shell::pidl::IsEqual( pidl, m_targetPidl.Get() ) )
				return false;

		m_targetPidl.Set( pidl, ownership );
		SetFlag( m_changedFields, TargetPidl );
		return true;
	}

	bool CShortcut::ModifyLinkDataFlags( DWORD clearFlags, DWORD setFlags )
	{
		if ( !::ModifyFlags( m_linkDataFlags, clearFlags, setFlags ) )
			return false;

		::SetFlag( m_linkDataFlags, LinkDataFlags );
		return true;
	}

	void CShortcut::Save( OUT CArchive& archive ) const
	{
		archive
			<< m_targetPath << m_targetPidl << m_workDirPath << m_arguments << m_description
			<< m_iconLocation.m_path << m_iconLocation.m_index << m_hotKey << m_showCmd << m_linkDataFlags;
	}

	void CShortcut::Load( IN CArchive& archive )
	{
		archive
			>> m_targetPath >> m_targetPidl >> m_workDirPath >> m_arguments >> m_description
			>> m_iconLocation.m_path >> m_iconLocation.m_index >> m_hotKey >> m_showCmd >> m_linkDataFlags;
	}

	CShortcut::TFields CShortcut::GetDiffFields( const CShortcut& right ) const
	{
		TFields diffFields = 0;

		SetFlag( diffFields, TargetPath, m_targetPath != right.m_targetPath );
		SetFlag( diffFields, TargetPidl, m_targetPidl != right.m_targetPidl );
		SetFlag( diffFields, WorkDirPath, m_workDirPath != right.m_workDirPath );
		SetFlag( diffFields, Arguments, m_arguments != right.m_arguments );
		SetFlag( diffFields, Description, m_description != right.m_description );
		SetFlag( diffFields, IconLocation, m_iconLocation != right.m_iconLocation );
		SetFlag( diffFields, HotKey, m_hotKey != right.m_hotKey );
		SetFlag( diffFields, ShowCmd, m_showCmd != right.m_showCmd );
		SetFlag( diffFields, LinkDataFlags, m_linkDataFlags != right.m_linkDataFlags );

		return diffFields;
	}

#ifdef USE_UT
	const CFlagTags& CShortcut::GetTags_Fields( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FLAG_TAG( TargetPath ) },
			{ FLAG_TAG( TargetPidl ) },
			{ FLAG_TAG( WorkDirPath ) },
			{ FLAG_TAG( Arguments ) },
			{ FLAG_TAG( Description ) },
			{ FLAG_TAG( IconLocation ) },
			{ FLAG_TAG( HotKey ) },
			{ FLAG_TAG( ShowCmd ) }
		};

		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}
#endif //USE_UT
}


namespace fmt
{
	const CEnumTags& GetTags_ShowCmd( void )
	{
		static const CEnumTags s_tags( _T("Normal Window|Minimized|Maximized"), _T("SW_SHOWNORMAL|SW_SHOWMINIMIZED|SW_SHOWMAXIMIZED"), SW_SHOWNORMAL, SW_SHOWNORMAL );
		return s_tags;
	}

	const CFlagTags& GetTags_ShellLinkDataFlags_Clip( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ SLDF_RUN_IN_SEPARATE, _T("Run in separate memory space") },
			{ SLDF_RUNAS_USER, _T("Run as Admin") }
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}


	std::tstring FormatTarget( const shell::CShortcut& shortcut )
	{	// display the relevant Target as either Path or Pidl:
		if ( shortcut.IsTargetFileBased() )
			return shortcut.GetTargetPath().Get();
		else
		{
			// SIGDN_DESKTOPABSOLUTEEDITING looks more readable than SIGDN_DESKTOPABSOLUTEPARSING, but it cannot be parsed-back in Paste.
			// So we'll use SIGDN_DESKTOPABSOLUTEPARSING, which will be a GUID path for special folders:
			//	SIGDN_DESKTOPABSOLUTEPARSING="::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
			//	rather than SIGDN_DESKTOPABSOLUTEEDITING="Control Panel\\All Control Panel Items\\Region".
			return shortcut.GetTargetPidl().GetName( SIGDN_DESKTOPABSOLUTEPARSING );
		}
	}

	void ParseTarget( OUT shell::CShortcut& rShortcut, const std::tstring& pathOrName ) throws_( CRuntimeException )
	{
		if ( path::IsValid( pathOrName ) )
			rShortcut.SetTargetPath( pathOrName );
		else if ( !pathOrName.empty() )
			rShortcut.SetTargetPidl( shell::pidl::ParseToPidl( pathOrName.c_str(), shell::CreateFileSysBindContext( FILE_ATTRIBUTE_NORMAL ) ) );
		else
			throw CRuntimeException( str::Format( _T("Empty shortcut target") ) );
	}


	std::tstring FormatIconLocation( const shell::CIconLocation& icon )
	{
		if ( icon.IsEmpty() )
			return str::GetEmpty();

		return str::Format( _T("%s (%d)"), icon.m_path.GetPtr(), icon.m_index );
	}

	bool ParseIconLocation( OUT shell::CIconLocation& rIconLoc, const std::tstring& text )
	{
		if ( !text.empty() )
		{
			static const std::tstring s_sep = _T(" (");
			size_t posSep = text.find_last_of( s_sep );

			if ( posSep != std::tstring::npos )
			{
				fs::CPath iconPath = text.substr( 0, posSep );

				if ( path::IsValid( iconPath.GetPtr() ) )
				{
					rIconLoc.m_path.Swap( iconPath );
					rIconLoc.m_index = num::StringToInteger<int>( text.c_str() + posSep + s_sep.length(), nullptr, 10 );
					return true;
				}
			}
		}

		return false;
	}


	std::tstring FormatHotKey( WORD hotKey )
	{
		return FormatKeyShortcut( LOBYTE( hotKey ), HIBYTE( hotKey ) );
	}

	void ParseHotKey( OUT WORD& rHotKey, const std::tstring& text )
	{	// no-op really: not practical to parse a hot-key string
		rHotKey, text;
	}


	const std::tstring& FormatShowCmd( int showCmd )
	{
		switch ( showCmd )
		{
			case SW_SHOWNORMAL:
			case SW_SHOWMINIMIZED:
			case SW_SHOWMAXIMIZED:
				return fmt::GetTags_ShowCmd().FormatUi( showCmd );
			default:
				ASSERT( false );
				return str::GetEmpty();
		}
	}

	void ParseShowCmd( OUT int& rShowCmd, const std::tstring& text )
	{
		rShowCmd = fmt::GetTags_ShowCmd().ParseUi( text );
	}


	std::tstring FormatLinkDataFlags( DWORD linkDataFlags, DWORD filterMask /*= SLDF_RUNAS_USER*/ )
	{
		return GetTags_ShellLinkDataFlags_Clip().FormatKey( linkDataFlags & filterMask );
	}

	void ParseLinkDataFlags( OUT DWORD& rLinkDataFlags, const std::tstring& text, DWORD filterMask /*= SLDF_RUNAS_USER*/ ) throws_( CRuntimeException )
	{
		int linkDataFlags = 0;
		GetTags_ShellLinkDataFlags_Clip().ParseKey( &linkDataFlags, text );

		::SetMaskedValue( rLinkDataFlags, filterMask, linkDataFlags );
	}


	enum ClipField { LinkName, Target, Arguments, WorkDirPath, Description, IconLocation, HotKey, ShowCmd, LinkDataFlags, _FieldCount };

	std::tstring FormatClipShortcut( const std::tstring& linkName, const shell::CShortcut& shortcut, TCHAR sepCh /*= '\t'*/ )
	{
		std::vector<std::tstring> fields( _FieldCount );

		fields[ LinkName ] = linkName;
		fields[ Target ] = fmt::FormatTarget( shortcut );
		fields[ Arguments ] = shortcut.GetArguments();
		fields[ WorkDirPath ] = shortcut.GetWorkDirPath().Get();
		fields[ Description ] = shortcut.GetDescription();
		fields[ IconLocation ] = fmt::FormatIconLocation( shortcut.GetIconLocation() );
		fields[ HotKey ] = fmt::FormatHotKey( shortcut.GetHotKey() );
		fields[ ShowCmd ] = fmt::FormatShowCmd( shortcut.GetShowCmd() );
		fields[ LinkDataFlags ] = fmt::FormatLinkDataFlags( shortcut.GetLinkDataFlags() );

		const TCHAR pSep[] = { sepCh, '\0' };
		return str::Join( fields, pSep );
	}

	shell::CShortcut& ParseClipShortcut( OUT shell::CShortcut& rShortcut, const TCHAR* pLine, const fs::CPath* pKeyPath /*= nullptr*/, TCHAR sepCh /*= '\t'*/ ) throws_( CRuntimeException )
	{
		std::vector<std::tstring> fields;
		const TCHAR pSep[] = { sepCh, '\0' };

		str::Split( fields, pLine, pSep );

		if ( fields.size() < _FieldCount )
		{
			std::tstring errorMsg = str::Format( _T("Invalid number of fields pasted: expecting %d, actual %d"), fields.size(), _FieldCount );
			if ( pKeyPath != nullptr )
				errorMsg += str::Format( _T(" - file: '%s'"), pKeyPath->GetFilenamePtr() );
			throw CRuntimeException( errorMsg );
		}

		ParseTarget( rShortcut, fields[ Target ] );

		rShortcut.SetArguments( fields[ Arguments ] );
		rShortcut.SetWorkDirPath( fields[ WorkDirPath ] );
		rShortcut.SetDescription( fields[ Description ] );

		shell::CIconLocation iconLocation;
		if ( ParseIconLocation( iconLocation, fields[ IconLocation ] ) )
			rShortcut.SetIconLocation( iconLocation );

		WORD hotKey = rShortcut.GetHotKey();
		ParseHotKey( hotKey, fields[ HotKey ] );		// no-op really
		rShortcut.SetHotKey( hotKey );

		int showCmd = rShortcut.GetShowCmd();
		ParseShowCmd( showCmd, fields[ ShowCmd ] );
		rShortcut.SetShowCmd( showCmd );

		DWORD linkDataFlags = rShortcut.GetLinkDataFlags();
		ParseLinkDataFlags( linkDataFlags, fields[ LinkDataFlags ] );
		rShortcut.SetLinkDataFlags( linkDataFlags );

		return rShortcut;
	}
}
