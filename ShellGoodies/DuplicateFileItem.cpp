
#include "stdafx.h"
#include "DuplicateFileItem.h"
#include "utl/ContainerUtilities.h"
#include "utl/Crc32.h"
#include "utl/ComparePredicates.h"
#include "utl/FileSystem.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDuplicateFileItem implementation

CDuplicateFileItem::CDuplicateFileItem( const fs::CPath& filePath, CDuplicateFilesGroup* pParentGroup )
	: CPathItemBase( filePath )
	, m_pParentGroup( pParentGroup )
	, m_modifyTime( fs::ReadLastModifyTime( filePath ) )
{
	ASSERT_PTR( m_pParentGroup );
}

bool CDuplicateFileItem::IsOriginalItem( void ) const
{
	return this == m_pParentGroup->GetItems().front();
}

bool CDuplicateFileItem::MakeOriginalItem( void )
{
	return m_pParentGroup->MakeOriginalItem( this );
}

bool CDuplicateFileItem::MakeDuplicateItem( void )
{
	return m_pParentGroup->MakeDuplicateItem( this );
}


// CDuplicateFilesGroup implementation

CDuplicateFilesGroup::~CDuplicateFilesGroup()
{
	utl::ClearOwningContainer( m_items );
}

CDuplicateFileItem* CDuplicateFilesGroup::FindItem( const fs::CPath& filePath ) const
{
	for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = m_items.begin(); itItem != m_items.end(); ++itItem )
		if ( filePath == ( *itItem )->GetFilePath() )
			return *itItem;

	return NULL;
}

bool CDuplicateFilesGroup::MakeOriginalItem( CDuplicateFileItem* pItem )
{
	ASSERT_PTR( pItem );
	std::vector< CDuplicateFileItem* >::iterator itFountItem = std::find( m_items.begin(), m_items.end(), pItem );
	ASSERT( itFountItem != m_items.end() );

	if ( pItem->IsOriginalItem() )
		return false;

	m_items.erase( itFountItem );
	m_items.insert( m_items.begin(), pItem );
	return true;
}

bool CDuplicateFilesGroup::MakeDuplicateItem( CDuplicateFileItem* pItem )
{
	ASSERT_PTR( pItem );
	std::vector< CDuplicateFileItem* >::iterator itFountItem = std::find( m_items.begin(), m_items.end(), pItem );
	ASSERT( itFountItem != m_items.end() );

	if ( !pItem->IsOriginalItem() )
		return false;

	m_items.erase( itFountItem );
	m_items.insert( m_items.end(), pItem );
	return true;
}

void CDuplicateFilesGroup::AddItem( const fs::CPath& filePath )
{
	ASSERT( !ContainsItem( filePath ) );

	m_items.push_back( new CDuplicateFileItem( filePath, this ) );
}

void CDuplicateFilesGroup::AddItem( CDuplicateFileItem* pItem )
{
	ASSERT_PTR( pItem );
	ASSERT( !ContainsItem( pItem->GetFilePath() ) );

	m_items.push_back( pItem );
	pItem->SetParentGroup( this );
}

void CDuplicateFilesGroup::ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress /*= NULL*/ ) throws_( CUserAbortedException )
{
	REQUIRE( HasDuplicates() );
	REQUIRE( 0 == m_contentKey.m_crc32 );			// CRC32 is yet to be computed

	typedef std::pair< fs::CFileContentKey, CDuplicateFileItem* > TKeyItemPair;
	typedef utl::COwningContainer< std::vector< TKeyItemPair >, func::DeleteValue > TKeyItemContainer;

	TKeyItemContainer scopedKeyItems;
	scopedKeyItems.reserve( m_items.size() );

	for ( std::vector< CDuplicateFileItem* >::iterator itItem = m_items.begin(); itItem != m_items.end(); ++itItem )
	{
		if ( pProgress != NULL )
			pProgress->AdvanceItem( ( *itItem )->GetFilePath().Get() );

		TKeyItemPair newItem( m_contentKey, *itItem );

		if ( newItem.first.ComputeCrc32( newItem.second->GetFilePath() ) )
		{
			if ( pProgress != NULL )
				pProgress->AdvanceStage( newItem.first.Format() );

			scopedKeyItems.push_back( newItem );
			utl::ReleaseOwnership( *itItem );			// release detached item ownership to prevent being deleted when unwiding the stack
		}
		else
		{
			delete *itItem;
			++rIgnoredCount;			// remaining items will be deleted when unwiding the stack
		}
	}

	m_items.clear();					// ownership was passed to scopedKeyItems

	typedef pred::CompareFirstSecond< pred::CompareValue, pred::CompareItemPath > TCompareKeyItemPair;

	std::sort( scopedKeyItems.begin(), scopedKeyItems.end(), pred::LessValue< TCompareKeyItemPair >() );			// sort by full key and path

	typedef std::pair< TKeyItemContainer::iterator, TKeyItemContainer::iterator > IteratorPair;
	typedef pred::CompareFirst< pred::CompareValue > TCompareKeyPair;

	for ( TKeyItemContainer::iterator itKeyItem = scopedKeyItems.begin(), itEnd = scopedKeyItems.end(); itKeyItem != itEnd; )
	{
		IteratorPair itPair = std::equal_range( itKeyItem, itEnd, *itKeyItem, pred::LessValue< TCompareKeyPair >() );		// range of items with same content key -> make new group if more than 1 item
		ASSERT( itPair.first == itKeyItem );
		size_t itemCount = std::distance( itPair.first, itPair.second );

		if ( itemCount > 1 )			// has multiple duplicates?
		{
			CDuplicateFilesGroup* pNewGroup = new CDuplicateFilesGroup( itPair.first->first );

			for ( itKeyItem = itPair.first; itKeyItem != itPair.second; ++itKeyItem  )
				pNewGroup->AddItem( utl::ReleaseOwnership( itKeyItem->second ) );

			rDuplicateGroups.push_back( pNewGroup );
			ENSURE( pNewGroup->HasDuplicates() );
		}
		else
			++itKeyItem;
	}
}

