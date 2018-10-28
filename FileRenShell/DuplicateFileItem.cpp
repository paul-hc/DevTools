
#include "stdafx.h"
#include "DuplicateFileItem.h"
#include "utl/ContainerUtilities.h"
#include "utl/CRC32.h"
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

bool CFileContentKey::Compute( const fs::CPath& filePath )
{
	m_fileSize = fs::GetFileSize( filePath.GetPtr() );
	if ( ULLONG_MAX == m_fileSize )
		return false;

	m_crc32 = GetCRC32FileCache().AcquireCrc32( filePath );
	if ( 0 == m_crc32 )
		return false;

	return true;
}

utl::CCRC32FileCache& CFileContentKey::GetCRC32FileCache( void )
{
	static utl::CCRC32FileCache crcCache;
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


// CDuplicateFileItem implementation

CDuplicateFileItem::CDuplicateFileItem( const fs::CPath& keyPath, const CDuplicateFilesGroup* pParentGroup )
	: CPathItemBase( keyPath )
	, m_pParentGroup( pParentGroup )
{
	ASSERT_PTR( m_pParentGroup );
}


// CDuplicateGroupsStore implementation

CDuplicateGroupsStore::~CDuplicateGroupsStore( void )
{
	utl::ClearOwningContainer( m_groups );
}

bool CDuplicateGroupsStore::Register( const fs::CPath& filePath )
{
	CFileContentKey contentKey;
	if ( !contentKey.Compute( filePath ) )
		return false;

	CDuplicateFilesGroup*& rpGroup = m_groupsMap[ contentKey ];

	if ( NULL == rpGroup )
	{
		rpGroup = new CDuplicateFilesGroup( contentKey );
		m_groups.push_back( rpGroup );
	}
	else if ( rpGroup->ContainsItem( filePath ) )		// already in the group?
		return false;

	rpGroup->AddItem( filePath );
	return true;
}

void CDuplicateGroupsStore::ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ULONGLONG minFileSize /*= 0*/ )
{
	utl::ClearOwningContainer( rDuplicateGroups );
	rIgnoredCount = 0;

	for ( std::vector< CDuplicateFilesGroup* >::iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); )
		if ( ( *itGroup )->HasDuplicates() )
		{
			if ( ( *itGroup )->GetContentKey().m_fileSize >= minFileSize )			// has minimum size?
				rDuplicateGroups.push_back( *itGroup );
			else
			{
				rIgnoredCount += ( *itGroup )->GetItems().size();
				delete *itGroup;
			}

			itGroup = m_groups.erase( itGroup );
		}
		else
			++itGroup;
}
