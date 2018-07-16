#ifndef RenameItem_h
#define RenameItem_h
#pragma once

#include "PathItemBase.h"


class CRenameItem : public CPathItemBase
{
public:
	CRenameItem( const fs::CPath& srcPath );
	virtual ~CRenameItem();

	const fs::CPath& GetSrcPath( void ) const { return GetKeyPath(); }
	const fs::CPath& GetDestPath( void ) const { return m_destPath; }
	const fs::CPath& GetSafeDestPath( void ) const { return !m_destPath.IsEmpty() ? m_destPath : GetSrcPath(); }	// use SRC if DEST emty

	bool IsModified( void ) const { return HasDestPath() && m_destPath.Get() != GetKeyPath().Get(); }		// case-sensitive string compare (not paths)
	bool HasDestPath( void ) const { return !m_destPath.IsEmpty(); }

	void Reset( void ) { m_destPath.Clear(); }
	fs::CPath& RefDestPath( void ) { return m_destPath; }
private:
	fs::CPath m_destPath;
};


namespace ren
{
	bool MakePairsFromItems( fs::TPathPairMap& rOutRenamePairs, const std::vector< CRenameItem* >& renameItems );
	void MakePairsToItems( std::vector< CRenameItem* >& rOutRenameItems, const fs::TPathPairMap& renamePairs );
	void AssignPairsToItems( const std::vector< CRenameItem* >& rOutRenameItems, const fs::TPathPairMap& renamePairs );
}


#endif // RenameItem_h