void CDuplicateFilesGroup::__ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress /*= NULL*/ ) throws_( CUserAbortedException )
{
	// old version using group stores

	REQUIRE( HasDuplicates() );
	REQUIRE( 0 == m_contentKey.m_crc32 );			// CRC32 is yet to be computed

	utl::COwningContainer< std::vector< CDuplicateFileItem* > > scopedItems;
	scopedItems.swap( m_items );	// take scoped ownership for exception safety

	CDuplicateGroupStore store;

	for ( std::vector< CDuplicateFileItem* >::iterator itItem = scopedItems.begin(); itItem != scopedItems.end(); ++itItem )
	{
		if ( pProgress != NULL )
			pProgress->AdvanceItem( ( *itItem )->GetFilePath().Get() );

		fs::CFileContentKey fullKey = m_contentKey;
		if ( fullKey.ComputeCrc32( ( *itItem )->GetFilePath() ) )
			store.RegisterItem( utl::ReleaseOwnership( *itItem ), fullKey );		// release item ownership to prevent being deleted when unwiding the stack
		else
			++rIgnoredCount;		// remaining items will be deleted when unwiding the stack
	}

	ENSURE( m_items.empty() );		// all items will be detached in real duplicate groups or discarded
	store.ExtractDuplicateGroups( rDuplicateGroups, rIgnoredCount, NULL );			// no progress reporting, it's fast once the slow CRC32 has been computed
}


// CDuplicateGroupStore implementation

CDuplicateGroupStore::~CDuplicateGroupStore( void )
{
	utl::ClearOwningContainer( m_groups );
}

size_t CDuplicateGroupStore::GetDuplicateItemCount( const std::vector< CDuplicateFilesGroup* >& groups )
{
	size_t dupItemCount = 0;

	for ( std::vector< CDuplicateFilesGroup* >::const_iterator itGroup = groups.begin(); itGroup != groups.end(); ++itGroup )
		if ( ( *itGroup )->HasDuplicates() )
			dupItemCount += ( *itGroup )->GetItems().size();

	return dupItemCount;
}

bool CDuplicateGroupStore::RegisterPath( const fs::CPath& filePath, const fs::CFileContentKey& contentKey )
{
	CDuplicateFilesGroup*& rpGroup = m_groupsMap[ contentKey ];

	if ( NULL == rpGroup )
	{
		rpGroup = new CDuplicateFilesGroup( contentKey );
		m_groups.push_back( rpGroup );
	}
	else if ( rpGroup->ContainsItem( filePath ) )		// already in the group?
		return false;									// file path not unique: a secondary occurrence due to overlapping search directories

	rpGroup->AddItem( filePath );
	return true;
}

void CDuplicateGroupStore::RegisterItem( CDuplicateFileItem* pItem, const fs::CFileContentKey& contentKey )
{
	ASSERT_PTR( pItem );
	ASSERT( contentKey.m_crc32 != 0 );

	CDuplicateFilesGroup*& rpGroup = m_groupsMap[ contentKey ];

	if ( NULL == rpGroup )
	{
		rpGroup = new CDuplicateFilesGroup( contentKey );
		m_groups.push_back( rpGroup );
	}

	rpGroup->AddItem( pItem );
}

void CDuplicateGroupStore::ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress /*= NULL*/ ) throws_( CUserAbortedException )
{
	utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > scopedGroups;
	scopedGroups.swap( m_groups );			// take scoped ownership for exception safety

	size_t dupGroupIndex = 0;
	for ( std::vector< CDuplicateFilesGroup* >::iterator itGroup = scopedGroups.begin(); itGroup != scopedGroups.end(); ++itGroup )
		if ( ( *itGroup )->HasDuplicates() )
		{
			if ( ( *itGroup )->HasCrc32() )								// checksum computed?
			{
				if ( pProgress != NULL )
					pProgress->AdvanceStage( ( *itGroup )->GetContentKey().Format() );

				rDuplicateGroups.push_back( *itGroup );
				*itGroup = NULL;										// mark detached group as NULL to prevent being deleted when unwiding the stack
			}
			else
			{
				std::vector< CDuplicateFilesGroup* > subGroups;			// grouped by file-size *and* CRC32
				( *itGroup )->ExtractCrc32Duplicates( subGroups, rIgnoredCount, pProgress );
				rDuplicateGroups.insert( rDuplicateGroups.end(), subGroups.begin(), subGroups.end() );
			}

			++dupGroupIndex;
		}

	m_groupsMap.clear();		// cleanup the empty store
}
