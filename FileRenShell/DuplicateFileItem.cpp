
#include "stdafx.h"
#include "DuplicateFileItem.h"
#include "utl/ContainerUtilities.h"
#include "utl/Crc32.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileContentKey implementation

bool CFileContentKey::operator<( const CFileContentKey& right ) const
{
	if ( m_fileSize < right.m_fileSize )
		return true;
	else if ( m_fileSize == right.m_fileSize )
		return m_crc32 < right.m_crc32;

	return false;
}

bool CFileContentKey::ComputeFileSize( const fs::CPath& filePath )
{
	m_fileSize = fs::GetFileSize( filePath.GetPtr() );
	return m_fileSize != ULLONG_MAX;
}

bool CFileContentKey::ComputeCrc32( const fs::CPath& filePath )
{
	m_crc32 = GetCrc32FileCache().AcquireCrc32( filePath );
	return m_crc32 != 0;
}

utl::CCrc32FileCache& CFileContentKey::GetCrc32FileCache( void )
{
	static utl::CCrc32FileCache crcCache;
	return crcCache;
}


// CDuplicateFilesGroup implementation

CDuplicateFilesGroup::~CDuplicateFilesGroup()
{
	utl::ClearOwningContainer( m_items );
}

CDuplicateFileItem* CDuplicateFilesGroup::FindItem( const fs::CPath& filePath ) const
{
	for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = m_items.begin(); itItem != m_items.end(); ++itItem )
		if ( filePath == ( *itItem )->GetKeyPath() )
			return *itItem;

	return NULL;
}

void CDuplicateFilesGroup::AddItem( const fs::CPath& filePath )
{
	ASSERT( !ContainsItem( filePath ) );

	m_items.push_back( new CDuplicateFileItem( filePath, this ) );
}

void CDuplicateFilesGroup::AddItem( CDuplicateFileItem* pItem )
{
	ASSERT_PTR( pItem );
	ASSERT( !ContainsItem( pItem->GetKeyPath() ) );

	m_items.push_back( pItem );
	pItem->SetParentGroup( this );
}

void CDuplicateFilesGroup::ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount )
{
	CDuplicateGroupsStore store;

	for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = m_items.begin(); itItem != m_items.end(); ++itItem )
	{
		CFileContentKey fullContentKey = m_contentKey;
		if ( fullContentKey.ComputeCrc32( ( *itItem )->GetKeyPath() ) )
			VERIFY( store.Register( ( *itItem )->GetKeyPath(), fullContentKey ) );
		else
			++rIgnoredCount;
	}

	m_items.clear();		// all items were detached in real duplicate groups
	store.ExtractDuplicateGroups( rDuplicateGroups, rIgnoredCount );
}


// CDuplicateFileItem implementation

CDuplicateFileItem::CDuplicateFileItem( const fs::CPath& filePath, const CDuplicateFilesGroup* pParentGroup )
	: CPathItemBase( filePath )
	, m_pParentGroup( pParentGroup )
	, m_modifyTime( fs::ReadLastModifyTime( filePath ) )
{
	ASSERT_PTR( m_pParentGroup );
}


// CDuplicateGroupsStore implementation

CDuplicateGroupsStore::~CDuplicateGroupsStore( void )
{
	utl::ClearOwningContainer( m_groups );
}

bool CDuplicateGroupsStore::Register( const fs::CPath& filePath, const CFileContentKey& contentKey )
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

void CDuplicateGroupsStore::Register( CDuplicateFileItem* pItem, const CFileContentKey& contentKey )
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

void CDuplicateGroupsStore::ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount )
{
	for ( std::vector< CDuplicateFilesGroup* >::iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); )
		if ( ( *itGroup )->HasDuplicates() )
		{
			if ( ( *itGroup )->HasCrc32() )								// checksum computed?
				rDuplicateGroups.push_back( *itGroup );
			else
			{
				std::vector< CDuplicateFilesGroup* > subGroups;			// grouped by file-size *and* CRC32
				( *itGroup )->ExtractCrc32Duplicates( subGroups, rIgnoredCount );
				rDuplicateGroups.insert( rDuplicateGroups.end(), subGroups.begin(), subGroups.end() );

				delete *itGroup;
			}

			itGroup = m_groups.erase( itGroup );
		}
		else
			++itGroup;
}
