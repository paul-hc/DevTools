
#include "stdafx.h"
#include "FileGroupCommands.h"
#include "Application.h"
#include "utl/AppTools.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/FileContent.h"
#include "utl/FileSystem.h"
#include "utl/Logger.h"
#include "utl/RuntimeException.h"
#include "utl/SerializeStdTypes.h"
#include "utl/TimeUtils.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"

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
		, m_pParentOwner( app::GetMainWnd() )
		, m_fileAccessMode( fs::Exist )
		, m_opFlags( FOF_ALLOWUNDO )
	{
	}

	bool CBaseFileGroupCmd::IsValid( void ) const override
	{
		return true;
	}

	const CTime& CBaseFileGroupCmd::GetTimestamp( void ) const override
	{
		return m_timestamp;
	}

	size_t CBaseFileGroupCmd::GetFileCount( void ) const override
	{
		return m_filePaths.size();
	}

	void CBaseFileGroupCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const override
	{
		utl::Assign( rLines, m_filePaths, func::tor::StringOf() );
	}

	std::tstring CBaseFileGroupCmd::Format( utl::Verbosity verbosity ) const override
	{
		std::tstring text = FormatCmdTag( this, verbosity );
		const TCHAR* pSep = GetSeparator( verbosity );

		if ( verbosity != utl::Brief )
			stream::Tag( text, str::Format( _T("[%d]"), m_filePaths.size() ), pSep );

		if ( utl::DetailFields == verbosity )
			stream::Tag( text, GetDestHeaderInfo(), pSep );

		if ( m_timestamp.GetTime() != 0 )
			stream::Tag( text, time_utl::FormatTimestamp( m_timestamp, verbosity != utl::Brief ? time_utl::s_outFormatUi : time_utl::s_outFormat ), pSep );

		return text;
	}

	void CBaseFileGroupCmd::Serialize( CArchive& archive ) override
	{
		__super::Serialize( archive );

		archive & m_timestamp;
		serial::SerializeValues( archive, m_filePaths );
	}

	bool CBaseFileGroupCmd::IsUndoable( void ) const override
	{
		return true;		// let it unexecute with error rather than being skipped in UNDO
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
		LogExecution( message );

		DWORD infoFlag = workingSet.m_succeeded ? NIIF_INFO : NIIF_ERROR;
		if ( workingSet.m_succeeded && SomeExist == workingSet.m_existStatus )
			infoFlag = NIIF_WARNING;
		UINT timeoutSecs = workingSet.m_succeeded && AllExist == workingSet.m_existStatus ? 10 : 30;

		ui::sys_tray::ShowBalloonTip( message.c_str(), _T("Shell Goodies"), infoFlag, timeoutSecs );

		if ( !workingSet.m_succeeded )
			return false;

		NotifyObservers();
		return true;
	}


	// CBaseFileGroupCmd::CWorkingSet implementation

	CBaseFileGroupCmd::CWorkingSet::CWorkingSet( const CBaseFileGroupCmd* pCmd, fs::AccessMode accessMode /*= fs::Read*/ )
		: m_existStatus( AllExist )
		, m_succeeded( false )
	{
		ASSERT_PTR( pCmd );
		const std::vector< fs::CPath >& filePaths = pCmd->GetFilePaths();

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

	CBaseFileGroupCmd::CWorkingSet::CWorkingSet( const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths,
												 fs::AccessMode accessMode /*= fs::Read*/ )
		: m_existStatus( AllExist )
		, m_succeeded( false )
	{
		REQUIRE( !srcFilePaths.empty() && srcFilePaths.size() == destFilePaths.size() );

		m_currFilePaths.reserve( srcFilePaths.size() );
		m_currDestFilePaths.reserve( destFilePaths.size() );

		for ( size_t i = 0; i != srcFilePaths.size(); ++i )
		{
			const fs::CPath& srcFilePath = srcFilePaths[ i ];

			if ( srcFilePath.FileExist( accessMode ) )
			{
				m_currFilePaths.push_back( srcFilePath );
				m_currDestFilePaths.push_back( destFilePaths[ i ] );
			}
			else
				m_badFilePaths.push_back( srcFilePath );
		}

		if ( m_currFilePaths.size() != srcFilePaths.size() )
			m_existStatus = !m_currFilePaths.empty() ? SomeExist : NoneExist;
	}

	bool CBaseFileGroupCmd::CWorkingSet::IsBadFilePath( const fs::CPath& filePath ) const
	{
		return !utl::Contains( m_badFilePaths, filePath );
	}


	// CBaseDeepTransferFilesCmd implementation

	CBaseDeepTransferFilesCmd::CBaseDeepTransferFilesCmd( CommandType cmdType, const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath )
		: cmd::CBaseFileGroupCmd( cmdType, srcFilePaths )
		, m_destDirPath( destDirPath )
		, m_srcCommonDirPath( path::ExtractCommonParentPath( srcFilePaths ) )
		, m_topDestDirPath( m_destDirPath )
	{
	}

	void CBaseDeepTransferFilesCmd::SetDeepRelDirPath( const fs::TDirPath& deepRelSubfolderPath )
	{
		REQUIRE( path::IsRelative( deepRelSubfolderPath.GetPtr() ) );

		m_topDestDirPath = m_destDirPath;
		m_destDirPath /= deepRelSubfolderPath;		// move dest folder deeper
	}

	bool CBaseDeepTransferFilesCmd::QueryDestFilePaths( std::vector< fs::CPath >& rDestFilePaths ) const
	{
		CWorkingSet workingSet( this, m_fileAccessMode );

		MakeDestFilePaths( rDestFilePaths, workingSet.m_currFilePaths );
		return workingSet.IsValid();
	}

	void CBaseDeepTransferFilesCmd::Serialize( CArchive& archive ) override
	{
		__super::Serialize( archive );

		// keep the order for backwards compatibility
		archive & m_srcCommonDirPath;
		archive & m_destDirPath;
		archive & m_topDestDirPath;
	}

	void CBaseDeepTransferFilesCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const override
	{
		std::vector< fs::CPath > destFilePaths;
		MakeDestFilePaths( destFilePaths, GetSrcFilePaths() );

		QueryFilePairLines( rLines, GetSrcFilePaths(), destFilePaths );
	}

	std::tstring CBaseDeepTransferFilesCmd::GetDestHeaderInfo( void ) const override
	{
		return str::Format( _T("-> %s"), m_destDirPath.GetPtr() );
	}

	void CBaseDeepTransferFilesCmd::MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const
	{
		rDestFilePaths.clear();
		rDestFilePaths.reserve( srcFilePaths.size() );

		for ( std::vector< fs::CPath >::const_iterator itSrcPath = srcFilePaths.begin(); itSrcPath != srcFilePaths.end(); ++itSrcPath )
			rDestFilePaths.push_back( MakeDeepDestFilePath( *itSrcPath ) );
	}

	fs::CPath CBaseDeepTransferFilesCmd::MakeDeepDestFilePath( const fs::CPath& srcFilePath ) const
	{
		fs::TDirPath srcParentDirPath = srcFilePath.GetParentPath();
		fs::TDirPath destRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), m_srcCommonDirPath.GetPtr() );

		fs::CPath targetFullPath = m_destDirPath / destRelPath / srcFilePath.GetFilename();
		return targetFullPath;
	}


	// CBaseShallowTransferFilesCmd implementation

	CBaseShallowTransferFilesCmd::CBaseShallowTransferFilesCmd( CommandType cmdType, const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath,
																const std::vector< fs::CPath >* pDestFilePaths /*= NULL*/ )
		: cmd::CBaseFileGroupCmd( cmdType, srcFilePaths )
		, m_destDirPath( destDirPath )
	{
		if ( pDestFilePaths != NULL )
			m_destFilePaths = *pDestFilePaths;		// explicit DEST only for unexecuting
	}

	bool CBaseShallowTransferFilesCmd::QueryDestFilePaths( std::vector< fs::CPath >& rDestFilePaths ) const
	{
		CWorkingSet workingSet( this, m_fileAccessMode );

		MakeDestFilePaths( rDestFilePaths, workingSet.m_currFilePaths );
		return workingSet.IsValid();
	}

	void CBaseShallowTransferFilesCmd::Serialize( CArchive& archive ) override
	{
		__super::Serialize( archive );

		archive & m_destDirPath;
		serial::SerializeValues( archive, m_destFilePaths );
	}

	void CBaseShallowTransferFilesCmd::QueryDetailLines( std::vector< std::tstring >& rLines ) const override
	{
		QueryFilePairLines( rLines, GetSrcFilePaths(), m_destFilePaths );
	}

	std::tstring CBaseShallowTransferFilesCmd::GetDestHeaderInfo( void ) const override
	{
		return str::Format( _T("-> %s"), m_destDirPath.GetPtr() );
	}

	void CBaseShallowTransferFilesCmd::MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const
	{
		rDestFilePaths.clear();
		rDestFilePaths.reserve( srcFilePaths.size() );

		for ( std::vector< fs::CPath >::const_iterator itSrcPath = srcFilePaths.begin(); itSrcPath != srcFilePaths.end(); ++itSrcPath )
			rDestFilePaths.push_back( DoMakeDestFilePath( *itSrcPath ) );
	}

	fs::CPath CBaseShallowTransferFilesCmd::MakeBackupDestFilePath( const fs::CPath& srcFilePath ) const
	{
		// flat destination dir path
		fs::CFileBackup backup( srcFilePath, m_destDirPath, fs::FileSizeAndCrc32 );

		return backup.MakeBackupFilePath();
	}
}


