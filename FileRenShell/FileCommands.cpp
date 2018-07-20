
#include "stdafx.h"
#include "FileCommands.h"
#include "FileModel.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/TimeUtils.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	const CEnumTags& GetTags_CommandType( void )
	{
		static const CEnumTags tags(
			_T("Rename Files|Touch Files|Change Destination Paths|Change Destination File States|Reset Destinations|Edit"),
			_T("RENAME|TOUCH|CHANGE_DEST_PATHS|CHANGE_DEST_FILE_STATES|RESET_DESTINATIONS|EDIT"),
			-1, RenameFile );

		return tags;
	}

	const CEnumTags& GetTags_StackType( void )
	{
		static const CEnumTags tags( _T("Undo|Redo") );
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


	// CBaseSerialCmd implementation

	IMPLEMENT_DYNAMIC( CBaseSerialCmd, CObject )

	CBaseSerialCmd::CBaseSerialCmd( CommandType cmdType /*= CommandType()*/ )
		: CCommand( cmdType, NULL, &GetTags_CommandType() )
	{
	}

	void CBaseSerialCmd::Serialize( CArchive& archive )
	{
		CCommand::Serialize( archive );		// dis-ambiguate from CObject::Serialize()
	}


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


	// CBaseFileCmd implementation

	const TCHAR CBaseFileCmd::s_fmtError[] = _T("* %s\n ERROR: %s");
	IErrorObserver* CBaseFileCmd::s_pErrorObserver = NULL;
	CLogger* CBaseFileCmd::s_pLogger = (CLogger*)-1;

	CBaseFileCmd::CBaseFileCmd( CommandType cmdType /*= CommandType()*/, const fs::CPath& srcPath /*= fs::CPath()*/ )
		: CBaseSerialCmd( cmdType )
		, m_srcPath( srcPath )
	{
		if ( (CLogger*)-1 == s_pLogger )
			s_pLogger = &app::GetLogger();
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
			throw CUserFeedbackException( HandleFileError( pExc ) );
		}
	}

	bool CBaseFileCmd::Unexecute( void )
	{
		std::auto_ptr< CBaseFileCmd > pUndoCmd( MakeUnexecuteCmd() );

		return pUndoCmd.get() != NULL && pUndoCmd->Execute();
	}

	UserFeedback CBaseFileCmd::HandleFileError( CException* pExc ) const
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
			s_pLogger->Log( s_fmtError, Format( utl::Detailed ).c_str(), errMsg.c_str() );
		return feedback;
	}

	std::tstring CBaseFileCmd::ExtractMessage( CException* pExc ) const
	{
		std::tstring message = ui::GetErrorMessage( pExc );
		pExc->Delete();

		if ( !m_srcPath.FileExist() )
			return str::Format( _T("Cannot find file: %s"), m_srcPath.GetPtr() );
		return message;
	}


	// CFileMacroCmd implementation

	IMPLEMENT_SERIAL( CFileMacroCmd, CObject, VERSIONABLE_SCHEMA | 1 )

	CFileMacroCmd::CFileMacroCmd( CommandType subCmdType, const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CMacroCommand( GetTags_CommandType().FormatKey( subCmdType ), subCmdType )
		, m_timestamp( timestamp )
	{
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


// CBaseChangeDestCmd implementation

CBaseChangeDestCmd::CBaseChangeDestCmd( cmd::CommandType cmdType, CFileModel* pFileModel, const std::tstring& cmdTag )
	: CCommand( cmdType, pFileModel, &cmd::GetTags_CommandType() )
	, m_cmdTag( cmdTag )
	, m_pFileModel( pFileModel )
	, m_hasOldDests( false )
{
	SetSubject( m_pFileModel );
}

std::tstring CBaseChangeDestCmd::Format( utl::Verbosity verbosity ) const
{
	if ( !m_cmdTag.empty() )
		return m_cmdTag;

	return __super::Format( verbosity );
}

bool CBaseChangeDestCmd::Execute( void )
{
	return ToggleExecute();
}

bool CBaseChangeDestCmd::Unexecute( void )
{
	// Since Execute() toggles between m_destPaths and OldDestPaths, calling the second time has the effect of Unexecute().
	// This assumes that this command is always executed through the command model for UNDO/REDO.
	REQUIRE( m_hasOldDests );
	return ToggleExecute();
}

bool CBaseChangeDestCmd::IsUndoable( void ) const
{
	return Changed == EvalChange();
}


// CChangeDestPathsCmd implementation

CChangeDestPathsCmd::CChangeDestPathsCmd( CFileModel* pFileModel, std::vector< fs::CPath >& rNewDestPaths, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestPaths, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetRenameItems().size() == rNewDestPaths.size() );

	m_srcPaths.reserve( m_pFileModel->GetRenameItems().size() );
	for ( std::vector< CRenameItem* >::const_iterator itItem = m_pFileModel->GetRenameItems().begin(); itItem != m_pFileModel->GetRenameItems().end(); ++itItem )
		m_srcPaths.push_back( ( *itItem )->GetSrcPath() );

	m_destPaths.swap( rNewDestPaths );
	ENSURE( m_srcPaths.size() == m_destPaths.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestPathsCmd::EvalChange( void ) const
{
	REQUIRE( m_srcPaths.size() == m_destPaths.size() );

	if ( m_destPaths.size() != m_pFileModel->GetRenameItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
	{
		const CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[ i ];

		if ( pRenameItem->GetKeyPath() != m_srcPaths[ i ] )						// keys different?
			return Expired;
		else if ( pRenameItem->GetDestPath().Get() != m_destPaths[ i ].Get() )	// case sensitive string compare
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestPathsCmd::ToggleExecute( void )
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
			{
				CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[ i ];

				std::swap( pRenameItem->RefDestPath(), m_destPaths[ i ] );		// from now on m_destPaths stores OldDestPaths
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}


// CChangeDestFileStatesCmd implementation

CChangeDestFileStatesCmd::CChangeDestFileStatesCmd( CFileModel* pFileModel, std::vector< fs::CFileState >& rNewDestStates, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestFileStates, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetTouchItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetTouchItems().size() == rNewDestStates.size() );

	m_srcStates.reserve( m_pFileModel->GetTouchItems().size() );
	for ( std::vector< CTouchItem* >::const_iterator itItem = m_pFileModel->GetTouchItems().begin(); itItem != m_pFileModel->GetTouchItems().end(); ++itItem )
		m_srcStates.push_back( ( *itItem )->GetSrcState() );

	m_destStates.swap( rNewDestStates );
	ENSURE( m_srcStates.size() == m_destStates.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestFileStatesCmd::EvalChange( void ) const
{
	REQUIRE( m_srcStates.size() == m_destStates.size() );

	if ( m_destStates.size() != m_pFileModel->GetTouchItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
	{
		const CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[ i ];

		if ( pTouchItem->GetKeyPath() != m_srcStates[ i ].m_fullPath )			// keys different?
			return Expired;
		else if ( pTouchItem->GetDestState() != m_destStates[ i ] )
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestFileStatesCmd::ToggleExecute( void )
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
			{
				CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[ i ];

				std::swap( pTouchItem->RefDestState(), m_destStates[ i ] );		// from now on m_destStates stores OldDestStates
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}


// CResetDestinationsCmd implementation

CResetDestinationsCmd::CResetDestinationsCmd( CFileModel* pFileModel )
	: CMacroCommand( _T("Reset"), cmd::ResetDestinations )
{
	if ( !pFileModel->GetRenameItems().empty() )		// lazy initialized?
	{
		std::vector< fs::CPath > emptyDestPaths( pFileModel->GetRenameItems().size() );
		AddCmd( new CChangeDestPathsCmd( pFileModel, emptyDestPaths, m_userInfo ) );
	}

	if ( !pFileModel->GetTouchItems().empty() )			// lazy initialized?
	{
		std::vector< fs::CFileState > emptyDestStates; emptyDestStates.reserve( pFileModel->GetTouchItems().size() );
		for ( std::vector< CTouchItem* >::const_iterator itTouchItem = pFileModel->GetTouchItems().begin(); itTouchItem != pFileModel->GetTouchItems().end(); ++itTouchItem )
			emptyDestStates.push_back( ( *itTouchItem )->GetSrcState() );

		AddCmd( new CChangeDestFileStatesCmd( pFileModel, emptyDestStates, m_userInfo ) );
	}
}

std::tstring CResetDestinationsCmd::Format( utl::Verbosity verbosity ) const
{
	return GetSubCommands().front()->Format( verbosity );
}
