
#include "stdafx.h"
#include "FileCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_Command( void )
	{
		static const CEnumTags tags( _T("Rename Files|Touch Files"), _T("RENAME|TOUCH"), -1, RenameFile );
		return tags;
	}


	// CFileCmd implementation

	CFileCmd::CFileCmd( Command command, const fs::CPath& srcPath )
		: CCommand( command, NULL, &cmd::GetTags_Command() )
		, m_srcPath( srcPath )
	{
	}


	// CFileMacroCmd implementation

	const TCHAR CFileMacroCmd::s_fmtError[] = _T("* %s\n ERROR: %s");

	CFileMacroCmd::CFileMacroCmd( Command subCmdType, IErrorObserver* pErrorObserver, const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CMacroCommand( GetTags_Command().FormatKey( subCmdType ), subCmdType )
		, m_timestamp( timestamp )
		, m_pErrorObserver( pErrorObserver )
	{
	}

	std::tstring CFileMacroCmd::Format( bool detailed ) const
	{
		detailed;
		std::tstring text = m_userInfo;

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp ), _T(" ") );
		return text;
	}

	bool CFileMacroCmd::Execute( void )
	{
		size_t doneCmdCount = ExecuteMacro( ExecuteMode );
		if ( doneCmdCount != m_subCommands.size() )				// aborted by user?
		{
			RollbackMacro( doneCmdCount, UnexecuteMode );
			return false;
		}

		return doneCmdCount != 0;								// any executed?
	}

	bool CFileMacroCmd::Unexecute( void )
	{
		size_t doneCmdCount = ExecuteMacro( UnexecuteMode );
		if ( doneCmdCount != m_subCommands.size() )				// aborted by user?
		{
			RollbackMacro( doneCmdCount, ExecuteMode );
			return false;
		}

		return doneCmdCount != 0;								// any unexecuted?
	}

	size_t CFileMacroCmd::ExecuteMacro( Mode mode )
	{
		if ( m_pErrorObserver != NULL )
			m_pErrorObserver->ClearFileErrors();

		for ( size_t pos = 0; pos != m_subCommands.size(); )
		{
			utl::ICommand* pCmd = m_subCommands[ pos ];

			try
			{
				if ( ExecuteMode == mode )
					pCmd->Execute();
				else
					pCmd->Unexecute();

				app::GetLogger().LogString( pCmd->Format( true ) );
			}
			catch ( CException* pExc )
			{
				switch ( HandleFileError( checked_static_cast< const CFileCmd* >( pCmd ), pExc ) )
				{
					case Retry:	 continue;
					case Ignore:
						RemoveCmdAt( pos );		// discard command since cannot un-execute it
						continue;
					case Abort:
						RemoveCmdAt( pos );		// discard command since cannot un-execute it
						return pos;
				}
			}

			++pos;
		}

		return m_subCommands.size();
	}

	void CFileMacroCmd::RollbackMacro( size_t doneCmdCount, Mode mode )
	{
		// rollback silently the commands that succeeded
		for ( size_t pos = 0; pos != doneCmdCount; ++pos )
		{
			utl::ICommand* pCmd = m_subCommands[ pos ];
			try
			{
				if ( ExecuteMode == mode )
					pCmd->Execute();
				else
					pCmd->Unexecute();
			}
			catch ( CException* pExc )
			{
				std::tstring errMsg = ExtractMessage( checked_static_cast< const CFileCmd* >( pCmd )->m_srcPath, pExc ); errMsg;
				TRACE( _T(" * Error rolling back %s macro command: %s\n"), Format( false ).c_str(), errMsg.c_str() );
			}
		}
	}

	void CFileMacroCmd::RemoveCmdAt( size_t pos )
	{
		REQUIRE( pos < m_subCommands.size() );
		delete m_subCommands[ pos ];
		m_subCommands.erase( m_subCommands.begin() + pos );
	}


	CFileMacroCmd::UserFeedback CFileMacroCmd::HandleFileError( const CFileCmd* pCmd, CException* pExc ) const
	{
		std::tstring errMsg = ExtractMessage( pCmd->m_srcPath, pExc );
		if ( m_pErrorObserver != NULL )
			m_pErrorObserver->OnFileError( pCmd->m_srcPath, errMsg );

		UserFeedback feedback;
		switch ( AfxMessageBox( errMsg.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
		{
			default: ASSERT( false );
			case IDRETRY:	return Retry;				// no logging on retry, give it another chance to fix the problem
			case IDABORT:	feedback = Abort; break;
			case IDIGNORE:	feedback = Ignore; break;
		}

		app::GetLogger().Log( s_fmtError, pCmd->Format( true ).c_str(), errMsg.c_str() );
		return feedback;
	}

	std::tstring CFileMacroCmd::ExtractMessage( const fs::CPath& srcPath, CException* pExc )
	{
		std::tstring message = ui::GetErrorMessage( pExc );
		pExc->Delete();

		if ( !srcPath.FileExist() )
			return str::Format( _T("Cannot find file:\n%s\n%s"), srcPath.GetPtr(), message.c_str() );
		return message;
	}
}


// CRenameFileCmd implementation

CRenameFileCmd::CRenameFileCmd( const fs::CPath& srcPath, const fs::CPath& destPath )
	: CFileCmd( cmd::RenameFile, srcPath )
	, m_destPath( destPath )
{
}

CRenameFileCmd::~CRenameFileCmd()
{
}

std::tstring CRenameFileCmd::Format( bool detailed ) const
{
	std::tstring text;
	if ( detailed )
		text = CFileCmd::Format( false );		// prepend "RENAME" tag

	stream::Tag( text, fmt::FormatRenameEntry( m_srcPath, m_destPath ), _T(" ") );
	return text;
}

bool CRenameFileCmd::Execute( void )
{
	CFile::Rename( m_srcPath.GetPtr(), m_destPath.GetPtr() );
	NotifyObservers();
	return true;
}

bool CRenameFileCmd::Unexecute( void )
{
	return CRenameFileCmd( m_destPath, m_srcPath ).Execute();
}

bool CRenameFileCmd::IsUndoable( void ) const
{
	return
		m_destPath.FileExist() &&
		!m_srcPath.FileExist();
}


// CTouchFileCmd implementation

CTouchFileCmd::CTouchFileCmd( const fs::CFileState& srcState, const fs::CFileState& destState )
	: CFileCmd( cmd::TouchFile, srcState.m_fullPath )
	, m_srcState( srcState )
	, m_destState( destState )
{
	ASSERT( m_srcState.m_fullPath == m_destState.m_fullPath );
}

CTouchFileCmd::~CTouchFileCmd()
{
}

std::tstring CTouchFileCmd::Format( bool detailed ) const
{
	std::tstring text;
	if ( detailed )
		text = CFileCmd::Format( false );		// prepend "TOUCH" tag

	stream::Tag( text, fmt::FormatTouchEntry( m_srcState, m_destState ), _T(" ") );
	return text;
}

bool CTouchFileCmd::Execute( void )
{
	if ( m_destState != m_srcState )
	{
		m_destState.WriteToFile();
		NotifyObservers();
	}
	return true;
}

bool CTouchFileCmd::Unexecute( void )
{
	return CTouchFileCmd( m_destState, m_srcState ).Execute();
}

bool CTouchFileCmd::IsUndoable( void ) const
{
	return
		m_srcState.FileExist() &&
		m_srcState != fs::CFileState::ReadFromFile( m_srcState.m_fullPath );
}