// CDeleteFilesCmd implementation

IMPLEMENT_SERIAL( CDeleteFilesCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CDeleteFilesCmd::CDeleteFilesCmd( const std::vector< fs::CPath >& filePaths )
	: cmd::CBaseFileGroupCmd( cmd::DeleteFiles, filePaths )
{
	m_fileAccessMode = fs::Write;
}

CDeleteFilesCmd::~CDeleteFilesCmd()
{
}

bool CDeleteFilesCmd::Execute( void ) override
{
	CWorkingSet workingSet( this, m_fileAccessMode );

	if ( workingSet.IsValid() )
		if ( shell::DeleteFiles( workingSet.m_currFilePaths, GetParentOwner(), m_opFlags ) )		// delete to RecycleBin
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			throw CUserAbortedException();			// go silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CDeleteFilesCmd::Unexecute( void ) override
{
	CUndeleteFilesCmd undoCmd( GetFilePaths() );
	undoCmd.CopyTimestampOf( *this );

	if ( undoCmd.Execute() )
	{
		ui::MessageBox( undoCmd.Format( utl::Detailed ), MB_SETFOREGROUND );		// notify user that command was undone (editor-less command)
		return true;
	}

	return false;
}


// CDeleteFilesCmd::CUndeleteFilesCmd implementation

bool CDeleteFilesCmd::CUndeleteFilesCmd::Execute( void ) override
{
	std::vector< fs::CPath > errorFilePaths;
	size_t restoredCount = shell::UndeleteFiles( GetFilePaths(), GetParentOwner(), &errorFilePaths );

	CWorkingSet workingSet( this );
	workingSet.m_badFilePaths.swap( errorFilePaths );
	workingSet.m_succeeded = restoredCount != 0;

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}


// CCopyFilesCmd implementation

IMPLEMENT_SERIAL( CCopyFilesCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CCopyFilesCmd::CCopyFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath )
	: cmd::CBaseDeepTransferFilesCmd( cmd::CopyFiles, srcFilePaths, destDirPath )
{
	m_fileAccessMode = fs::ReadWrite;
}

CCopyFilesCmd::~CCopyFilesCmd()
{
}

CCopyFilesCmd* CCopyFilesCmd::MakePasteCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath )
{
	return SetCommandType( new CCopyFilesCmd( srcFilePaths, destDirPath ), cmd::PasteCopyFiles );
}

bool CCopyFilesCmd::Execute( void ) override
{
	CWorkingSet workingSet( this, m_fileAccessMode );

	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, workingSet.m_currFilePaths );

	if ( workingSet.IsValid() )
		if ( shell::CopyFiles( workingSet.m_currFilePaths, destFilePaths, GetParentOwner(), m_opFlags ) )		// undoable copy files
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			throw CUserAbortedException();			// go silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CCopyFilesCmd::Unexecute( void ) override
{
	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, GetSrcFilePaths() );

	std::vector< fs::TDirPath > destFolderPaths;
	fs::QueryFolderPaths( destFolderPaths, destFilePaths, true );		// store all dest folders for post-delete cleanup

	CDeleteFilesCmd undoCmd( destFilePaths );		// delete destination files
	undoCmd.CopyTimestampOf( *this );
	ClearFlag( undoCmd.m_opFlags, FOF_ALLOWUNDO );

	if ( undoCmd.Execute() )
	{
		if ( size_t delSubdirCount = shell::DeleteEmptyMultiSubdirs( GetTopDestDirPath(), destFolderPaths ) )
		{
			std::tstring message = str::Format( _T("Folders Cleanup: delete %d empty leftover sub-folders"), delSubdirCount );

			LogMessage( message );
			ui::MessageBox( message, MB_SETFOREGROUND );		// notify user that command was undone (editor-less command)
		}
		return true;
	}

	return false;
}


