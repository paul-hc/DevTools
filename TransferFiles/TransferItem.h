#ifndef TransferItem_h
#define TransferItem_h
#pragma once

#include "FileInfo.h"
#include "utl/ConsoleInputOutput.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"


struct CXferOptions;


enum FileAction { FileCopy, FileMove, TargetFileDelete };

const CEnumTags& GetTags_FileAction( void );


struct CBackupInfo
{
	CBackupInfo( const CXferOptions* pOptions );
public:
	fs::CPath m_dirPath;
	io::CUserQueryCreateDirectory m_uqCreateDir;
};


struct CTransferItem
{
	CTransferItem( const CFileFind& sourceFinder, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath );
	CTransferItem( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath, const fs::CPath& rootTargetDirPath );
	~CTransferItem();

	bool Transfer( FileAction fileAction, CBackupInfo* pBackupInfo );
	std::ostream& Print( std::ostream& os, FileAction fileAction, bool showTimestamp = false ) const;
private:
	bool BackupExistingTarget( CBackupInfo* pBackupInfo );

	static fs::CPath MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& rootSourceDirPath,
											 const fs::CPath& rootTargetDirPath );
public:
	CFileInfo m_sourceFileInfo;
	CFileInfo m_targetFileInfo;
	fs::CPath m_targetBackupFilePath;
};


#endif // TransferItem_h
