
#include "stdafx.h"
#include "FileGroupCommands.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
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

	const TCHAR CBaseFileGroupCmd::s_lineEnd[] = _T("\n");

	CBaseFileGroupCmd::CBaseFileGroupCmd( CommandType cmdType /*= CommandType()*/, const std::vector< fs::CPath >& filePaths /*= std::vector< fs::CPath >()*/,
										  const CTime& timestamp /*= CTime::GetCurrentTime()*/ )
		: CBaseSerialCmd( cmdType )
		, m_filePaths( filePaths )
		, m_timestamp( timestamp )
	{
	}

	bool CBaseFileGroupCmd::IsValid( void ) const
	{
		return true;
	}

	const CTime& CBaseFileGroupCmd::GetTimestamp( void ) const
	{
		return m_timestamp;
	}

	size_t CBaseFileGroupCmd::GetFileCount( void ) const
	{
		return m_filePaths.size();
	}

	void CBaseFileGroupCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const
	{
		utl::Assign( rLines, m_filePaths, func::tor::StringOf() );
	}

	std::tstring CBaseFileGroupCmd::Format( utl::Verbosity verbosity ) const
	{
		static const TCHAR s_space[] = _T(" ");
		std::tstring text = GetTags_CommandType().Format( GetTypeID(), verbosity != utl::Brief ? CEnumTags::UiTag : CEnumTags::KeyTag );

		if ( verbosity != utl::Brief )
			stream::Tag( text, str::Format( _T("[%d]"), m_filePaths.size() ), s_space );
		if ( utl::DetailedLine == verbosity )
			stream::Tag( text, GetDestHeaderInfo(), s_space );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, verbosity != utl::Brief ? time_utl::s_outFormatUi : time_utl::s_outFormat ), s_space );

		return text;
	}

	void CBaseFileGroupCmd::Serialize( CArchive& archive )
	{
		__super::Serialize( archive );

		archive & m_timestamp;
		serial::SerializeValues( archive, m_filePaths );
	}

	std::tstring CBaseFileGroupCmd::GetDestHeaderInfo( void ) const
	{
		return std::tstring();
	}

	void CBaseFileGroupCmd::QueryFilePairLines( std::vector< std::tstring >& rLines, const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths )
	{
		ASSERT( srcFilePaths.size() == destFilePaths.size() );

		rLines.clear();
		rLines.reserve( srcFilePaths.size() );

		for ( size_t i = 0; i != srcFilePaths.size(); ++i )
			rLines.push_back( fmt::FormatRenameEntry( srcFilePaths[ i ], destFilePaths[ i ] ) );
	}

	bool CBaseFileGroupCmd::HandleExecuteResult( const CWorkingSet& workingSet, const std::tstring& groupDetails )
	{
		if ( s_pErrorObserver != NULL )
			for ( std::vector< fs::CPath >::const_iterator itBadFilePath = workingSet.m_badFilePaths.begin(); itBadFilePath != workingSet.m_badFilePaths.end(); ++itBadFilePath )
				s_pErrorObserver->OnFileError( *itBadFilePath, str::Format( _T("Cannot access file: %s"), itBadFilePath->GetPtr() ) );

		if ( s_pLogger != NULL )
		{
			std::tstring message = Format( utl::Detailed );

			if ( !workingSet.m_succeeded )
				stream::Tag( message, _T(" * ERROR"), s_lineEnd );
			stream::Tag( message, groupDetails, s_lineEnd );

			if ( !workingSet.m_succeeded )
				message += _T("\n ERROR");
			else if ( !workingSet.m_badFilePaths.empty() )
			{
				stream::Tag( message, str::Format( _T(" WARNING: Cannot access %d files:"), workingSet.m_badFilePaths.size() ), s_lineEnd );
				stream::Tag( message, str::Join( workingSet.m_badFilePaths, s_lineEnd ), s_lineEnd );
			}
			s_pLogger->LogString( message );
		}

		if ( !workingSet.m_succeeded )
			return false;

		NotifyObservers();
		return true;
	}


	// CBaseFileGroupCmd::CWorkingSet implementation

	CBaseFileGroupCmd::CWorkingSet::CWorkingSet( const CBaseFileGroupCmd& cmd, fs::AccessMode accessMode /*= fs::Read*/ )
		: m_existStatus( AllExist )
		, m_succeeded( false )
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

	if ( workingSet.IsValid() )
		if ( shell::DeleteFiles( workingSet.m_currFilePaths, app::GetMainWnd() ) )		// delete to RecycleBin
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			return false;			// silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CDeleteFilesCmd::Unexecute( void )
{
	CUndeleteFilesCmd undoCmd( GetFilePaths() );
	undoCmd.CopyTimestampOf( *this );
	if ( undoCmd.Execute() )
	{
		AfxMessageBox( undoCmd.Format( utl::Detailed ).c_str() );		// notify user that command was undone (editor-less command)
		return true;
	}

	return false;
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

	CWorkingSet workingSet( *this );
	workingSet.m_badFilePaths.swap( errorFilePaths );
	workingSet.m_succeeded = restoredCount != 0;

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}


