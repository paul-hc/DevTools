
#include "stdafx.h"
#include "FileMacroCommands.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/Logger.h"
#include "utl/TimeUtils.h"
#include "utl/SerializeStdTypes.h"

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
		static const CEnumTags tags( _T("Abort|Retry|Ignore") );
		return tags;
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

	bool CFileMacroCmd::IsValid( void ) const
	{
		return !IsEmpty();
	}

	const CTime& CFileMacroCmd::GetTimestamp( void ) const
	{
		return m_timestamp;
	}

	size_t CFileMacroCmd::GetFileCount( void ) const
	{
		return GetSubCommands().size();
	}

	void CFileMacroCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const
	{
		rLines.clear();
		rLines.reserve( m_subCommands.size() );

		for ( std::vector< utl::ICommand* >::const_iterator itCmd = m_subCommands.begin(); itCmd != m_subCommands.end(); ++itCmd )
		{
			CBaseFileCmd* pCmd = checked_static_cast< CBaseFileCmd* >( *itCmd );
			rLines.push_back( pCmd->Format( utl::Detailed ) );
		}
	}

	std::tstring CFileMacroCmd::Format( utl::Verbosity verbosity ) const
	{
		std::tstring text = GetTags_CommandType().Format( GetTypeID(), verbosity != utl::Brief ? CEnumTags::UiTag : CEnumTags::KeyTag );

		if ( verbosity != utl::Brief )
			stream::Tag( text, str::Format( _T("[%d]"), GetSubCommands().size() ), _T(" ") );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, verbosity != utl::Brief ? time_utl::s_outFormatUi : time_utl::s_outFormat ), _T(" ") );

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
			CBaseFileCmd* pCmd = checked_static_cast< CBaseFileCmd* >( *itCmd );

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

	void CFileMacroCmd::Serialize( CArchive& archive )
	{
		CMacroCommand::Serialize( archive );

		archive & m_timestamp;
	}


	// CBaseFileCmd implementation

	const TCHAR CBaseFileCmd::s_fmtError[] = _T("* %s\n ERROR: %s");

	CBaseFileCmd::CBaseFileCmd( CommandType cmdType /*= CommandType()*/, const fs::CPath& srcPath /*= fs::CPath()*/ )
		: CBaseSerialCmd( cmdType )
		, m_srcPath( srcPath )
	{
	}

	void CBaseFileCmd::ExecuteHandle( void ) throws_( CUserFeedbackException )
	{
		try
		{
			Execute();

			if ( s_pLogger != NULL )
				s_pLogger->LogString( Format( utl::Detailed ) );
		}
		catch ( CException* pExc )
		{
			throw CUserFeedbackException( HandleFileError( pExc, m_srcPath ) );
		}
	}

	bool CBaseFileCmd::Unexecute( void )
	{
		std::auto_ptr< CBaseFileCmd > pUndoCmd( MakeUnexecuteCmd() );

		return pUndoCmd.get() != NULL && pUndoCmd->Execute();
	}

	UserFeedback CBaseFileCmd::HandleFileError( CException* pExc, const fs::CPath& srcPath ) const
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

		if ( s_pLogger != NULL )
			s_pLogger->Log( s_fmtError, Format( utl::Detailed ).c_str(), errMsg.c_str() );
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

std::tstring CRenameFileCmd::Format( utl::Verbosity verbosity ) const
{
	std::tstring text;
	if ( verbosity != utl::Brief )
		text = CBaseFileCmd::Format( utl::Brief );		// prepend "RENAME" tag

	stream::Tag( text, fmt::FormatRenameEntry( m_srcPath, m_destPath ), _T(" ") );
	return text;
}

bool CRenameFileCmd::Execute( void )
{
	CFile::Rename( m_srcPath.GetPtr(), m_destPath.GetPtr() );
	NotifyObservers();
	return true;
}

std::auto_ptr< cmd::CBaseFileCmd > CRenameFileCmd::MakeUnexecuteCmd( void ) const
{
	return std::auto_ptr< cmd::CBaseFileCmd >( new CRenameFileCmd( m_destPath, m_srcPath ) );
}

bool CRenameFileCmd::IsUndoable( void ) const
{
	//return m_destPath.FileExist() && !m_srcPath.FileExist();
	return true;		// let it unexecute with error rather than being skipped in UNDO
}

void CRenameFileCmd::Serialize( CArchive& archive )
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

std::tstring CTouchFileCmd::Format( utl::Verbosity verbosity ) const
{
	std::tstring text;
	if ( verbosity != utl::Brief )
		text = CBaseFileCmd::Format( utl::Brief );		// prepend "TOUCH" tag

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

std::auto_ptr< cmd::CBaseFileCmd > CTouchFileCmd::MakeUnexecuteCmd( void ) const
{
	return std::auto_ptr< cmd::CBaseFileCmd >( new CTouchFileCmd( m_destState, m_srcState ) );
}

bool CTouchFileCmd::IsUndoable( void ) const
{
	//return m_srcState.FileExist() && m_srcState != fs::CFileState::ReadFromFile( m_srcState.m_fullPath );
	return true;		// let it unexecute with error rather than being skipped in UNDO
}

void CTouchFileCmd::Serialize( CArchive& archive )
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
