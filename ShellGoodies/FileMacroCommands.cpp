
#include "stdafx.h"
#include "FileMacroCommands.h"
#include "utl/AppTools.h"
#include "utl/ContainerOwnership.h"
#include "utl/CommandModel.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/Logger.h"
#include "utl/TimeUtils.h"
#include "utl/SerializeStdTypes.h"
#include "utl/UI/SystemTray_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	// CUserFeedbackException class

	class CUserFeedbackException : public CRuntimeException
	{
	public:
		CUserFeedbackException( UserFeedback feedback ) : CRuntimeException( GetTags_Feedback().FormatUi( feedback ) ), m_feedback( feedback ) {}
	private:
		static const CEnumTags& GetTags_Feedback( void );
	public:
		const UserFeedback m_feedback;
	};

	const CEnumTags& CUserFeedbackException::GetTags_Feedback( void )
	{
		static const CEnumTags s_tags( _T("Abort|Retry|Ignore") );
		return s_tags;
	}
}


namespace cmd
{
	// CFileMacroCmd implementation

	IMPLEMENT_SERIAL( CFileMacroCmd, CObject, VERSIONABLE_SCHEMA | 1 )

	CFileMacroCmd::CFileMacroCmd( CommandType subCmdType, const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CMacroCommand( GetTags_CommandType().FormatKey( subCmdType ), subCmdType )
		, m_timestamp( timestamp )
	{
	}

	bool CFileMacroCmd::IsValid( void ) const override
	{
		return !IsEmpty();
	}

	const CTime& CFileMacroCmd::GetTimestamp( void ) const override
	{
		return m_timestamp;
	}

	size_t CFileMacroCmd::GetFileCount( void ) const override
	{
		return GetSubCommands().size();
	}

	void CFileMacroCmd::Serialize( CArchive& archive )
	{
		CMacroCommand::Serialize( archive );

		archive & m_timestamp;
	}

	void CFileMacroCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const override
	{
		rLines.clear();
		rLines.reserve( m_subCommands.size() );

		for ( std::vector< utl::ICommand* >::const_iterator itCmd = m_subCommands.begin(); itCmd != m_subCommands.end(); ++itCmd )
		{
			CBaseFileCmd* pCmd = checked_static_cast<CBaseFileCmd*>( *itCmd );
			rLines.push_back( pCmd->Format( utl::Detailed ) );
		}
	}

	std::tstring CFileMacroCmd::Format( utl::Verbosity verbosity ) const override
	{
		REQUIRE( !HasOriginCmd() );

		std::tstring text = FormatCmdTag( this, verbosity );
		const TCHAR* pSep = GetSeparator( verbosity );

		if ( verbosity != utl::Brief )
			stream::Tag( text, str::Format( _T("[%d]"), GetSubCommands().size() ), pSep );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, verbosity != utl::Brief ? time_utl::s_outFormatUi : time_utl::s_outFormat ), pSep );

