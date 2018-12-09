
#include "stdafx.h"
#include "TransferItem.h"
#include "XferOptions.h"
#include "utl/FileSystem.h"
#include "utl/FileContent.h"
#include "utl/RuntimeException.h"
#include <iostream>


const CEnumTags& GetTags_FileAction( void )
{
	static const CEnumTags s_tags( _T("Copy|Move|Delete") );
	return s_tags;
}


// CBackupInfo implementation

CBackupInfo::CBackupInfo( const CXferOptions* pOptions )
	: m_dirPath( *pOptions->m_pBackupDirPath )
	, m_uqCreateDir( _T("Create backup directory"), pOptions->m_userPrompt != PromptNever )
{
}


// CTransferItem implementation

CTransferItem::CTransferItem( const CFileFind& sourceFinder, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath )
	: m_sourceFileInfo( sourceFinder )
	, m_targetFileInfo( MakeDeepTargetFilePath( m_sourceFileInfo.m_filePath, rootSourceDirPath, rootTargetDirPath ) )
{
}

CTransferItem::CTransferItem( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath )
	: m_sourceFileInfo( srcFilePath )
	, m_targetFileInfo( MakeDeepTargetFilePath( m_sourceFileInfo.m_filePath, rootSourceDirPath, rootTargetDirPath ) )
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
		os << m_sourceFileInfo.m_filePath.Get();
		if ( showTimestamp && m_sourceFileInfo.Exist() )
			os << " (" << m_sourceFileInfo.m_lastModifyTime.Format( s_fmtTimestamp ) << ")";
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
	os << m_targetFileInfo.m_filePath;
	if ( showTimestamp && m_targetFileInfo.Exist() )
		os << " (" << m_targetFileInfo.m_lastModifyTime.Format( s_fmtTimestamp ) << ")";
	os << std::endl;

	if ( !m_targetBackupFilePath.IsEmpty() )
		os << s_linePrefix << "[~] " << path::StripCommonPrefix( m_targetBackupFilePath.GetPtr(), m_targetFileInfo.m_filePath.GetParentPath().GetPtr() ) << std::endl;

	return os;
}

bool CTransferItem::Transfer( FileAction fileAction, CBackupInfo* pBackupInfo )
{
	if ( m_sourceFileInfo.IsRegularFile() )
		try
		{
			if ( m_targetFileInfo.IsReadOnly() )
				fs::thr::MakeFileWritable( m_targetFileInfo.m_filePath.GetPtr() );

			if ( pBackupInfo != NULL )
				BackupExistingTarget( pBackupInfo );

			switch ( fileAction )
			{
				case FileCopy:
					fs::thr::CopyFile( m_sourceFileInfo.m_filePath.GetPtr(), m_targetFileInfo.m_filePath.GetPtr(), false );
					return true;
				case FileMove:
					fs::thr::MoveFile( m_sourceFileInfo.m_filePath.GetPtr(), m_targetFileInfo.m_filePath.GetPtr(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
					return true;
				case TargetFileDelete:
					if ( m_targetFileInfo.Exist() )
					{
						fs::thr::DeleteFile( m_targetFileInfo.m_filePath.GetPtr() );
						return true;
					}
					break;
			}
		}
		catch ( const std::exception& exc )
		{
			io::ReportException( exc );
		}

	return false;
}

bool CTransferItem::BackupExistingTarget( CBackupInfo* pBackupInfo )
{
	ASSERT_PTR( pBackupInfo );

	const fs::CPath& srcFilePath = m_targetFileInfo.m_filePath;		// target file is the source for backup

	if ( !srcFilePath.FileExist() )
		return false;			// no existing target file to backup

	fs::CPath backupDirPath = srcFilePath.GetParentPath();
	if ( !pBackupInfo->m_dirPath.IsEmpty() )
		if ( path::IsRelative( pBackupInfo->m_dirPath.GetPtr() ) )
		{
			backupDirPath /= pBackupInfo->m_dirPath;
			fs::CvtAbsoluteToCWD( backupDirPath );
		}
		else
			backupDirPath = pBackupInfo->m_dirPath;

	if ( fs::CreationError == pBackupInfo->m_uqCreateDir.Acquire( backupDirPath ) )
		return false;

	try
	{
		fs::CFileBackup backup( srcFilePath, backupDirPath, fs::FileSize );		// use quick size-based content identification
		if ( fs::Created == backup.CreateBackupFile( m_targetBackupFilePath ) )
			return true;

		m_targetBackupFilePath.Clear();
		return false;
	}
	catch ( const CRuntimeException& exc )
	{
		io::TraceException( exc );
	}

	return false;
}
