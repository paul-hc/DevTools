
#include "stdafx.h"
#include "AppCommands.h"
#include "Application.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/TimeUtils.h"
#include "utl/UI/SystemTray_fwd.h"

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
	void PrefixMsgTypeLine( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType )
	{
		ASSERT_PTR( pOutput );
		stream::Tag( *pOutput, app::GetTags_MsgType().FormatKey( msgType ), NULL );
		stream::Tag( *pOutput, coreMessage, _T("\n") );
	}

	void SuffixMsgType( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType )
	{
		ASSERT_PTR( pOutput );
		stream::Tag( *pOutput, coreMessage, _T(" ") );
		stream::Tag( *pOutput, app::GetTags_MsgType().FormatKey( msgType ), _T(" ") );
	}

	void FormatLogMessage( std::tstring* pOutput, const std::tstring& coreMessage, app::MsgType msgType )
	{
		ASSERT_PTR( pOutput );

		static const CEnumTags s_fmtMsgTags( _T("* %s\n  ERROR|! %s  WARNING|%s") );

		*pOutput = str::Format( s_fmtMsgTags.FormatUi( msgType ).c_str(), coreMessage.c_str() );
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

	std::tstring CBaseSerialCmd::FormatLogMessage( const std::tstring& coreMessage, app::MsgType msgType, const TCHAR* pTitle /*= NULL*/ ) const
	{
		std::tstring message = pTitle != NULL ? pTitle : FormatExecTitle().c_str();

		if ( utl::Contains( coreMessage, _T('\n') ) )		// multi-line?
			cmd::PrefixMsgTypeLine( &message, coreMessage, msgType );
		else
			cmd::SuffixMsgType( &message, coreMessage, msgType );

		return message;
	}

	void CBaseSerialCmd::RecordMessage( const std::tstring& coreMessage, app::MsgType msgType )
	{
		std::tstring title = FormatExecTitle();

		ShowBalloon( coreMessage, msgType, title.c_str() );
		LogOutput( FormatLogMessage( coreMessage, msgType, title.c_str() ) );
	}


	bool CBaseSerialCmd::ShowBalloon( const std::tstring& coreMessage, app::MsgType msgType, const TCHAR* pTitle /*= NULL*/ ) const
	{
		return sys_tray::ShowBalloonMessage( coreMessage, pTitle != NULL ? pTitle : FormatExecTitle().c_str(), msgType );
	}

	void CBaseSerialCmd::LogOutput( const std::tstring& message )
	{
		if ( s_pLogger != NULL )
			s_pLogger->LogString( message );
	}
}
