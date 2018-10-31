
#include "stdafx.h"
#include "DuplicateFileItem.h"
#include "utl/ContainerUtilities.h"
#include "utl/Crc32.h"
#include "utl/FileSystem.h"
#include "utl/StringUtilities.h"

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


// CDuplicateFileItem implementation

CDuplicateFileItem::CDuplicateFileItem( const fs::CPath& filePath, const CDuplicateFilesGroup* pParentGroup )
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

std::tstring CDuplicateFilesGroup::FormatContentKey( size_t groupIndex ) const
{
	std::tstring text = str::Format( _T("Group no. %d, file-size=%s"),
		groupIndex + 1,
		num::FormatFileSize( m_contentKey.m_fileSize, num::Bytes ).c_str(),
		m_contentKey.m_crc32 );

	if ( m_contentKey.m_crc32 != 0 )
		text += str::Format( _T(", CRC32=%X") );

	return text;
}

void CDuplicateFilesGroup::ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressBox* pProgressBox /*= NULL*/ ) throws_( CUserAbortedException )
{
	utl::COwningContainer< std::vector< CDuplicateFileItem* > > items;
	items.swap( m_items );			// take scoped ownership for exception safety

	CDuplicateGroupsStore store;

	for ( std::vector< CDuplicateFileItem* >::iterator itItem = items.begin(); itItem != items.end(); ++itItem )
	{
		if ( pProgressBox != NULL )
			pProgressBox->AdvanceStepItem( ( *itItem )->GetKeyPath().Get() );

		CFileContentKey fullKey = m_contentKey;
		if ( fullKey.ComputeCrc32( ( *itItem )->GetKeyPath() ) )
		{
			store.RegisterItem( *itItem, fullKey );
			*itItem = NULL;			// mark detached item as NULL to prevent being deleted when exiting the scope
		}
		else
			++rIgnoredCount;		// remaining items will be deleted when exiting the scope
	}

	ENSURE( m_items.empty() );		// all items will be detached in real duplicate groups or discarded
	store.ExtractDuplicateGroups( rDuplicateGroups, rIgnoredCount, NULL );			// no progress reporting, it's fast once the slow CRC32 has been computed
}


// CDuplicateGroupsStore implementation

CDuplicateGroupsStore::~CDuplicateGroupsStore( void )
{
	utl::ClearOwningContainer( m_groups );
}

size_t CDuplicateGroupsStore::GetTotalDuplicateItemCount( void ) const
{
	size_t dupItemCount = 0;

	for ( std::vector< CDuplicateFilesGroup* >::const_iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); ++itGroup )
		if ( ( *itGroup )->HasDuplicates() )
			dupItemCount += ( *itGroup )->GetItems().size();

	return dupItemCount;
}

bool CDuplicateGroupsStore::RegisterPath( const fs::CPath& filePath, const CFileContentKey& contentKey )
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

void CDuplicateGroupsStore::RegisterItem( CDuplicateFileItem* pItem, const CFileContentKey& contentKey )
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

void CDuplicateGroupsStore::ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressBox* pProgressBox /*= NULL*/ ) throws_( CUserAbortedException )
{
	size_t dupGroupIndex = 0;
	for ( std::vector< CDuplicateFilesGroup* >::iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); )
		if ( ( *itGroup )->HasDuplicates() )
		{
			if ( pProgressBox != NULL )
				pProgressBox->AdvanceStage( ( *itGroup )->FormatContentKey( dupGroupIndex ) );

			if ( ( *itGroup )->HasCrc32() )								// checksum computed?
				rDuplicateGroups.push_back( *itGroup );
			else
			{
				std::vector< CDuplicateFilesGroup* > subGroups;			// grouped by file-size *and* CRC32
				( *itGroup )->ExtractCrc32Duplicates( subGroups, rIgnoredCount, pProgressBox );
				rDuplicateGroups.insert( rDuplicateGroups.end(), subGroups.begin(), subGroups.end() );

				delete *itGroup;
			}

			itGroup = m_groups.erase( itGroup );
			++dupGroupIndex;
		}
		else
			++itGroup;
}
