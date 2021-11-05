
#include "stdafx.h"
#include "DuplicateFileItem.h"
#include "ContainerUtilities.h"
#include "Crc32.h"
#include "ComparePredicates.h"
#include "FileSystem.h"
#include "IProgressService.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDuplicateFileItem implementation

CDuplicateFileItem::CDuplicateFileItem( const CFileFind& foundFile )
	: CFileStateItem( foundFile )
	, m_pParentGroup( NULL )
{
}

bool CDuplicateFileItem::IsOriginalItem( void ) const
{
	ASSERT_PTR( m_pParentGroup );
	return this == m_pParentGroup->GetItems().front();
}

bool CDuplicateFileItem::MakeOriginalItem( void )
{
	ASSERT_PTR( m_pParentGroup );
	return m_pParentGroup->MakeOriginalItem( this );
}

bool CDuplicateFileItem::MakeDuplicateItem( void )
{
	ASSERT_PTR( m_pParentGroup );
	return m_pParentGroup->MakeDuplicateItem( this );
}


// CDuplicateFilesGroup implementation

CDuplicateFilesGroup::~CDuplicateFilesGroup()
{
	utl::ClearOwningContainer( m_items );
}

void CDuplicateFilesGroup::SortDuplicates( void )
{
	if ( HasDuplicates() )
		std::sort( m_items.begin() + 1, m_items.end(), pred::TLess_PathItem() );
}

void CDuplicateFilesGroup::AddItem( CDuplicateFileItem* pDupItem )
{
	ASSERT_PTR( pDupItem );
	ASSERT( !ContainsItem( pDupItem->GetFilePath() ) );

	m_items.push_back( pDupItem );
	pDupItem->SetParentGroup( this );
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
	SortDuplicates();
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
	SortDuplicates();
	return true;
}

void CDuplicateFilesGroup::ExtractChecksumDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException )
{
	REQUIRE( HasDuplicates() );
	REQUIRE( 0 == m_contentKey.m_crc32 );			// CRC32 is yet to be computed
	ASSERT_PTR( pProgressSvc );

	typedef std::pair< fs::CFileContentKey, CDuplicateFileItem* > TKeyItemPair;
	typedef utl::COwningContainer< std::vector<TKeyItemPair>, func::DeleteValue > TKeyItemContainer;

	TKeyItemContainer scopedKeyItems;
	scopedKeyItems.reserve( m_items.size() );

	for ( std::vector< CDuplicateFileItem* >::iterator itItem = m_items.begin(); itItem != m_items.end(); ++itItem )
	{
		pProgressSvc->AdvanceItem( ( *itItem )->GetFilePath().Get() );

		TKeyItemPair newItem( m_contentKey, *itItem );

		newItem.first.StoreCrc32( newItem.second->GetState(), true );		// lazy evaluate CRC32 with caching
		if ( newItem.first.HasCrc32() )
		{
			pProgressSvc->AdvanceStage( newItem.first.Format() );

			scopedKeyItems.push_back( newItem );
			utl::ReleaseOwnership( *itItem );		// release detached item ownership to prevent being deleted when unwiding the stack
		}
		else
		{
			delete *itItem;
			++rIgnoredCount;			// remaining items will be deleted when unwiding the stack
		}
	}

	m_items.clear();					// ownership was passed to scopedKeyItems

	typedef pred::CompareFirstSecond< pred::CompareValue, pred::ComparePathItem > TCompareKeyItemPair;

	func::SortPathItems<TCompareKeyItemPair>( scopedKeyItems );

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

CDuplicateFilesGroup* CDuplicateGroupStore::RegisterItem( CDuplicateFileItem* pDupItem )
{
	ASSERT_PTR( pDupItem );

	fs::CFileContentKey contentKey = pDupItem->GetContentKey();
	CDuplicateFilesGroup*& rpGroup = m_groupsMap[ contentKey ];

	if ( NULL == rpGroup )
	{
		rpGroup = new CDuplicateFilesGroup( contentKey );
		m_groups.push_back( rpGroup );
	}

	rpGroup->AddItem( pDupItem );
	return rpGroup;
}

void CDuplicateGroupStore::ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException )
{
	ASSERT_PTR( pProgressSvc );

	utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > scopedGroups;
	scopedGroups.swap( m_groups );			// take scoped ownership for exception safety

	for ( size_t i = 0; i != scopedGroups.size(); ++i )
	{
		CDuplicateFilesGroup* pGroup = scopedGroups[ i ];

		if ( pGroup->HasDuplicates() )
		{
			if ( pGroup->HasCrc32() )								// checksum computed?
			{
				pProgressSvc->AdvanceStage( pGroup->GetContentKey().Format() );

				rDuplicateGroups.push_back( pGroup );
				scopedGroups[ i ] = NULL;							// mark detached group as NULL to prevent being deleted when unwiding the stack
			}
			else
			{
				std::vector< CDuplicateFilesGroup* > subGroups;		// grouped by file-size *and* CRC32

				pGroup->ExtractChecksumDuplicates( subGroups, rIgnoredCount, pProgressSvc );
				rDuplicateGroups.insert( rDuplicateGroups.end(), subGroups.begin(), subGroups.end() );
			}
		}
	}

	m_groupsMap.clear();		// cleanup the empty store

	utl::for_each( rDuplicateGroups, func::SortGroupDuplicates() );		// sort each group's duplicate items by path
}
