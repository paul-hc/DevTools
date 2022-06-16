
#include "stdafx.h"
#include "AppCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/TimeUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_CommandType( void )
	{
		static CEnumTags s_tags( -1, RenameFile );

		if ( s_tags.IsEmpty() )
		{	// persistent commands
			s_tags.AddTagPair( _T("Rename Files"), _T("RENAME") );
			s_tags.AddTagPair( _T("Touch Files"), _T("TOUCH") );
			s_tags.AddTagPair( _T("Find Duplicates"), _T("FIND_DUPLICATES") );
			s_tags.AddTagPair( _T("Delete Files"), _T("DELETE_FILES") );
			s_tags.AddTagPair( _T("Copy Files"), _T("COPY_FILES") );
			s_tags.AddTagPair( _T("Paste Copy Files"), _T("PASTE_COPY_FILES") );
			s_tags.AddTagPair( _T("Move Files"), _T("MOVE_FILES") );
			s_tags.AddTagPair( _T("Paste Move Files"), _T("PASTE_MOVE_FILES") );
			s_tags.AddTagPair( _T("Create Folders"), _T("CREATE_FOLDERS") );
			s_tags.AddTagPair( _T("Paste Create Folders"), _T("PASTE_CREATE_FOLDERS") );
			s_tags.AddTagPair( _T("Paste Create Deep Folders"), _T("PASTE_CREATE_DEEP_FOLDERS") );
			s_tags.AddTagPair( _T("Copy and Paste Files as Backup"), _T("COPY_PASTE_FILES_AS_BACKUP") );
			s_tags.AddTagPair( _T("Cut and Paste Files as Backup"), _T("CUT_PASTE_FILES_AS_BACKUP") );

			// transient commands
			s_tags.AddTagPair( _T("Change Destination Paths"), _T("CHANGE_DEST_PATHS") );
			s_tags.AddTagPair( _T("Change Destination File States"), _T("CHANGE_DEST_FILE_STATES") );
			s_tags.AddTagPair( _T("Reset Destinations"), _T("RESET_DESTINATIONS") );
			s_tags.AddTagPair( _T("Edit"), _T("EDIT") );
			s_tags.AddTagPair( _T("SortRenameItems") );
			s_tags.AddTagPair( _T("OnRenameListSelChanged") );
			s_tags.AddTagPair( _T("Undelete Files"), _T("UNDELETE_FILES") );
		}
		return s_tags;
	}


	bool IsPersistentCmd( const utl::ICommand* pCmd )
	{
		if ( const IPersistentCmd* pPersistCmd = dynamic_cast<const IPersistentCmd*>( pCmd ) )
			return pPersistCmd->IsValid();

		return false;
	}

	bool IsZombieCmd( const utl::ICommand* pCmd )
	{
		if ( const IPersistentCmd* pPersistCmd = dynamic_cast<const IPersistentCmd*>( pCmd ) )
			return !pPersistCmd->IsValid();

		if ( const CMacroCommand* pMacroCmd = dynamic_cast<const CMacroCommand*>( pCmd ) )
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

	bool CBaseSerialCmd::LogMessage( const std::tstring& message, app::MsgType msgType )
	{
		DWORD infoFlag = NIIF_NONE;
		UINT timeoutSecs = 10;

		switch ( msgType )
		{
			case app::Error:	infoFlag = NIIF_ERROR; timeoutSecs = 30; break;
			case app::Warning:	infoFlag = NIIF_WARNING; timeoutSecs = 20; break;
			case app::Info:		infoFlag = NIIF_INFO; break;
		}

		ui::sys_tray::ShowBalloonTip( message, _T("Shell Goodies"), infoFlag, timeoutSecs );

		if ( s_pLogger != NULL )
			s_pLogger->LogString( message );

		return s_pLogger != NULL;
	}

	void CBaseSerialCmd::LogExecution( const std::tstring& message, app::MsgType msgType )
	{
		std::tstring execMessage = utl::GetTags_ExecMode().FormatUi( CCommandModel::GetExecMode() );
		stream::Tag( execMessage, message, _T(": ") );

		LogMessage( execMessage, msgType );
	}
}