// CMoveFilesCmd implementation

IMPLEMENT_SERIAL( CMoveFilesCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CMoveFilesCmd::CMoveFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath )
	: cmd::CBaseDeepTransferFilesCmd( cmd::MoveFiles, srcFilePaths, destDirPath )
{
	m_fileAccessMode = fs::ReadWrite;
}

CMoveFilesCmd::~CMoveFilesCmd()
{
}

CMoveFilesCmd* CMoveFilesCmd::MakePasteCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath )
{
	return SetCommandType( new CMoveFilesCmd( srcFilePaths, destDirPath ), cmd::PasteMoveFiles );
}

bool CMoveFilesCmd::Execute( void ) override
{
	CWorkingSet workingSet( this, m_fileAccessMode );

	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, workingSet.m_currFilePaths );

	if ( workingSet.IsValid() )
		if ( shell::MoveFiles( workingSet.m_currFilePaths, destFilePaths, GetParentOwner(), m_opFlags ) )		// undoable move files
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			throw CUserAbortedException();			// go silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CMoveFilesCmd::Unexecute( void ) override
{
	std::vector< fs::CPath > destFilePaths;
	MakeDestFilePaths( destFilePaths, GetSrcFilePaths() );

	std::vector< fs::CPath > destFolderPaths;
	fs::QueryFolderPaths( destFolderPaths, destFilePaths, true );		// store all dest folders for post-delete cleanup

	CMoveFilesCmd undoCmd( destFilePaths, GetSrcCommonDirPath() );		// move back destination files to source common directory
	undoCmd.SetTypeID( GetCmdType() );
	undoCmd.CopyTimestampOf( *this );

	if ( undoCmd.Execute() )
	{
		if ( size_t delSubdirCount = shell::DeleteEmptyMultiSubdirs( GetTopDestDirPath(), destFolderPaths ) )
		{
			std::tstring message = str::Format( _T("Folders Cleanup: delete %d empty leftover sub-folders"), delSubdirCount );

			LogMessage( message );
			ui::MessageBox( message, MB_SETFOREGROUND );		// notify user that command was undone (editor-less command)
		}
		return true;
	}

	return false;
}


