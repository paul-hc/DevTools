#ifndef TransferItem_h
#define TransferItem_h
#pragma once

#include "FileInfo.h"
#include "utl/ConsoleInputOutput.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem_fwd.h"


struct CXferOptions;


enum FileAction { FileCopy, FileMove, TargetFileDelete };

const CEnumTags& GetTags_FileAction( void );


struct CBackupInfo
{
	CBackupInfo( const CXferOptions* pOptions );
public:
	fs::CPath m_dirPath;
	fs::FileContentMatch m_matchContentBy;
	mutable io::CUserQueryCreateDirectory m_uqCreateDir;
};


struct CTransferItem
{
	CTransferItem( const CFileFind& sourceFinder, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath );
	CTransferItem( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath );
	~CTransferItem();

	bool PassesFileFilter( const CXferOptions* pOptions ) const;
	bool Transfer( FileAction fileAction, const CBackupInfo* pBackupInfo );
	std::ostream& Print( std::ostream& os, FileAction fileAction, bool showTimestamp = false ) const;
private:
	bool IsSrcNewer( const CTime& earliestTimestamp ) const;
	bool HasDifferentContents( fs::FileContentMatch matchContentBy ) const;

	bool BackupExistingTarget( const CBackupInfo& backupInfo );

	static fs::CPath MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath,
											 const fs::CPath& rootTargetDirPath );
public:
	CFileInfo m_source;
	CFileInfo m_target;
	fs::CPath m_targetBackupFilePath;
};


#endif // TransferItem_h
