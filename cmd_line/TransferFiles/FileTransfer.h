#ifndef FileTransfer_h
#define FileTransfer_h
#pragma once

#include <set>
#include "utl/ConsoleApplication.h"
#include "utl/FileSystem_fwd.h"
#include "TransferItem.h"


struct CXferOptions;


class CFileTransfer
	: private fs::IEnumeratorImpl
{
public:
	CFileTransfer( const CXferOptions* pOptions );
	~CFileTransfer();

	int Run( void );

	std::ostream& PrintStatistics( std::ostream& os ) const;
private:
	void SearchSourceFiles( const fs::CPath& dirPath );
	int Transfer( void );

	// user-interactive
	bool CanAlterTargetFile( const CTransferItem& item );
	bool CreateTargetDirectory( const CTransferItem& item );

	static std::tstring FormatProtectedFileAttr( BYTE fileAttr );
private:
	// fs::IEnumerator interface (files only)
	virtual void OnAddFileInfo( const fs::CFileState& fileState );
	virtual void AddFoundFile( const fs::CPath& filePath ) { filePath; ASSERT( false ); }
	virtual bool AddFoundSubDir( const fs::TDirPath& subDirPath );

	bool AddTransferItem( CTransferItem* pTransferItem );

	typedef std::map<fs::CPath, CTransferItem*> TTransferItemMap;		// uses pred::TLess_NaturalPath
private:
	const CXferOptions* m_pOptions;
	TTransferItemMap m_transferItems;
	size_t m_fileCount;
	size_t m_createdDirCount;

	// interactive state
	io::CUserQuery m_uqOverrideReadOnly;
	io::CUserQuery m_uqOverrideFiles;
	io::CUserQueryCreateDirectory m_uqCreateDirs;
};


#endif // FileTransfer_h