// CCreateFoldersCmd implementation

IMPLEMENT_SERIAL( CCreateFoldersCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CCreateFoldersCmd::CCreateFoldersCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath, Structure structure /*= CreateNormal*/ )
	: cmd::CBaseDeepTransferFilesCmd( cmd::CreateFolders, srcFilePaths, destDirPath )
{
	switch ( structure )
	{
		case PasteDirs:			SetTypeID( cmd::PasteCreateFolders ); break;
		case PasteDeepStruct:	SetTypeID( cmd::PasteCreateDeepFolders ); break;
	}

	m_fileAccessMode = fs::Exist;
}

CCreateFoldersCmd::~CCreateFoldersCmd()
{
}

bool CCreateFoldersCmd::Execute( void ) override
{
	CWorkingSet workingSet( this, m_fileAccessMode );

	std::vector< fs::CPath > destFolderPaths;
	MakeDestFilePaths( destFolderPaths, workingSet.m_currFilePaths );
	fs::SortByPathDepth( destFolderPaths );		// for creation order: folder -> sub-folder

	if ( destFolderPaths.empty() )
		return false;							// nothing to do

	size_t createdCount = 0;

	for ( std::vector< fs::CPath >::iterator itDestFolderPath = destFolderPaths.begin(); itDestFolderPath != destFolderPaths.end(); ++itDestFolderPath )
		if ( !fs::IsValidDirectory( itDestFolderPath->GetPtr() ) )
			try
			{
				fs::thr::CreateDirPath( itDestFolderPath->GetPtr() );
				++createdCount;
			}
			catch ( CRuntimeException& exc )
			{
				app::TraceException( exc );
				workingSet.m_badFilePaths.push_back( *itDestFolderPath );
			}

	workingSet.m_succeeded = destFolderPaths.empty() || createdCount != 0;
	if ( workingSet.m_succeeded )
		if ( createdCount != GetSrcFolderPaths().size() )	// created fewer directories?
			app::ReportError( str::Format( _T("Created %d new folders out of %d total folders on clipboard."), createdCount, GetSrcFolderPaths().size() ), app::Info );

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CCreateFoldersCmd::Unexecute( void ) override
{
	std::vector< fs::CPath > destFolderPaths;
	MakeDestFilePaths( destFolderPaths, GetSrcFolderPaths() );
	fs::SortByPathDepth( destFolderPaths, false );		// for deletion order: sub-folder -> folder

	CDeleteFilesCmd undoCmd( destFolderPaths );			// delete destination files
	undoCmd.CopyTimestampOf( *this );
	ClearFlag( undoCmd.m_opFlags, FOF_ALLOWUNDO );

	if ( undoCmd.Execute() )
	{
		if ( size_t delSubdirCount = shell::DeleteEmptyMultiSubdirs( GetTopDestDirPath(), destFolderPaths ) )
		{
			std::tstring message = str::Format( _T("Folders Cleanup: delete %d empty leftover sub-folders"), delSubdirCount );

			LogMessage( message );
			ui::MessageBox( message, MB_SETFOREGROUND );		// notify user that command was undone (editor-less command)
		}
		return true;
	}

	return false;
}


// CCopyPasteFilesAsBackupCmd implementation

IMPLEMENT_SERIAL( CCopyPasteFilesAsBackupCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CCopyPasteFilesAsBackupCmd::CCopyPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath )
	: cmd::CBaseShallowTransferFilesCmd( cmd::CopyPasteFilesAsBackup, srcFilePaths, destDirPath )
{
	m_fileAccessMode = fs::ReadWrite;
	SetFlag( m_opFlags, FOF_NOCONFIRMATION );		// no need to bother the user on backup - the intent is to override files

	// object fully constructed (can call virtual methods): initialize DEST paths
	MakeDestFilePaths( m_destFilePaths, srcFilePaths );
}

CCopyPasteFilesAsBackupCmd::~CCopyPasteFilesAsBackupCmd()
{
}

bool CCopyPasteFilesAsBackupCmd::Execute( void ) override
{
	CWorkingSet workingSet( GetSrcFilePaths(), m_destFilePaths, m_fileAccessMode );

	if ( workingSet.IsValid() )
		if ( shell::CopyFiles( workingSet.m_currFilePaths, workingSet.m_currDestFilePaths, GetParentOwner(), m_opFlags ) )		// undoable copy files, no override prompt
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			throw CUserAbortedException();			// go silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CCopyPasteFilesAsBackupCmd::Unexecute( void ) override
{
	CDeleteFilesCmd undoCmd( m_destFilePaths );		// delete destination files

	undoCmd.CopyTimestampOf( *this );
	ClearFlag( undoCmd.m_opFlags, FOF_ALLOWUNDO );
	SetFlag( undoCmd.m_opFlags, FOF_NOCONFIRMATION );

	return undoCmd.Execute();
}


// CCutPasteFilesAsBackupCmd implementation

IMPLEMENT_SERIAL( CCutPasteFilesAsBackupCmd, CBaseSerialCmd, VERSIONABLE_SCHEMA | 1 )

CCutPasteFilesAsBackupCmd::CCutPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath )
	: cmd::CBaseShallowTransferFilesCmd( cmd::CutPasteFilesAsBackup, srcFilePaths, destDirPath )
{
	m_fileAccessMode = fs::ReadWrite;
	SetFlag( m_opFlags, FOF_NOCONFIRMATION );		// no need to bother the user on backup - the intent is to override files

	// object fully constructed (can call virtual methods): initialize DEST paths
	MakeDestFilePaths( m_destFilePaths, srcFilePaths );
}

CCutPasteFilesAsBackupCmd::CCutPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath, const std::vector< fs::CPath >& destFilePaths )
	: cmd::CBaseShallowTransferFilesCmd( cmd::CutPasteFilesAsBackup, srcFilePaths, destDirPath, &destFilePaths )
{	// only for unexecution
	m_fileAccessMode = fs::ReadWrite;
}

