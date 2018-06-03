#ifndef FileService_h
#define FileService_h
#pragma once

#include "FileCommands_fwd.h"
#include <set>


class CRenameItem;
class CTouchItem;


class CFileService
{
public:
	CFileService( cmd::IErrorObserver* pErrorObserver = NULL );

	CMacroCommand* MakeRenameCmds( const std::vector< CRenameItem* >& renameItems ) const;
	CMacroCommand* MakeTouchCmds( const std::vector< CTouchItem* >& touchItems ) const;
private:
	static bool IsDistinctWorkingSet( const std::vector< CRenameItem* >& renameItems );		// all SRC and DEST paths are a distinct working set?
	static fs::CPath MakeUniqueIntermPath( const fs::CPath& destPath, const std::set< fs::CPath >& destToBeSet );
private:
	cmd::IErrorObserver* m_pErrorObserver;
};


#endif // FileService_h
