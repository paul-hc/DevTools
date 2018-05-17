// Copyleft 2004-2016 Paul H. Cocoveanu

#ifndef FileTransfer_h
#define FileTransfer_h

#include <set>
#include "utl/FileSystem.h"
#include "TransferItem.h"


struct CXferOptions;


class CFileTransfer : private fs::IEnumerator
{
public:
	CFileTransfer( const CXferOptions& options );
	~CFileTransfer();

	int Run( void );

	std::ostream& PrintStatistics( std::ostream& os ) const;
private:
	void SearchSourceFiles( const std::tstring& dirPath );
	int Transfer( void );

	bool CanAlterTargetFile( const CTransferItem& node );

	static std::tstring FormatProtectedFileAttr( DWORD fileAttr );
private:
	typedef std::map< fs::CPath, CTransferItem*, pred::Less_EquivalentPath > TransferItemMap;

	// fs::IEnumerator interface (files only)
	virtual void AddFoundFile( const TCHAR* pFilePath ) { pFilePath; ASSERT( false ); }
	virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	virtual void AddFile( const CFileFind& foundFile );

	bool AddTransferItem( CTransferItem* pTransferItem );
private:
	const CXferOptions& m_options;
	TransferItemMap m_xferNodesMap;
	size_t m_fileCount;
	size_t m_createdDirCount;

	fs::TPathSet m_failedTargetDirs;
};


#endif // FileTransfer_h
