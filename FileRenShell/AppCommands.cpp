
#include "stdafx.h"
#include "AppCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_CommandType( void )
	{
		static const CEnumTags tags(
			_T("Rename Files|Touch Files|Find Duplicates|Delete Files|Move Files|Change Destination Paths|Change Destination File States|Reset Destinations|Edit|Undelete Files"),
			_T("RENAME|TOUCH|FIND_DUPLICATES|DELETE_FILES|MOVE_FILES|CHANGE_DEST_PATHS|CHANGE_DEST_FILE_STATES|RESET_DESTINATIONS|EDIT|UNDELETE_FILES"),
			-1, RenameFile );

		return tags;
	}


	bool IsPersistentCmd( const utl::ICommand* pCmd )
	{
		if ( const IPersistentCmd* pPersistCmd = dynamic_cast< const IPersistentCmd* >( pCmd ) )
			return pPersistCmd->IsValid();

		return false;
	}

	bool IsZombieCmd( const utl::ICommand* pCmd )
	{
		if ( const IPersistentCmd* pPersistCmd = dynamic_cast< const IPersistentCmd* >( pCmd ) )
			return !pPersistCmd->IsValid();

		if ( const CMacroCommand* pMacroCmd = dynamic_cast< const CMacroCommand* >( pCmd ) )
			return pMacroCmd->IsEmpty();

		return NULL == pCmd;
	}


	// command formatting

	const std::tstring& FormatCmdTag( const utl::ICommand* pCmd, utl::Verbosity verbosity )
	{
		ASSERT_PTR( pCmd );
		return GetTags_CommandType().Format( pCmd->GetTypeID(), utl::Brief == verbosity ? CEnumTags::KeyTag : CEnumTags::UiTag );
	}

	const TCHAR* GetSeparator( utl::Verbosity verbosity )
	{
		static const TCHAR s_space[] = _T(" "), s_field[] = _T("|");
		return utl::DetailFields == verbosity ? s_field : s_space;
	}

	std::tstring FormatCmdLine( const utl::ICommand* pCmd, utl::Verbosity verbosity )
	{
		ASSERT_PTR( pCmd );
		std::tstring text = pCmd->Format( verbosity );
		if ( utl::DetailFields == verbosity )
			str::Replace( text, GetSeparator( utl::DetailFields ), GetSeparator( utl::Detailed ) );
		return text;
	}

	void QueryCmdFields( std::vector< std::tstring >& rFields, const utl::ICommand* pCmd )
	{
		str::Split( rFields, pCmd->Format( utl::DetailFields ).c_str(), GetSeparator( utl::DetailFields ) );
	}
}


namespace cmd
{
	// CBaseSerialCmd implementation

	IMPLEMENT_DYNAMIC( CBaseSerialCmd, CObject )

	IErrorObserver* CBaseSerialCmd::s_pErrorObserver = NULL;
	CLogger* CBaseSerialCmd::s_pLogger = (CLogger*)-1;

	CBaseSerialCmd::CBaseSerialCmd( CommandType cmdType /*= CommandType()*/ )
		: CCommand( cmdType, NULL, &GetTags_CommandType() )
	{
		if ( (CLogger*)-1 == s_pLogger )
			s_pLogger = app::GetLogger();
	}

	void CBaseSerialCmd::Serialize( CArchive& archive )
	{
		CCommand::Serialize( archive );		// dis-ambiguate from CObject::Serialize()
	}
}