		return text;
	}

	bool CFileMacroCmd::Execute( void ) override
	{
		ExecuteMacro( ExecuteMode );			// removes commands that failed
		return !m_subCommands.empty();			// any succeeeded?
	}

	bool CFileMacroCmd::Unexecute( void ) override
	{
		ExecuteMacro( UnexecuteMode );			// removes commands that failed
		return !m_subCommands.empty();			// any succeeeded?
	}

	void CFileMacroCmd::ExecuteMacro( CmdMode cmdMode )
	{
		CExecMessage macroMessage;

		for ( std::vector< utl::ICommand* >::iterator itCmd = m_subCommands.begin(); itCmd != m_subCommands.end(); )
		{
			CBaseFileCmd* pCmd = checked_static_cast<CBaseFileCmd*>( *itCmd );

			try
			{
				macroMessage.StoreLeafCmd( pCmd, cmdMode )->ExecuteHandle();
			}
			catch ( CUserFeedbackException& exc )
			{
				switch ( exc.m_feedback )
				{
					case Retry:
						continue;
					case Ignore:
						macroMessage.AppendLeafMessage();
						// discard command since we cannot unexecute
						delete *itCmd;
						itCmd = m_subCommands.erase( itCmd );
						continue;
					case Abort:
						macroMessage.AppendLeafMessage();
						// discard remaining commands since will not be unexecuted
						std::for_each( itCmd, m_subCommands.end(), func::Delete() );
						itCmd = m_subCommands.erase( itCmd, m_subCommands.end() );
						continue;		// will break the loop, since we are at end()
				}
			}

			macroMessage.AppendLeafMessage();
			++itCmd;
		}

		std::tstring title = FormatExecTitle();

		// display balloon tip with aggregate macro message
		sys_tray::ShowBalloonMessage( macroMessage.m_message, title.c_str(), macroMessage.m_msgType );

		cmd::PrefixMsgTypeLine( &title, macroMessage.m_message, macroMessage.m_msgType );
		CBaseSerialCmd::LogOutput( title );
	}


	// CFileMacroCmd::CExecMessage implementation

	CBaseFileCmd* CFileMacroCmd::CExecMessage::StoreLeafCmd( CBaseFileCmd* pLeafCmd, CmdMode cmdMode )
	{
		ASSERT_PTR( pLeafCmd );
		m_pLeafCmd = pLeafCmd;

		if ( ExecuteMode == cmdMode )
		{
			m_pReverseLeafCmd.reset();
			return m_pLeafCmd;
		}
		else
		{
			m_pReverseLeafCmd = m_pLeafCmd->MakeUnexecuteCmd();
			return m_pReverseLeafCmd.get();
		}
	}

	void CFileMacroCmd::CExecMessage::AppendLeafMessage( void )
	{
		ASSERT_PTR( m_pLeafCmd );

		if ( m_pReverseLeafCmd.get() != NULL )		// a reverse command?
			m_pLeafCmd->SetExecMessage( m_pReverseLeafCmd->GetExecMessage() );	// store the message in the original command

		if ( m_cmdPos++ != 0 )
			m_message += _T("\n");

		const TMessagePair& execMessage = m_pLeafCmd->GetExecMessage();

		m_message += execMessage.first;
		m_msgType = std::min( m_msgType, execMessage.second );		// Error wins over Warning or Info

		// reset the handled commands data-members as they go out-of-scope
		m_pLeafCmd = NULL;
		m_pReverseLeafCmd.reset();
	}


	// CBaseFileCmd implementation

	const TCHAR CBaseFileCmd::s_fmtError[] = _T("* %s\n  ERROR: ");

	CBaseFileCmd::CBaseFileCmd( CommandType cmdType /*= CommandType()*/, const fs::CPath& srcPath /*= fs::CPath()*/ )
		: CBaseSerialCmd( cmdType )
		, m_srcPath( srcPath )
	{
	}

	void CBaseFileCmd::RecordMessage( const std::tstring& coreMessage, app::MsgType msgType )
	{
		// note: owner macro command stores the aggregate messages for balloon and logging
		m_execMessage.first = coreMessage;
		m_execMessage.second = msgType;
	}

	void CBaseFileCmd::ExecuteHandle( void ) throws_( CUserFeedbackException )
	{
		try
		{
			m_execMessage = TMessagePair();		// reset the message

			Execute();

			RecordMessage( Format( utl::Detailed ), app::Info );
		}
		catch ( CException* pExc )
		{
			throw CUserFeedbackException( HandleFileError( pExc, m_srcPath ) );
		}
	}

	bool CBaseFileCmd::Unexecute( void ) override
	{
		std::auto_ptr<CBaseFileCmd> pUndoCmd( MakeUnexecuteCmd() );

		return pUndoCmd.get() != NULL && pUndoCmd->Execute();
	}

	UserFeedback CBaseFileCmd::HandleFileError( CException* pExc, const fs::CPath& srcPath )
	{
		std::tstring errMsg = ExtractMessage( pExc, srcPath );

		if ( s_pErrorObserver != NULL )
			s_pErrorObserver->OnFileError( srcPath, errMsg );

		UserFeedback feedback;
		switch ( AfxMessageBox( errMsg.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
		{
			default: ASSERT( false );
			case IDRETRY:	return Retry;				// no logging on retry, give it another chance to fix the problem
			case IDABORT:	feedback = Abort; break;
			case IDIGNORE:	feedback = Ignore; break;
		}

		RecordMessage( str::Format( s_fmtError, Format( utl::Detailed ).c_str() ) + errMsg, app::Error );
		return feedback;
	}

	std::tstring CBaseFileCmd::ExtractMessage( CException* pExc, const fs::CPath& srcPath )
	{
		std::tstring message = mfc::CRuntimeException::MessageOf( *pExc );
		pExc->Delete();

		if ( !srcPath.FileExist() )
			return str::Format( _T("Cannot find file: %s"), srcPath.GetPtr() );
		return message;
	}
}


// CRenameFileCmd implementation

IMPLEMENT_SERIAL( CRenameFileCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CRenameFileCmd::CRenameFileCmd( const fs::CPath& srcPath, const fs::CPath& destPath )
	: CBaseFileCmd( cmd::RenameFile, srcPath )
	, m_destPath( destPath )
{
}

CRenameFileCmd::~CRenameFileCmd()
{
}

std::tstring CRenameFileCmd::Format( utl::Verbosity verbosity ) const override
{
	if ( HasOriginCmd() )
		return GetOriginCmd()->Format( verbosity );

	std::tstring text;
	if ( verbosity != utl::Brief )
		text = __super::Format( utl::Brief );		// prepend "RENAME" tag

	stream::Tag( text, fmt::FormatRenameEntry( m_srcPath, m_destPath ), _T(" ") );
	return text;
}

bool CRenameFileCmd::Execute( void )
{
	CFile::Rename( m_srcPath.GetPtr(), m_destPath.GetPtr() );
	NotifyObservers();
	return true;
}

std::auto_ptr<cmd::CBaseFileCmd> CRenameFileCmd::MakeUnexecuteCmd( void ) override
{
	cmd::CBaseFileCmd* pUnexecCmd = new CRenameFileCmd( m_destPath, m_srcPath );
	pUnexecCmd->SetOriginCmd( this );
	return std::auto_ptr<cmd::CBaseFileCmd>( pUnexecCmd );
}

bool CRenameFileCmd::IsUndoable( void ) const override
{
	//return m_destPath.FileExist() && !m_srcPath.FileExist();
	return true;		// let it unexecute with error rather than being skipped in UNDO
}

void CRenameFileCmd::Serialize( CArchive& archive ) override
{
	cmd::CBaseFileCmd::Serialize( archive );

	archive & m_srcPath;
	archive & m_destPath;
}


// CTouchFileCmd implementation

IMPLEMENT_SERIAL( CTouchFileCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CTouchFileCmd::CTouchFileCmd( const fs::CFileState& srcState, const fs::CFileState& destState )
	: CBaseFileCmd( cmd::TouchFile, srcState.m_fullPath )
	, m_srcState( srcState )
	, m_destState( destState )
{
	ASSERT( m_srcState.m_fullPath == m_destState.m_fullPath );
}

CTouchFileCmd::~CTouchFileCmd()
{
}

std::tstring CTouchFileCmd::Format( utl::Verbosity verbosity ) const override
{
	if ( HasOriginCmd() )
		return GetOriginCmd()->Format( verbosity );

	std::tstring text;
	if ( verbosity != utl::Brief )
		text = __super::Format( utl::Brief );		// prepend "TOUCH" tag

	stream::Tag( text, fmt::FormatTouchEntry( m_srcState, m_destState ), _T(" ") );
	return text;
}

bool CTouchFileCmd::Execute( void ) override
{
	if ( m_destState != m_srcState )
	{
		m_destState.WriteToFile();
		NotifyObservers();
	}
	return true;
}

std::auto_ptr<cmd::CBaseFileCmd> CTouchFileCmd::MakeUnexecuteCmd( void ) override
{
	cmd::CBaseFileCmd* pUnexecCmd = new CTouchFileCmd( m_destState, m_srcState );
	pUnexecCmd->SetOriginCmd( this );
	return std::auto_ptr<cmd::CBaseFileCmd>( pUnexecCmd );
}

bool CTouchFileCmd::IsUndoable( void ) const override
{
	//return m_srcState.FileExist() && m_srcState != fs::CFileState::ReadFromFile( m_srcState.m_fullPath );
	return true;		// let it unexecute with error rather than being skipped in UNDO
}

void CTouchFileCmd::Serialize( CArchive& archive ) override
{
	cmd::CBaseFileCmd::Serialize( archive );

	if ( archive.IsStoring() )
	{
		archive << m_srcState;
		archive << m_destState;
	}
	else
	{
		archive >> m_srcState;
		archive >> m_destState;

		m_srcPath = m_srcState.m_fullPath;			// assign path (e.g. for exception messages)
	}
}
