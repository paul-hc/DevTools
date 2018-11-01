#ifndef DuplicateFileItem_h
#define DuplicateFileItem_h
#pragma once

#include "utl/IProgressCallback.h"
#include "PathItemBase.h"
#include <hash_map>


class CSrcPathItem : public CPathItemBase
{
public:
	CSrcPathItem( const fs::CPath& srcPath )
		: CPathItemBase( srcPath )
	{
		SetDisplayCode( srcPath.Get() );
	}
};


namespace utl { class CCrc32FileCache; }


struct CFileContentKey
{
	CFileContentKey( void ) : m_fileSize( 0ull ), m_crc32( 0u ) {}
	CFileContentKey( ULONGLONG fileSize, UINT crc32 ) : m_fileSize( fileSize ), m_crc32( crc32 ) { ASSERT( m_fileSize != 0ull && m_crc32 != 0u ); }

	bool ComputeFileSize( const fs::CPath& filePath );
	bool ComputeCrc32( const fs::CPath& filePath );

	bool operator==( const CFileContentKey& right ) const { return m_fileSize == right.m_fileSize && m_crc32 == right.m_crc32; }
	bool operator<( const CFileContentKey& right ) const;

	static utl::CCrc32FileCache& GetCrc32FileCache( void );
public:
	ULONGLONG m_fileSize;	// in bytes
	UINT m_crc32;			// CRC32 checksum
};


namespace stdext
{
	size_t hash_value( const CFileContentKey& key );
}


class CDuplicateFilesGroup;


class CDuplicateFileItem : public CPathItemBase
{
public:
	CDuplicateFileItem( const fs::CPath& filePath, const CDuplicateFilesGroup* pParentGroup );

	const CDuplicateFilesGroup* GetParentGroup( void ) const { return m_pParentGroup; }
	void SetParentGroup( const CDuplicateFilesGroup* pParentGroup ) { m_pParentGroup = pParentGroup; }

	bool IsOriginalItem( void ) const;				// first item in the group?
	bool IsDuplicateItem( void ) const { return !IsOriginalItem(); }

	const CTime& GetModifyTime( void ) const { return m_modifyTime; }

	struct AsModifyTime
	{
		const CTime& operator()( const CDuplicateFileItem* pItem ) const { return pItem->GetModifyTime(); }
	};
private:
	const CDuplicateFilesGroup* m_pParentGroup;
	CTime m_modifyTime;			// last modification time of file
};


// group: multiple duplicates sharing the same content key
//
class CDuplicateFilesGroup
{
public:
	CDuplicateFilesGroup( const CFileContentKey& contentKey ) : m_contentKey( contentKey ) {}
	~CDuplicateFilesGroup();

	CFileContentKey GetContentKey( void ) const { return m_contentKey; }

	bool HasDuplicates( void ) const { return m_items.size() > 1; }
	bool HasCrc32( void ) const { return m_contentKey.m_crc32 != 0; }

	const std::vector< CDuplicateFileItem* >& GetItems( void ) const { return m_items; }
	CDuplicateFileItem* FindItem( const fs::CPath& filePath ) const;
	bool ContainsItem( const fs::CPath& filePath ) const { return FindItem( filePath ) != NULL; }

	void AddItem( const fs::CPath& filePath );
	void AddItem( CDuplicateFileItem* pItem );

	// step 2 CRC32 evaluation and regrouping
	void ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress = NULL ) throws_( CUserAbortedException );
	void __ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress = NULL ) throws_( CUserAbortedException );	// old version using group stores

	std::tstring FormatContentKey( void ) const;
private:
	CFileContentKey m_contentKey;
	std::vector< CDuplicateFileItem* > m_items;
};


class CDuplicateGroupStore
{
public:
	CDuplicateGroupStore( void ) {}
	~CDuplicateGroupStore( void );

	size_t GetDuplicateItemCount( void ) const { return GetDuplicateItemCount( m_groups ); }
	static size_t GetDuplicateItemCount( const std::vector< CDuplicateFilesGroup* >& groups );

	bool RegisterPath( const fs::CPath& filePath, const CFileContentKey& contentKey );
	void RegisterItem( CDuplicateFileItem* pItem, const CFileContentKey& contentKey );

	// extract groups with more than 1 item
	void ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, ui::IProgressCallback* pProgress = NULL ) throws_( CUserAbortedException );
private:
	stdext::hash_map< CFileContentKey, CDuplicateFilesGroup* > m_groupsMap;
	std::vector< CDuplicateFilesGroup* > m_groups;				// with ownership, in the order they were registered
};


#endif // DuplicateFileItem_h
