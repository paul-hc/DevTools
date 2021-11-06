
#include "stdafx.h"
#include "TransferItem.h"
#include "XferOptions.h"
#include "utl/FileSystem.h"
#include "utl/FileContent.h"
#include "utl/RuntimeException.h"
#include "utl/TimeUtils.h"
#include <iostream>


const CEnumTags& GetTags_FileAction( void )
{
	static const CEnumTags s_tags( _T("Copy|Move|Delete") );
	return s_tags;
}


// CBackupInfo implementation

CBackupInfo::CBackupInfo( const CXferOptions* pOptions )
	: m_dirPath( *pOptions->m_pBackupDirPath )
	, m_matchContentBy( CheckFullContent == pOptions->m_filterBy ? fs::FileSizeAndCrc32 : fs::FileSize )
	, m_uqCreateDir( _T("Create backup directory"), pOptions->m_userPrompt != PromptNever )
{
}


// CTransferItem implementation

CTransferItem::CTransferItem( const fs::CFileState& fileState, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath )
	: m_source( fileState )
	, m_target( fs::CFileState::ReadFromFile( MakeDeepTargetFilePath( m_source.m_fullPath, rootSourceDirPath, rootTargetDirPath ) ) )
{
}

CTransferItem::CTransferItem( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath )
	: m_source( fs::CFileState::ReadFromFile( srcFilePath ) )
	, m_target( fs::CFileState::ReadFromFile( MakeDeepTargetFilePath( m_source.m_fullPath, rootSourceDirPath, rootTargetDirPath ) ) )
{
}

CTransferItem::~CTransferItem()
{
}

fs::CPath CTransferItem::MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath,
												 const fs::CPath& rootTargetDirPath )
{
	fs::CPath srcParentDirPath = srcFilePath.GetParentPath();
	std::tstring targetRelPath = path::StripCommonPrefix( srcParentDirPath.GetPtr(), rootSourceDirPath.GetPtr() );

	fs::CPath targetFullPath = rootTargetDirPath / targetRelPath / srcFilePath.GetFilename();
	return targetFullPath;
}

std::ostream& CTransferItem::Print( std::ostream& os, FileAction fileAction, bool showTimestamp /*= false*/ ) const
{
	static const char s_fmtTimestamp[] = "%b %d, %Y";
	static const char s_linePrefix[] = "  ";

	if ( FileCopy == fileAction || FileMove == fileAction )
	{
		os << s_linePrefix;
		if ( fileAction == FileMove )
			os << "[-] ";
		os << m_source.m_fullPath.Get();
		if ( showTimestamp && m_source.IsValid() )
			os << " (" << m_source.m_modifTime.Format( s_fmtTimestamp ) << ")";
		os << " ->" << std::endl;
	}

	os << s_linePrefix;
	switch ( fileAction )
	{
		case FileMove:
			os << "[+] ";
			break;
		case TargetFileDelete:
			os << "[-] ";
			break;
	}
	os << m_target.m_fullPath;
	if ( showTimestamp && m_target.IsValid() )
		os << " (" << m_target.m_modifTime.Format( s_fmtTimestamp ) << ")";
	os << std::endl;

	if ( !m_targetBackupFilePath.IsEmpty() )
		os << s_linePrefix << "[~] " << path::StripCommonPrefix( m_targetBackupFilePath.GetPtr(), m_target.m_fullPath.GetParentPath().GetPtr() ) << std::endl;

	return os;
}

bool CTransferItem::PassesFileFilter( const CXferOptions* pOptions ) const
{
	ASSERT_PTR( pOptions );

	if ( pOptions->m_filterBy >= CheckTimestamp )
		if ( IsSrcNewer( pOptions->m_earliestTimestamp ) )
			return true;

	if ( pOptions->m_filterBy >= CheckFileSize )
		if ( HasDifferentContents( CheckFullContent == pOptions->m_filterBy ? fs::FileSizeAndCrc32 : fs::FileSize ) )
			return true;

	return false;
}

