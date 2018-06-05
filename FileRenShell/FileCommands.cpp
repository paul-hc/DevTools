
#include "stdafx.h"
#include "FileCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/TimeUtils.h"

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


	// CUserFeedbackException implementation

	CUserFeedbackException::CUserFeedbackException( UserFeedback feedback )
		: CRuntimeException( GetTags_Feedback().FormatUi( feedback ) )
		, m_feedback( feedback )
	{
	}

	const CEnumTags& CUserFeedbackException::GetTags_Feedback( void )
	{
		static const CEnumTags tags( _T("Abort|Retry|Ignore") );
		return tags;
	}


	// CFileCmd implementation

	const TCHAR CFileCmd::s_fmtError[] = _T("* %s\n ERROR: %s");
	IErrorObserver* CFileCmd::s_pErrorObserver = NULL;
	CLogger* CFileCmd::s_pLogger = (CLogger*)-1;

	CFileCmd::CFileCmd( Command command, const fs::CPath& srcPath )
		: CCommand( command, NULL, &cmd::GetTags_Command() )
		, m_srcPath( srcPath )
	{
		if ( (CLogger*)-1 == s_pLogger )
			s_pLogger = &app::GetLogger();
	}

	void CFileCmd::ExecuteHandle( void ) throws_( CUserFeedbackException )
	{
		try
		{
			Execute();

			if ( s_pLogger != NULL )
				s_pLogger->LogString( Format( true ) );
		}
		catch ( CException* pExc )
		{
			throw CUserFeedbackException( HandleFileError( pExc ) );
		}
	}

	bool CFileCmd::Unexecute( void )
	{
		std::auto_ptr< CFileCmd > pUndoCmd( MakeUnexecuteCmd() );

		return pUndoCmd.get() != NULL && pUndoCmd->Execute();
	}

	UserFeedback CFileCmd::HandleFileError( CException* pExc ) const
	{
		std::tstring errMsg = ExtractMessage( pExc );

		if ( s_pErrorObserver != NULL )
			s_pErrorObserver->OnFileError( m_srcPath, errMsg );

		UserFeedback feedback;
		switch ( AfxMessageBox( errMsg.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
		{
			default: ASSERT( false );
			case IDRETRY:	return Retry;				// no logging on retry, give it another chance to fix the problem
			case IDABORT:	feedback = Abort; break;
			case IDIGNORE:	feedback = Ignore; break;
		}

		if ( s_pLogger != NULL )
			s_pLogger->Log( s_fmtError, Format( true ).c_str(), errMsg.c_str() );
		return feedback;
	}

	std::tstring CFileCmd::ExtractMessage( CException* pExc ) const
	{
		std::tstring message = ui::GetErrorMessage( pExc );
		pExc->Delete();

		if ( !m_srcPath.FileExist() )
			return str::Format( _T("Cannot find file: %s"), m_srcPath.GetPtr() );
		return message;
	}


	// CFileMacroCmd implementation

	CFileMacroCmd::CFileMacroCmd( Command subCmdType, const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CMacroCommand( GetTags_Command().FormatKey( subCmdType ), subCmdType )
		, m_timestamp( timestamp )
	{
	}

	std::tstring CFileMacroCmd::Format( bool detailed ) const
	{
		std::tstring text = GetTags_Command().Format( GetTypeID(), detailed ? CEnumTags::UiTag : CEnumTags::KeyTag );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, detailed ? time_utl::s_outFormatUi : time_utl::s_outFormat ), _T(" ") );

		return text;
	}

	bool CFileMacroCmd::Execute( void )
	{
		ExecuteMacro( ExecuteMode );			// removes commands that failed
		return !m_subCommands.empty();			// any succeeeded?
	}

	bool CFileMacroCmd::Unexecute( void )
	{
		ExecuteMacro( UnexecuteMode );			// removes commands that failed
		return !m_subCommands.empty();			// any succeeeded?
	}

	void CFileMacroCmd::ExecuteMacro( Mode mode )
	{
		for ( std::vector< utl::ICommand* >::iterator itCmd = m_subCommands.begin(); itCmd != m_subCommands.end(); )
		{
			CFileCmd* pCmd = checked_static_cast< CFileCmd* >( *itCmd );

			try
			{
				if ( ExecuteMode == mode )
					pCmd->ExecuteHandle();
				else
					pCmd->MakeUnexecuteCmd()->ExecuteHandle();
			}
			catch ( CUserFeedbackException& exc )
			{
				switch ( exc.m_feedback )
				{
					case Retry:
						continue;
					case Ignore:
						// discard command since cannot unexecute
						delete *itCmd;
						itCmd = m_subCommands.erase( itCmd );
						continue;
					case Abort:
						// discard remaining commands since will not be unexecuted
						std::for_each( itCmd, m_subCommands.end(), func::Delete() );
						itCmd = m_subCommands.erase( itCmd, m_subCommands.end() );
						return;
				}
			}

			++itCmd;
		}
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

std::auto_ptr< cmd::CFileCmd > CRenameFileCmd::MakeUnexecuteCmd( void ) const
{
	return std::auto_ptr< cmd::CFileCmd >( new CRenameFileCmd( m_destPath, m_srcPath ) );
}

bool CRenameFileCmd::IsUndoable( void ) const
{
	//return m_destPath.FileExist() && !m_srcPath.FileExist();
	return true;		// let it unexecute with error rather than being skipped in UNDO
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

std::auto_ptr< cmd::CFileCmd > CTouchFileCmd::MakeUnexecuteCmd( void ) const
{
	return std::auto_ptr< cmd::CFileCmd >( new CTouchFileCmd( m_destState, m_srcState ) );
}

bool CTouchFileCmd::IsUndoable( void ) const
{
	//return m_srcState.FileExist() && m_srcState != fs::CFileState::ReadFromFile( m_srcState.m_fullPath );
	return true;		// let it unexecute with error rather than being skipped in UNDO
}
