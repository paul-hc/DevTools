
#include "stdafx.h"
#include "AppCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_CommandType( void )
	{
		static const CEnumTags tags(
			_T("Rename Files|Touch Files|Find Duplicates|Delete Files|Undelete Files|Change Destination Paths|Change Destination File States|Reset Destinations|Edit"),
			_T("RENAME|TOUCH|FIND_DUPLICATES|DELETE_FILES|UNDELETE_FILES|CHANGE_DEST_PATHS|CHANGE_DEST_FILE_STATES|RESET_DESTINATIONS|EDIT"),
			-1, RenameFile );

		return tags;
	}


	bool IsPersistentCmd( const utl::ICommand* pCmd )
	{
		if ( is_a< CObject >( pCmd ) )
			if ( const CMacroCommand* pMacroCmd = dynamic_cast< const CMacroCommand* >( pCmd ) )
				return !pMacroCmd->IsEmpty();
			else
				return true;

		return false;
	}

	bool IsZombieCmd( const utl::ICommand* pCmd )
	{
		if ( const CMacroCommand* pMacroCmd = dynamic_cast< const CMacroCommand* >( pCmd ) )
			return pMacroCmd->IsEmpty();

		return pCmd != NULL;
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