bool CTransferItem::IsSrcNewer( const CTime& earliestTimestamp ) const
{
	if ( time_utl::IsValid( earliestTimestamp ) )		// SRC vs TIME
	{
		TRACE( _T("<earliest: %s>  <source: %s>\n"),
			time_utl::FormatTimestamp( earliestTimestamp ).c_str(),
			time_utl::FormatTimestamp( m_source.m_modifTime ).c_str() );

		if ( m_source.m_modifTime < earliestTimestamp )
			return false;

		TRACE( _T("  copy newer: %s\n"), m_source.m_fullPath.GetPtr() );
	}

	if ( m_target.IsValid() )								// SRC vs TARGET timestamps
	{
		TRACE( _T("<source: %s>  <target: %s>\n"),
			time_utl::FormatTimestamp( m_source.m_modifTime ).c_str(),
			time_utl::FormatTimestamp( m_target.m_modifTime ).c_str() );

		if ( m_source.m_modifTime <= m_target.m_modifTime )
			return false;

		TRACE( _T("  copy newer: %s\n  to:         %s\n"),
			m_source.m_fullPath.GetPtr(),
			m_target.m_fullPath.GetPtr() );
	}

	return true;
}

bool CTransferItem::HasDifferentContents( fs::FileContentMatch matchContentBy ) const
{
	if ( !m_target.IsValid() )
		return true;			// TARGET missing, must transfer

	switch ( fs::EvalTransferMatch( m_source.m_fullPath, m_target.m_fullPath, false, matchContentBy ) )
	{
		case fs::SizeMismatch:
		case fs::Crc32Mismatch:
			return true;		// SRC is changed, must transfer
		default:
			return false;		// SRC up-to-date, filter-out
	}
}

bool CTransferItem::Transfer( FileAction fileAction, const CBackupInfo* pBackupInfo )
{
	if ( m_source.IsRegularFile() )
		try
		{
			if ( m_target.IsReadOnly() )
				fs::thr::MakeFileWritable( m_target.m_fullPath.GetPtr() );

			if ( pBackupInfo != NULL )
				BackupExistingTarget( *pBackupInfo );

			switch ( fileAction )
			{
				case FileCopy:
					fs::thr::CopyFile( m_source.m_fullPath.GetPtr(), m_target.m_fullPath.GetPtr(), false );
					return true;
				case FileMove:
					fs::thr::MoveFile( m_source.m_fullPath.GetPtr(), m_target.m_fullPath.GetPtr(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
					return true;
				case TargetFileDelete:
					if ( m_target.IsValid() )
					{
						fs::thr::DeleteFile( m_target.m_fullPath.GetPtr() );
						return true;
					}
					break;
			}
		}
		catch ( const std::exception& exc )
		{
			app::ReportException( exc );
		}

	return false;
}

bool CTransferItem::BackupExistingTarget( const CBackupInfo& backupInfo )
{
	const fs::CPath& srcFilePath = m_target.m_fullPath;		// target file is the source for backup

	if ( !srcFilePath.FileExist() )
		return false;			// no existing target file to backup

	fs::CPath backupDirPath = srcFilePath.GetParentPath();
	if ( !backupInfo.m_dirPath.IsEmpty() )
		if ( path::IsRelative( backupInfo.m_dirPath.GetPtr() ) )
		{
			backupDirPath /= backupInfo.m_dirPath;
			fs::CvtAbsoluteToCWD( backupDirPath );
		}
		else
			backupDirPath = backupInfo.m_dirPath;

	if ( fs::CreationError == backupInfo.m_uqCreateDir.Acquire( backupDirPath ) )
		return false;

	try
	{
		fs::CFileBackup backup( srcFilePath, backupDirPath, backupInfo.m_matchContentBy );
		if ( fs::Created == backup.CreateBackupFile( m_targetBackupFilePath ) )
			return true;

		m_targetBackupFilePath.Clear();
		return false;
	}
	catch ( const CRuntimeException& exc )
	{
		app::TraceException( exc );
	}

	return false;
}
