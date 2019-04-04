
#include "stdafx.h"
#include "FileGroupCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/Logger.h"
#include "utl/SerializeStdTypes.h"
#include "utl/TimeUtils.h"
#include "utl/UI/ShellUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace cmd
{
	// CBaseFileGroupCmd implementation

	CBaseFileGroupCmd::CBaseFileGroupCmd( CommandType cmdType /*= CommandType()*/, const std::vector< fs::CPath >& filePaths /*= std::vector< fs::CPath >()*/,
										  const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CBaseSerialCmd( cmdType )
		, m_filePaths( filePaths )
		, m_timestamp( timestamp )
	{
	}

	std::tstring CBaseFileGroupCmd::Format( utl::Verbosity verbosity ) const
	{
		std::tstring text = GetTags_CommandType().Format( GetTypeID(), verbosity != utl::Brief ? CEnumTags::UiTag : CEnumTags::KeyTag );

		if ( verbosity != utl::Brief )
			stream::Tag( text, str::Format( _T("[%d]"), m_filePaths.size() ), _T(" ") );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, verbosity != utl::Brief ? time_utl::s_outFormatUi : time_utl::s_outFormat ), _T(" ") );

		return text;
	}

	void CBaseFileGroupCmd::Serialize( CArchive& archive )
	{
		__super::Serialize( archive );

		archive & m_timestamp;
		serial::SerializeValues( archive, m_filePaths );
	}

	void CBaseFileGroupCmd::HandleExecuteResult( bool succeeded, const CWorkingSet& workingSet, const std::tstring& details /*= str::GetEmpty()*/ ) const
	{
		if ( s_pErrorObserver != NULL )
			for ( std::vector< fs::CPath >::const_iterator itBadFilePath = workingSet.m_badFilePaths.begin(); itBadFilePath != workingSet.m_badFilePaths.end(); ++itBadFilePath )
				s_pErrorObserver->OnFileError( *itBadFilePath, str::Format( _T("Cannot access file: %s"), itBadFilePath->GetPtr() ) );

		if ( s_pLogger != NULL )
		{
			std::tstring message = Format( utl::Detailed );
			stream::Tag( message, details, _T(" ") );

			if ( !succeeded )
				message += _T("\n ERROR");
			else if ( !workingSet.m_badFilePaths.empty() )
			{
				message += str::Format( _T("\n WARNING: Cannot access %d files:\n"), workingSet.m_badFilePaths.size() );
				message += str::Join( workingSet.m_badFilePaths, _T("\n") );
			}
			s_pLogger->LogString( message );
		}
	}


	// CBaseFileGroupCmd::CWorkingSet implementation

	CBaseFileGroupCmd::CWorkingSet::CWorkingSet( const CBaseFileGroupCmd& cmd, fs::AccessMode accessMode /*= fs::Read*/ )
		: m_existStatus( AllExist )
	{
		const std::vector< fs::CPath >& filePaths = cmd.GetFilePaths();

		ASSERT( !filePaths.empty() );

		m_currFilePaths.reserve( filePaths.size() );

		for ( std::vector< fs::CPath >::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
			if ( itFilePath->FileExist( accessMode ) )
				m_currFilePaths.push_back( *itFilePath );
			else
				m_badFilePaths.push_back( *itFilePath );

		if ( m_currFilePaths.size() != filePaths.size() )
			m_existStatus = !m_currFilePaths.empty() ? SomeExist : NoneExist;
	}
}


// CDeleteFilesCmd implementation

IMPLEMENT_SERIAL( CDeleteFilesCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CDeleteFilesCmd::CDeleteFilesCmd( const std::vector< fs::CPath >& filePaths )
	: cmd::CBaseFileGroupCmd( cmd::DeleteFiles, filePaths )
{
}

CDeleteFilesCmd::~CDeleteFilesCmd()
{
}

bool CDeleteFilesCmd::Execute( void )
{
	CWorkingSet workingSet( *this, fs::Write );

	if ( !shell::DeleteFiles( workingSet.m_currFilePaths, app::GetMainWnd() ) )		// delete to RecycleBin
	{
		if ( !shell::AnyOperationAborted() )
			HandleExecuteResult( false, workingSet );

		return false;
	}

	HandleExecuteResult( true, workingSet );
	NotifyObservers();
	return true;
}

bool CDeleteFilesCmd::Unexecute( void )
{
	CUndeleteFilesCmd undoCmd( GetFilePaths() );
	return undoCmd.Execute();
}

bool CDeleteFilesCmd::IsUndoable( void ) const
{
	return true;		// let it unexecute with error rather than being skipped in UNDO
}


// CDeleteFilesCmd::CUndeleteFilesCmd implementation

bool CDeleteFilesCmd::CUndeleteFilesCmd::Execute( void )
{
	std::vector< fs::CPath > errorFilePaths;
	size_t restoredCount = shell::UndeleteFiles( GetFilePaths(), app::GetMainWnd(), &errorFilePaths );
	bool succeeded = restoredCount != 0;

	CWorkingSet workingSet( *this );
	workingSet.m_badFilePaths.swap( errorFilePaths );

	HandleExecuteResult( succeeded, workingSet );
	if ( succeeded )
		NotifyObservers();
	return succeeded;
}
