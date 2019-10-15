
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

	void MakePairsToItems( std::vector< CRenameItem* >& rOutRenameItems, const fs::TPathPairMap& renamePairs )
	{
		REQUIRE( rOutRenameItems.empty() );

		for ( fs::TPathPairMap::const_iterator itPair = renamePairs.begin(); itPair != renamePairs.end(); ++itPair )
		{
			CRenameItem* pItem = new CRenameItem( itPair->first );

			pItem->RefDestPath() = itPair->second;
			rOutRenameItems.push_back( pItem );
		}
	}

	void AssignPairsToItems( const std::vector< CRenameItem* >& items, const fs::TPathPairMap& renamePairs )
	{
		REQUIRE( items.size() == renamePairs.size() );

		size_t pos = 0;
		for ( fs::TPathPairMap::const_iterator itPair = renamePairs.begin(); itPair != renamePairs.end(); ++itPair, ++pos )
		{
			CRenameItem* pItem = items[ pos ];

			if ( pItem->GetSrcPath() == itPair->first )			// both containers must be in the same order
				pItem->RefDestPath() = itPair->second;
			else
			{
				ASSERT( false );
				break;
			}
		}
	}

	void QueryDestFnames( std::vector< std::tstring >& rDestFnames, const std::vector< CRenameItem* >& items )
	{
		rDestFnames.clear();
		rDestFnames.reserve( items.size() );

		for ( std::vector< CRenameItem* >::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			rDestFnames.push_back( fs::CPathParts( ( *itItem )->GetSafeDestPath().Get() ).m_fname );
	}
}


CRenameItem::CRenameItem( const fs::CPath& srcPath )
	: CPathItemBase( srcPath )
{
}

CRenameItem::~CRenameItem()
{
}
