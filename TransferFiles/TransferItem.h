// Copyleft 2004 Paul H. Cocoveanu
//
#ifndef TransferItem_h
#define TransferItem_h

#include "FileInfo.h"
#include "utl/Path.h"


enum FileAction { FileCopy, FileMove, TargetFileRemove };


struct CTransferItem
{
	CTransferItem( const CFileFind& sourceFinder, const std::tstring& rootSourceDirPath, const std::tstring& rootTargetDirPath );
	CTransferItem( const std::tstring& sourceFilePath, const std::tstring& rootSourceDirPath, const std::tstring& rootTargetDirPath );
	~CTransferItem();

	bool Transfer( FileAction fileAction );
	std::ostream& Print( std::ostream& os, FileAction fileAction, bool showTimestamp = false ) const;
private:
	static std::tstring MakeTargetFilePath( const std::tstring& sourceFullPath, const std::tstring& rootSourceDirPath,
											const std::tstring& rootTargetDirPath );
public:
	CFileInfo m_sourceFileInfo;
	CFileInfo m_targetFileInfo;
};


#endif // TransferItem_h
