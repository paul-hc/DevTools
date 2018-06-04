
#include "stdafx.h"
#include "RenameItem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ren
{
	bool MakePairsFromItems( fs::TPathPairMap& rOutRenamePairs, const std::vector< CRenameItem* >& renameItems )
	{
		rOutRenamePairs.clear();
		for ( std::vector< CRenameItem* >::const_iterator itItem = renameItems.begin(); itItem != renameItems.end(); ++itItem )
			rOutRenamePairs[ ( *itItem )->GetSrcPath() ] = ( *itItem )->GetDestPath();

		return rOutRenamePairs.size() == renameItems.size();			// all SRC keys unique?
	}

	void AssignPairsToItems( const std::vector< CRenameItem* >& rOutRenameItems, const fs::TPathPairMap& renamePairs )
	{
		REQUIRE( rOutRenameItems.size() == renamePairs.size() );

		size_t pos = 0;
		for ( fs::TPathPairMap::const_iterator itPair = renamePairs.begin(); itPair != renamePairs.end(); ++itPair, ++pos )
		{
			CRenameItem* pItem = rOutRenameItems[ pos ];

			if ( pItem->GetSrcPath() == itPair->first )			// both containers must be in the same order
				pItem->RefDestPath() = itPair->second;
			else
			{
				ASSERT( false );
				break;
			}
		}
	}
}


CRenameItem::CRenameItem( const fs::CPath& srcPath )
	: CBasePathItem( srcPath )
{
}

CRenameItem::~CRenameItem()
{
}
