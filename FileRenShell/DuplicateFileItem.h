#ifndef DuplicateFileItem_h
#define DuplicateFileItem_h
#pragma once

#include "PathItemBase.h"


namespace utl { class CCRC32FileCache; }


struct CFileContentKey
{
	CFileContentKey( void ) : m_fileSize( 0ull ), m_crc32( 0u ) {}
	CFileContentKey( ULONGLONG fileSize, UINT crc32 ) : m_fileSize( fileSize ), m_crc32( crc32 ) { ASSERT( m_fileSize != 0ull && m_crc32 != 0u ); }

	bool Compute( const fs::CPath& filePath );
	static utl::CCRC32FileCache& GetCRC32FileCache( void );

	bool operator==( const CFileContentKey& right ) const { return m_fileSize == right.m_fileSize && m_crc32 == right.m_crc32; }
	bool operator<( const CFileContentKey& right ) const;
public:
	ULONGLONG m_fileSize;	// in bytes
	UINT m_crc32;			// CRC32 checksum
};


class CDuplicateFileItem;


class CDuplicateFilesGroup
{
public:
	CDuplicateFilesGroup( const CFileContentKey& contentKey ) : m_contentKey( contentKey ) {}
	~CDuplicateFilesGroup();

	size_t GetGroupPos( void ) const { return m_groupPos; }
	CFileContentKey GetContentKey( void ) const { return m_contentKey; }

	const std::vector< CDuplicateFileItem* >& GetItems( void ) const { return m_items; }
	CDuplicateFileItem* FindItem( const fs::CPath& filePath ) const;

	void AddItem( const fs::CPath& filePath );
private:
	size_t m_groupPos;
	CFileContentKey m_contentKey;
	std::vector< CDuplicateFileItem* > m_items;
};


class CDuplicateFileItem : public CPathItemBase
{
public:
	CDuplicateFileItem( const fs::CPath& keyPath, const CDuplicateFilesGroup* pParentGroup );

	const CDuplicateFilesGroup* GetParentGroup( void ) const { return m_pParentGroup; }
	bool IsOriginalItem( void ) const { return this == m_pParentGroup->GetItems().front(); }
	bool IsDuplicateItem( void ) const { return !IsOriginalItem(); }
private:
	const CDuplicateFilesGroup* m_pParentGroup;
};


class CDuplicateGroupsStore
{
public:
	CDuplicateGroupsStore( void ) {}
	~CDuplicateGroupsStore( void );

	bool Register( const fs::CPath& filePath );
	void ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups );		// extract groups with more than 1 item (multiple duplicates sharing the same content key)
private:
	std::map< CFileContentKey, CDuplicateFilesGroup* > m_groupsMap;
	std::vector< CDuplicateFilesGroup* > m_groups;				// with ownership, in the order they were registered
};


#endif // DuplicateFileItem_h