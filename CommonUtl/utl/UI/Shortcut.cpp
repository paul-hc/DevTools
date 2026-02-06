
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
		return ::HasFlag( shell::GetRawSfgaofFlags( pLnkPath ), SFGAO_LINK );
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


	const CFlagTags& GetTags_ShellLinkDataFlags( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
		#ifdef _DEBUG
			{ FLAG_TAG( SLDF_HAS_ID_LIST ) },			// Shell link saved with ID list
			{ FLAG_TAG( SLDF_HAS_LINK_INFO ) },			// link saved with LinkInfo used by .lnk files to locate the target if the targets's path has changed.
			{ FLAG_TAG( SLDF_HAS_NAME ) },				// link has a name.
			{ FLAG_TAG( SLDF_HAS_RELPATH ) },			// link has a relative path.
			{ FLAG_TAG( SLDF_HAS_WORKINGDIR ) },		// link has a working directory.
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
			{ FLAG_AS_TAG( DWORD, SLDF_RESERVED ) }			// Reserved-- so we can use the low word as an index value in the future
		#endif

		#else
			NULL_TAG
		#endif //_DEBUG
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}


	// CIconLocation implementation

	std::tstring CIconLocation::Format( void ) const
	{
		if ( IsEmpty() )
			return str::GetEmpty();

		return str::Format( _T("%s:%d"), m_path.GetPtr(), m_index );
	}

	bool CIconLocation::Parse( const std::tstring& text )
	{
		if ( !text.empty() )
		{
			size_t posSep = text.find( ':' );

			if ( posSep != std::tstring::npos )
			{
				fs::CPath iconPath = text.substr( 0, posSep );

				if ( path::IsValidPath( iconPath.GetPtr() ) )
				{
					const TCHAR* pIndex = text.c_str() + posSep + 1;

					m_path.Swap( iconPath );
					m_index = num::StringToInteger<int>( pIndex, nullptr, 10 );
					return true;
				}
			}
		}

		return false;
	}


	// CShortcut implementation

	CShortcut::CShortcut( void )
		: m_linkDataFlags( 0 )
		, m_hotKey( 0 )
		, m_showCmd( SW_HIDE )
		, m_changedFields( 0 )
	{
	}

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

		enum { BufferSize = INFOTIPSIZE };			// 1024: max size for GetArguments()
		std::vector<TCHAR> buffer( BufferSize );
		TCHAR* pBuffer = utl::Data( buffer );

		if ( HR_OK( pShellLink->GetPath( pBuffer, BufferSize, nullptr, SLGP_RAWPATH ) ) )		// possible to err with E_NOTIMPL, or INPLACE_S_TRUNCATED?!
			m_targetPath.Set( pBuffer );
		else if ( HR_OK( pShellLink->GetPath( pBuffer, BufferSize, nullptr, 0 ) ) )
			m_targetPath.Set( pBuffer );

		HR_AUDIT( pShellLink->GetIDList( &m_targetPidl.Ref() ) );

		if ( HR_OK( pShellLink->GetWorkingDirectory( pBuffer, BufferSize ) ) )
			m_workDirPath.Set( pBuffer );

		if ( HR_OK( pShellLink->GetArguments( pBuffer, BufferSize ) ) )
			m_arguments = pBuffer;

		if ( HR_OK( pShellLink->GetDescription( pBuffer, BufferSize ) ) )
			m_description = pBuffer;

		if ( HR_OK( pShellLink->GetIconLocation( pBuffer, BufferSize, &m_iconLocation.m_index ) ) )
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

	bool CShortcut::IsValidTarget( void ) const
	{
		if ( HasTarget() )
			return IsTargetNonFileSys()
				? shell::ShellItemExist( m_targetPidl )
				: shell::ShellItemExist( m_targetPath.GetPtr() );

		return false;
	}

	bool CShortcut::operator==( const CShortcut& right ) const
	{
		return
			m_linkDataFlags == right.m_linkDataFlags &&
			m_targetPath.Get() == right.m_targetPath.Get() &&		// case-sensitive compare
			m_targetPidl == right.m_targetPidl &&
			m_workDirPath.Get() == right.m_workDirPath.Get() &&		// case-sensitive compare
			m_arguments == right.m_arguments &&
			m_description == right.m_description &&
			m_hotKey == right.m_hotKey &&
			m_iconLocation == right.m_iconLocation &&
			m_showCmd == right.m_showCmd;
	}

	CShortcut::TFields CShortcut::GetDiffFields( const CShortcut& right ) const
	{
		TFields diffFields = 0;

		SetFlag( diffFields, LinkDataFlags, m_linkDataFlags != right.m_linkDataFlags );
		SetFlag( diffFields, TargetPath, m_targetPath != right.m_targetPath );
		SetFlag( diffFields, TargetPidl, m_targetPidl != right.m_targetPidl );
		SetFlag( diffFields, WorkDirPath, m_workDirPath != right.m_workDirPath );
		SetFlag( diffFields, Arguments, m_arguments != right.m_arguments );
		SetFlag( diffFields, Description, m_description != right.m_description );
		SetFlag( diffFields, IconLocation, m_iconLocation != right.m_iconLocation );
		SetFlag( diffFields, HotKey, m_hotKey != right.m_hotKey );
		SetFlag( diffFields, ShowCmd, m_showCmd != right.m_showCmd );

		return diffFields;
	}

	bool CShortcut::SetTargetPidl( PIDLIST_ABSOLUTE pidl )
	{
		bool changed = !shell::pidl::IsEqual( pidl, m_targetPidl.Get() );

		m_targetPidl.Reset( pidl );
		if ( changed )
			SetFlag( m_changedFields, TargetPidl );

		return changed;
	}

	bool CShortcut::ModifyLinkDataFlags( DWORD clearFlags, DWORD setFlags )
	{
		if ( !::ModifyFlags( m_linkDataFlags, clearFlags, setFlags ) )
			return false;

		::SetFlag( m_linkDataFlags, LinkDataFlags );
		return true;
	}

	std::tstring CShortcut::FormatTarget( SIGDN pidlFmt /*= SIGDN_DESKTOPABSOLUTEEDITING*/ ) const
	{	// display the relevant Target as either Path (file system) or PIDL (shell special object, e.g. Control Panel apps):
		if ( IsTargetFileSys() )
			return m_targetPath.Get();
		else if ( IsTargetNonFileSys() )
			return m_targetPidl.GetName( pidlFmt );		// e.g. "Control Panel\\All Control Panel Items\\Region"

		return str::GetEmpty();
	}

	bool CShortcut::StoreTarget( const std::tstring& targetShellPath, SIGDN pidlFmt /*= SIGDN_DESKTOPABSOLUTEPARSING*/ )
	{
		bool changed = false;

		if ( path::IsValidPath( targetShellPath ) )
		{
			changed = SetTargetPath( targetShellPath );
			if ( changed )
				changed |= SetTargetPidl( shell::pidl::ParseToPidlFileSys( targetShellPath.c_str() ) );
		}
		else if ( path::IsValidGuidPath( targetShellPath ) )
		{
			if ( SIGDN_DESKTOPABSOLUTEPARSING == pidlFmt )
				changed |= SetTargetPidl( shell::pidl::ParseToPidlFileSys( targetShellPath.c_str() ) );
			else
			{
				ASSERT( false );	// assume the pidl string was formatted as SIGDN_DESKTOPABSOLUTEEDITING, which is user-readable but not parsable
				TRACE( _T("! CShortcut::StoreTarget(): ignoring input PIDL string '%s' since it's not parsable\n"), targetShellPath.c_str() );
			}
		}
		else
		{
			ASSERT( targetShellPath.empty() );
			changed |= SetTargetPath( targetShellPath );
			changed |= SetTargetPidl( nullptr );
		}

		return changed;
	}

	void CShortcut::Save( OUT CArchive& archive ) const
	{
		archive
			<< m_linkDataFlags << m_targetPath << m_targetPidl << m_workDirPath << m_arguments << m_description
			<< m_iconLocation.m_path << m_iconLocation.m_index << m_hotKey << m_showCmd;
	}

	void CShortcut::Load( IN CArchive& archive )
	{
		archive
			>> m_linkDataFlags >> m_targetPath >> m_targetPidl >> m_workDirPath >> m_arguments >> m_description
			>> m_iconLocation.m_path >> m_iconLocation.m_index >> m_hotKey >> m_showCmd;
	}


	const CFlagTags& CShortcut::GetTags_Fields( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
		#ifdef _DEBUG
			{ FLAG_TAG( TargetPath ) },
			{ FLAG_TAG( TargetPidl ) },
			{ FLAG_TAG( WorkDirPath ) },
			{ FLAG_TAG( Arguments ) },
			{ FLAG_TAG( Description ) },
			{ FLAG_TAG( IconLocation ) },
			{ FLAG_TAG( HotKey ) },
			{ FLAG_TAG( ShowCmd ) },
			{ FLAG_TAG( LinkDataFlags ) }
		#else
			NULL_TAG
		#endif //_DEBUG
		};

		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}
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
			FLAG_TAG_KEY( SLDF_RUN_IN_SEPARATE, "Run in separate memory space" ),
			FLAG_TAG_KEY( SLDF_RUNAS_USER, "Run as Admin" )
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}


	// clipboard output/input

	enum ClipField { LinkName, Target, Arguments, WorkDirPath, Description, IconLocation, HotKey, ShowCmd, LinkDataFlags, _FieldCount };

	std::tstring FormatClipShortcut( const std::tstring& linkName, const shell::CShortcut& shortcut, TCHAR sepCh /*= '\t'*/ )
	{
		std::vector<std::tstring> fields( _FieldCount );

		fields[ LinkName ] = linkName;
		fields[ Target ] = fmt::FormatTarget( shortcut );
		fields[ Arguments ] = shortcut.GetArguments();
		fields[ WorkDirPath ] = shortcut.GetWorkDirPath().Get();
		fields[ Description ] = shortcut.GetDescription();
		fields[ IconLocation ] = shortcut.GetIconLocation().Format();
		fields[ HotKey ] = fmt::FormatHotKeyValue( shortcut );
		fields[ ShowCmd ] = fmt::FormatShowCmd( shortcut );
		fields[ LinkDataFlags ] = fmt::FormatLinkDataFlags( shortcut );

		const TCHAR pSep[] = { sepCh, '\0' };
		return str::Join( fields, pSep );
	}

	bool ParseClipShortcut( OUT shell::CShortcut& rShortcut, const TCHAR* pLine, TCHAR sepCh /*= '\t'*/ ) throws_( CRuntimeException )
	{
		std::vector<std::tstring> fields;
		const TCHAR pSep[] = { sepCh, '\0' };

		str::Split( fields, pLine, pSep );

		if ( fields.size() < _FieldCount )
		{
			std::tstring errorMsg = str::Format( _T("Invalid number of fields pasted: expecting %d, actual %d"), fields.size(), _FieldCount );
			if ( !fields.empty() )
				errorMsg += str::Format( _T(" - file: '%s'"), fields[ LinkName ].c_str() );

			throw CRuntimeException( errorMsg );
		}

		bool changed = false;

		changed |= ParseTarget( rShortcut, fields[ Target ] );
		changed |= rShortcut.SetArguments( fields[ Arguments ] );
		changed |= rShortcut.SetWorkDirPath( fields[ WorkDirPath ] );
		changed |= rShortcut.SetDescription( fields[ Description ] );
		changed |= ParseIconLocation( rShortcut, fields[ IconLocation ] );
		changed |= ParseHotKeyValue( rShortcut, fields[ HotKey ] );
		changed |= ParseShowCmd( rShortcut, fields[ ShowCmd ] );
		changed |= ParseLinkDataFlags( rShortcut, fields[ LinkDataFlags ] );

		return changed;
	}


	// field output/input

	std::tstring FormatTarget( const shell::CShortcut& shortcut )
	{
		return shortcut.FormatTarget( SIGDN_DESKTOPABSOLUTEPARSING );
	}

	bool ParseTarget( OUT shell::CShortcut& rShortcut, const std::tstring& targetPathOrName ) throws_( CRuntimeException )
	{
		if ( targetPathOrName.empty() )
			throw CRuntimeException( _T("Empty shortcut target") );

		return rShortcut.StoreTarget( targetPathOrName, SIGDN_DESKTOPABSOLUTEPARSING );
	}


	std::tstring FormatIconLocation( const shell::CShortcut& shortcut )
	{
		return shortcut.GetIconLocation().Format();
	}

	bool ParseIconLocation( OUT shell::CShortcut& rShortcut, const std::tstring& text )
	{
		shell::CIconLocation iconLocation;
		return iconLocation.Parse( text ) && rShortcut.SetIconLocation( iconLocation );
	}


	std::tstring FormatHotKey( WORD hotKey )
	{
		return fmt::FormatKeyShortcut( LOBYTE( hotKey ), HIBYTE( hotKey ) );
	}

	std::tstring FormatHotKeyValue( const shell::CShortcut& shortcut )
	{
		WORD hotKey = shortcut.GetHotKey();
		return str::Format( _T("%04X (%s)"), hotKey, fmt::FormatKeyShortcut( LOBYTE( hotKey ), HIBYTE( hotKey ) ).c_str() );
	}

	bool ParseHotKeyValue( OUT shell::CShortcut& rShortcut, const std::tstring& text )
	{	// no-op really: not practical to parse a hot-key string
		UINT hotKey = 0;

		if ( _stscanf( text.c_str(), _T( "%04X" ), &hotKey ) != 1 )
		{
			TRACE( _T("- Invalid HotKey field: '%s'\n"), text.c_str() );
			return false;
		}

		return rShortcut.SetHotKey( static_cast<WORD>( hotKey ) );
	}


	const std::tstring& FormatShowCmd( const shell::CShortcut& shortcut )
	{
		switch ( shortcut.GetShowCmd() )
		{
			case SW_SHOWNORMAL:
			case SW_SHOWMINIMIZED:
			case SW_SHOWMAXIMIZED:
				return fmt::GetTags_ShowCmd().FormatUi( shortcut.GetShowCmd() );
			default:
				ASSERT( false );
				return str::GetEmpty();
		}
	}

	bool ParseShowCmd( OUT shell::CShortcut& rShortcut, const std::tstring& text )
	{
		return rShortcut.SetShowCmd( fmt::GetTags_ShowCmd().ParseUi( text ) );
	}


	std::tstring FormatLinkDataFlags( const shell::CShortcut& shortcut )
	{
		return str::Format( _T("%08X"), shortcut.GetLinkDataFlags() );
	}

	bool ParseLinkDataFlags( OUT shell::CShortcut& rShortcut, const std::tstring& text ) throws_( CRuntimeException )
	{
		DWORD linkDataFlags = 0;

		if ( _stscanf( text.c_str(), _T("%08X"), &linkDataFlags ) != 1 )
			throw CRuntimeException( str::Format( _T("Invalid LinkDataFlags field: '%s'"), text.c_str() ) );

		return rShortcut.SetLinkDataFlags( linkDataFlags );
	}

	std::tstring FormatLinkDataFlagTags( DWORD linkDataFlags, DWORD filterMask /*= SLDF_RUNAS_USER*/ )
	{
		return GetTags_ShellLinkDataFlags_Clip().FormatKey( linkDataFlags & filterMask );
	}

	void ParseLinkDataFlagTags( OUT DWORD& rLinkDataFlags, const std::tstring& text, DWORD filterMask /*= SLDF_RUNAS_USER*/ ) throws_( CRuntimeException )
	{
		int linkDataFlags = 0;
		GetTags_ShellLinkDataFlags_Clip().ParseKey( &linkDataFlags, text );
		::SetMaskedValue( rLinkDataFlags, filterMask, linkDataFlags );
	}
}
