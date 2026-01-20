#ifndef Shortcut_h
#define Shortcut_h
#pragma once

#include <shobjidl_core.h>		// for IShellLink, CLSID_ShellLink
#include "utl/Path.h"
#include "ShellPidl.h"


class CEnumTags;
class CFlagTags;
class CArchive;


namespace shell
{
	// LINK vs. SHORTCUT terminology:
	//	LINK: refers to a IShellLink from a loaded shortcut .lnk file.
	//	SHORTCUT: refers to the physical file and the shell::CShortcut state object.

	bool IsValidShortcutFile( const TCHAR* pLnkPath );		// checks for SFGAO_LINK attribute

	CComPtr<IShellLink> CreateShellLink( void );			// new shell link object

	CComPtr<IShellLink> LoadLinkFromFile( const TCHAR* pLnkPath );
	bool SaveLinkToFile( const TCHAR* pDestLnkPath, IShellLink* pShellLink );

	fs::CPath GetLinkTargetPath( IShellLink* pShellLink, DWORD pFlags = 0, OUT WIN32_FIND_DATA* pFindData = nullptr );
	fs::CPath GetShortcutTargetPath( const TCHAR* pLnkPath, OUT std::tstring* pArguments = nullptr, OUT WIN32_FIND_DATA* pFindData = nullptr );

	bool ResolveLink( OUT fs::CPath& rTargetPath, IShellLink* pShellLink, CWnd* pWnd = nullptr, DWORD rFlags = 0 );
	bool ResolveShortcut( OUT fs::CPath& rTargetPath, const TCHAR* pLnkPath, CWnd* pWnd = nullptr );
#ifdef USE_UT
	const CFlagTags& GetTags_SFGAO_Flags( void );
	const CFlagTags& GetTags_ShellLinkDataFlags( void );
#endif
}


namespace shell
{
	struct CIconLocation
	{
		CIconLocation( void ) : m_index(-1 ) {}
		CIconLocation( const fs::CPath& iconPath, int iconIndex ) : m_path( iconPath ), m_index( iconIndex ) {}

		bool IsEmpty( void ) const { return m_path.IsEmpty(); }

		bool operator==( const CIconLocation& right ) const { return m_path == right.m_path && m_index == right.m_index; }
		bool operator!=( const CIconLocation& right ) const { return !operator==( right ); }
	public:
		fs::CPath m_path;
		int m_index;
	};


	class CShortcut
	{
	public:
		CShortcut( void ) : m_hotKey( 0 ), m_showCmd( SW_HIDE ), m_changedFields( 0 ), m_linkDataFlags( 0 ) {}
		CShortcut( IN IShellLink* pShellLink );

		void Reset( IN IShellLink* pShellLink = nullptr );
		bool WriteToLink( OUT IShellLink* pDestShellLink ) const;		// uses m_changedFields to pick fields to update

		bool IsTargetEmpty( void ) const { return !IsTargetFileBased() && !m_targetPidl.HasValue(); }
		bool IsTargetValid( void ) const;
		bool IsTargetFileBased( void ) const { return !m_targetPath.IsEmpty(); }

		bool operator==( const CShortcut& right ) const;
		bool operator!=( const CShortcut& right ) const { return !operator==( right ); }

		const fs::CPath& GetTargetPath( void ) const { return m_targetPath; }
		const shell::CPidlAbsCp& GetTargetPidl( void ) const { return m_targetPidl; }
		const fs::CPath& GetWorkDirPath( void ) const { return m_workDirPath; }
		const std::tstring& GetArguments( void ) const { return m_arguments; }
		const std::tstring& GetDescription( void ) const { return m_description; }
		const CIconLocation& GetIconLocation( void ) const { return m_iconLocation; }
		WORD GetHotKey( void ) const { return m_hotKey; }
		int GetShowCmd( void ) const { return m_showCmd; }
		DWORD GetLinkDataFlags( void ) const { return m_linkDataFlags; }

		bool IsRunAsAdmin( void ) const { return HasFlag( m_linkDataFlags, SLDF_RUNAS_USER ); }
		bool IsRunInSepMemSpace( void ) const { return HasFlag( m_linkDataFlags, SLDF_RUN_IN_SEPARATE ); }

		enum Fields
		{
			TargetPath		= BIT_FLAG( 0 ),	// exclusive with TargetPidl
			TargetPidl		= BIT_FLAG( 1 ),	// exclusive with TargetPath
			WorkDirPath		= BIT_FLAG( 2 ),
			Arguments		= BIT_FLAG( 3 ),
			Description		= BIT_FLAG( 4 ),
			IconLocation	= BIT_FLAG( 5 ),
			HotKey			= BIT_FLAG( 6 ),
			ShowCmd			= BIT_FLAG( 7 ),
			LinkDataFlags	= BIT_FLAG( 8 )
		};
		typedef int TFields;

		bool IsDirty( void ) const { return m_changedFields != 0; }
		TFields GetChangedFields( void ) const { return m_changedFields; }
		void ClearChangedFields( void ) { m_changedFields = 0; }

