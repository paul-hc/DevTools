#ifndef RenameItem_h
#define RenameItem_h
#pragma once

#include "BasePathItem.h"
#include "FileWorkingSet_fwd.h"


class CRenameItem : public CBasePathItem
{
public:
	CRenameItem( const TPathPair* pPathPair );
	virtual ~CRenameItem();

	const fs::CPath& GetSrcPath( void ) const { return m_pPathPair->first; }		// AKA key path
	const fs::CPath& GetDestPath( void ) const { return m_pPathPair->second; }
private:
	const TPathPair* m_pPathPair;
};


#endif // RenameItem_h