// CMoveFilesCmd implementation

IMPLEMENT_SERIAL( CMoveFilesCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CMoveFilesCmd::CMoveFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath )
	: cmd::CBaseFileGroupCmd( cmd::MoveFiles, srcFilePaths )
	, m_srcCommonDirPath( path::ExtractCommonParentPath( srcFilePaths ) )
	, m_destDirPath( destDirPath )
{
}

CMoveFilesCmd::~CMoveFilesCmd()
{
}

void CMoveFilesCmd::Serialize( CArchive& archive )
{
	__super::Serialize( archive );

	archive & m_destDirPath;
	archive & m_srcCommonDirPath;
}

void CMoveFilesCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const
{
	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, GetSrcFilePaths() );

	QueryFilePairLines( rLines, GetSrcFilePaths(), destFilePaths );
}

bool CMoveFilesCmd::Execute( void )
{
	CWorkingSet workingSet( *this, fs::ReadWrite );

	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, workingSet.m_currFilePaths );

	if ( workingSet.IsValid() )
		if ( shell::MoveFiles( workingSet.m_currFilePaths, destFilePaths, app::GetMainWnd() ) )		// undoable move files
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			return false;			// silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CMoveFilesCmd::Unexecute( void )
{
	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, GetSrcFilePaths() );

	CMoveFilesCmd undoCmd( destFilePaths, m_srcCommonDirPath );			// move back destination files to source common directory
	undoCmd.CopyTimestampOf( *this );
	if ( undoCmd.Execute() )
	{
		AfxMessageBox( undoCmd.Format( utl::Detailed ).c_str() );		// notify user that command was undone (editor-less command)
		return true;
	}

	return false;
}

bool CMoveFilesCmd::IsUndoable( void ) const
{
	return true;		// let it unexecute with error rather than being skipped in UNDO
}

std::tstring CMoveFilesCmd::GetDestHeaderInfo( void ) const
{
	return str::Format( _T("-> %s"), m_destDirPath.GetPtr() );
}

void CMoveFilesCmd::MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const
{
	rDestFilePaths.clear();
	rDestFilePaths.reserve( srcFilePaths.size() );

	for ( std::vector< fs::CPath >::const_iterator itSrcPath = srcFilePaths.begin(); itSrcPath != srcFilePaths.end(); ++itSrcPath )
		rDestFilePaths.push_back( MakeDeepDestFilePath( *itSrcPath ) );
}

fs::CPath CMoveFilesCmd::MakeDeepDestFilePath( const fs::CPath& srcFilePath ) const
{
	fs::CPath srcParentDirPath = srcFilePath.GetParentPath();
	std::tstring destRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), m_srcCommonDirPath.GetPtr() );

	fs::CPath targetFullPath = m_destDirPath / destRelPath / srcFilePath.GetFilename();
	return targetFullPath;
}