CCutPasteFilesAsBackupCmd::~CCutPasteFilesAsBackupCmd()
{
}

bool CCutPasteFilesAsBackupCmd::Execute( void ) override
{
	CWorkingSet workingSet( GetSrcFilePaths(), m_destFilePaths, m_fileAccessMode );

	if ( workingSet.IsValid() )
		if ( shell::MoveFiles( workingSet.m_currFilePaths, workingSet.m_currDestFilePaths, GetParentOwner(), m_opFlags ) )		// undoable move files
			workingSet.m_succeeded = true;
		else if ( shell::AnyOperationAborted() )
			throw CUserAbortedException();			// go silent if cancelled by user

	return HandleExecuteResult( workingSet, str::Join( workingSet.m_currFilePaths, s_lineEnd ) );
}

bool CCutPasteFilesAsBackupCmd::Unexecute( void ) override
{
	const std::vector< fs::CPath >& srcFilePaths = GetSrcFilePaths();
	fs::TDirPath srcCommonDirPath( path::ExtractCommonParentPath( srcFilePaths ) );

	CCutPasteFilesAsBackupCmd undoCmd( m_destFilePaths, srcCommonDirPath, srcFilePaths );		// move back destination files to source common directory

	undoCmd.CopyTimestampOf( *this );

	return undoCmd.Execute();
}
