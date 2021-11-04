#ifndef DuplicateFileItem_h
#define DuplicateFileItem_h
#pragma once

#include "FileContent.h"
#include "PathItemBase.h"
#include <hash_map>


namespace utl { interface IProgressService; }
class CDuplicateFilesGroup;


class CDuplicateFileItem : public CPathItemBase
{
public:
	CDuplicateFileItem( const fs::CPath& filePath, CDuplicateFilesGroup* pParentGroup );

	CDuplicateFilesGroup* GetParentGroup( void ) const { return m_pParentGroup; }
	void SetParentGroup( CDuplicateFilesGroup* pParentGroup ) { m_pParentGroup = pParentGroup; }

	bool IsOriginalItem( void ) const;				// first item in the group?
	bool IsDuplicateItem( void ) const { return !IsOriginalItem(); }
	bool MakeOriginalItem( void );
	bool MakeDuplicateItem( void );

	const CTime& GetModifyTime( void ) const { return m_modifyTime; }

	struct AsModifyTime
	{
		const CTime& operator()( const CDuplicateFileItem* pItem ) const { return pItem->GetModifyTime(); }
	};
private:
	CDuplicateFilesGroup* m_pParentGroup;
	CTime m_modifyTime;			// last modification time of file
};


// group: multiple duplicates sharing the same content key
//
class CDuplicateFilesGroup
{
public:
	CDuplicateFilesGroup( const fs::CFileContentKey& contentKey ) : m_contentKey( contentKey ) {}
	~CDuplicateFilesGroup();

	fs::CFileContentKey GetContentKey( void ) const { return m_contentKey; }

	bool HasDuplicates( void ) const { return m_items.size() > 1; }
	bool HasCrc32( void ) const { return m_contentKey.m_crc32 != 0; }

	const std::vector< CDuplicateFileItem* >& GetItems( void ) const { return m_items; }
	size_t GetDuplicatesCount( void ) const { ASSERT( !m_items.empty() ); return m_items.size() - 1; }		// excluding the original item

	CDuplicateFileItem* GetOriginalItem( void ) const { return !m_items.empty() ? m_items.front() : NULL; }
	CDuplicateFileItem* FindItem( const fs::CPath& filePath ) const;
	bool ContainsItem( const fs::CPath& filePath ) const { return FindItem( filePath ) != NULL; }

	void AddItem( const fs::CPath& filePath );
	void AddItem( CDuplicateFileItem* pItem );

	// step 2 CRC32 evaluation and regrouping
	void ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc = NULL ) throws_( CUserAbortedException );
	void __ExtractCrc32Duplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc = NULL ) throws_( CUserAbortedException );	// old version using group stores

	bool MakeOriginalItem( CDuplicateFileItem* pItem );
	bool MakeDuplicateItem( CDuplicateFileItem* pItem );

	template< typename CompareT >
	const CDuplicateFileItem* GetSortingItem( CompareT compare )
	{
		return *std::min_element( m_items.begin(), m_items.end(), pred::MakeLessPtr( compare ) );		// sort ascending - lowest value; sort descending: highest value
	}
private:
	fs::CFileContentKey m_contentKey;
	std::vector< CDuplicateFileItem* > m_items;
};


class CDuplicateGroupStore
{
public:
	CDuplicateGroupStore( void ) {}
	~CDuplicateGroupStore( void );

	size_t GetDuplicateItemCount( void ) const { return GetDuplicateItemCount( m_groups ); }
	static size_t GetDuplicateItemCount( const std::vector< CDuplicateFilesGroup* >& groups );

	bool RegisterPath( const fs::CPath& filePath, const fs::CFileContentKey& contentKey );
	void RegisterItem( CDuplicateFileItem* pItem, const fs::CFileContentKey& contentKey );

	// extract groups with more than 1 item
	void ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc = NULL ) throws_( CUserAbortedException );
private:
	stdext::hash_map< fs::CFileContentKey, CDuplicateFilesGroup* > m_groupsMap;
	std::vector< CDuplicateFilesGroup* > m_groups;				// with ownership, in the order they were registered
};


#endif // DuplicateFileItem_h
