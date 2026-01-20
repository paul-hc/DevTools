#ifndef FileService_h
#define FileService_h
#pragma once

#include "utl/Command.h"
#include <set>


class CRenameItem;
class CTouchItem;
class CEditLinkItem;


class CFileService
{
public:
	CFileService( void );

	std::auto_ptr<CMacroCommand> MakeRenameCmds( const std::vector<CRenameItem*>& renameItems ) const;
	std::auto_ptr<CMacroCommand> MakeTouchCmds( const std::vector<CTouchItem*>& touchItems ) const;
	std::auto_ptr<CMacroCommand> MakeEditLinkCmds( const std::vector<CEditLinkItem*>& editLinkItems ) const;
private:
	static bool IsDistinctWorkingSet( const std::vector<CRenameItem*>& renameItems );		// all SRC and DEST paths are a distinct working set?
	static fs::CPath MakeUniqueIntermPath( const fs::CPath& destPath, const std::set<fs::CPath>& destToBeSet );
};


#endif // FileService_h