		bool SetTargetPath( const fs::CPath& targetPath ) { return AssignField( m_targetPath, targetPath, TargetPath ); }
		bool SetTargetPidl( PIDLIST_ABSOLUTE pidl, utl::Ownership ownership = utl::MOVE );
		bool SetWorkDirPath( const fs::CPath& workDirPath ) { return AssignField( m_workDirPath, workDirPath, WorkDirPath ); }
		bool SetArguments( const std::tstring& arguments ) { return AssignField( m_arguments, arguments, Arguments ); }
		bool SetDescription( const std::tstring& description ) { return AssignField( m_description, description, Description ); }
		bool SetIconLocation( const CIconLocation& iconLocation ) { return AssignField( m_iconLocation, iconLocation, IconLocation ); }
		bool SetHotKey( WORD hotKey ) { return AssignField( m_hotKey, hotKey, HotKey ); }
		bool SetShowCmd( int showCmd ) { return AssignField( m_showCmd, showCmd, ShowCmd ); }
		bool SetLinkDataFlags( DWORD linkDataFlags ) { return AssignField( m_linkDataFlags, linkDataFlags, LinkDataFlags ); }
		bool ModifyLinkDataFlags( DWORD clearFlags, DWORD setFlags );

		friend inline CArchive& operator<<( CArchive& archive, const CShortcut& shortcut ) { shortcut.Save( archive ); return archive; }
		friend inline CArchive& operator>>( CArchive& archive, OUT CShortcut& rShortcut ) { rShortcut.Load( archive ); return archive; }

		TFields GetDiffFields( const CShortcut& right ) const;		// evaluate fields that are different from 'right'

	#ifdef USE_UT
		static const CFlagTags& GetTags_Fields( void );
	#endif
	protected:
		void Save( OUT CArchive& archive ) const;
		void Load( IN CArchive& archive );
	private:
		template< typename FieldT >
		bool AssignField( OUT FieldT& rField, const FieldT& value, Fields field )
		{
			if ( !utl::ModifyValue( rField, value ) )
				return false;

			SetFlag( m_changedFields, field );
			return true;
		}
	private:
		fs::CPath m_targetPath;
		shell::CPidlAbsCp m_targetPidl;
		fs::CPath m_workDirPath;
		std::tstring m_arguments;
		std::tstring m_description;
		CIconLocation m_iconLocation;
		WORD m_hotKey;					// LOBYTE = vkCode, HIBYTE = modifierFlags
		int m_showCmd;

		// IShellLinkDataList data
		DWORD m_linkDataFlags;			// SHELL_LINK_DATA_FLAGS

		// transient
		mutable TFields m_changedFields;
	};
}


namespace pred
{
	struct IsValidShortcutFile
	{
		bool operator()( const TCHAR* pFilePath ) const { return shell::IsValidShortcutFile( pFilePath ); }
		bool operator()( const std::tstring& filePath ) const { return shell::IsValidShortcutFile( filePath.c_str() ); }
		bool operator()( const fs::CPath& filePath ) const { return shell::IsValidShortcutFile( filePath.GetPtr() ); }
	};
}


namespace fmt
{
	const CEnumTags& GetTags_ShowCmd( void );		// restricted only to: SW_SHOWNORMAL/SW_SHOWMINIMIZED/SW_SHOWMAXIMIZED
	const CFlagTags& GetTags_ShellLinkDataFlags_Clip( void );

	std::tstring FormatTarget( const shell::CShortcut& shortcut );
	std::tstring FormatIconLocation( const shell::CIconLocation& icon );
	std::tstring FormatHotKey( WORD hotKey );		// LOBYTE = vkCode, HIBYTE = modifierFlags
	const std::tstring& FormatShowCmd( int showCmd );
	std::tstring FormatLinkDataFlags( DWORD linkDataFlags, DWORD filterMask = SLDF_RUNAS_USER );

	// clipboard input
	void ParseTarget( OUT shell::CShortcut& rShortcut, const std::tstring& pathOrName ) throws_( CRuntimeException );
	bool ParseIconLocation( OUT shell::CIconLocation& rIconLoc, const std::tstring& text );
	void ParseHotKey( OUT WORD& rHotKey, const std::tstring& text );		// no-op
	void ParseShowCmd( OUT int& rShowCmd, const std::tstring& text ) throws_( CRuntimeException );
	void ParseLinkDataFlags( OUT DWORD& rLinkDataFlags, const std::tstring& text, DWORD filterMask = SLDF_RUNAS_USER ) throws_( CRuntimeException );

	std::tstring FormatClipShortcut( const std::tstring& linkName, const shell::CShortcut& shortcut, TCHAR sepCh = '\t' );
	shell::CShortcut& ParseClipShortcut( OUT shell::CShortcut& rShortcut, const TCHAR* pLine, const fs::CPath* pKeyPath = nullptr, TCHAR sepCh = '\t' ) throws_( CRuntimeException );
}


#endif // Shortcut_h
