#ifndef DuplicateFileItem_h
#define DuplicateFileItem_h
#pragma once

#include "FileContent.h"
#include "FileStateItem.h"
#include <hash_map>


namespace utl { interface IProgressService; }
class CDuplicateFilesGroup;


class CDuplicateFileItem : public CFileStateItem
{
public:
	CDuplicateFileItem( const CFileFind& foundFile );
	CDuplicateFileItem( const fs::CPath& filePath, CDuplicateFilesGroup* pParentGroup );	// legacy

	fs::CFileContentKey GetContentKey( void ) const { return fs::CFileContentKey( GetState() ); }

	CDuplicateFilesGroup* GetParentGroup( void ) const { return m_pParentGroup; }
	void SetParentGroup( CDuplicateFilesGroup* pParentGroup ) { m_pParentGroup = pParentGroup; }

	bool IsOriginalItem( void ) const;				// first item in the group?
	bool IsDuplicateItem( void ) const { return !IsOriginalItem(); }
	bool MakeOriginalItem( void );
	bool MakeDuplicateItem( void );

//	const CTime& GetModifyTime( void ) const { return GetState().m_modifTime; }

//	struct AsModifyTime
//	{
//		const CTime& operator()( const CDuplicateFileItem* pItem ) const { return pItem->GetModifyTime(); }
//	};
private:
	CDuplicateFilesGroup* m_pParentGroup;
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

/**/	void AddItem( const fs::CPath& filePath );
	void AddItem( CDuplicateFileItem* pDupItem );

	// step 2 CRC32 evaluation and regrouping
	void ExtractChecksumDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException );

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

	CDuplicateFilesGroup* RegisterItem( CDuplicateFileItem* pDupItem );

/**/	bool RegisterPath( const fs::CPath& filePath, const fs::CFileContentKey& contentKey );
/**/	void RegisterItem( CDuplicateFileItem* pItem, const fs::CFileContentKey& contentKey );

	// extract groups with more than 1 item
	void ExtractDuplicateGroups( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException );
private:
	stdext::hash_map< fs::CFileContentKey, CDuplicateFilesGroup* > m_groupsMap;
	std::vector< CDuplicateFilesGroup* > m_groups;				// with ownership, in the order they were registered
};


namespace func
{
	struct AsGroupFileSize
	{
		UINT64 operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetContentKey().m_fileSize; }
	};

	struct AsGroupCrc32
	{
		UINT operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetContentKey().m_crc32; }
	};

	struct AsGroupDuplicateCount
	{
		size_t operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetItems().size() - 1; }
	};


	struct AsOriginalItem
	{
		const CDuplicateFileItem* operator()( const CDuplicateFilesGroup* pDupGroup ) const
		{
			ASSERT_PTR( pDupGroup );
			return pDupGroup->GetOriginalItem();
		}
	};

	struct AsOriginalItemPath
	{
		const fs::CPath& operator()( const CDuplicateFilesGroup* pDupGroup ) const
		{
			ASSERT_PTR( pDupGroup );
			return pDupGroup->GetOriginalItem()->GetFilePath();
		}
	};
}


namespace pred
{
	typedef CompareAdapter<CompareNaturalPath, func::AsOriginalItemPath> TCompareOriginalItemPath;
	typedef LessValue<TCompareOriginalItemPath> TLess_OriginalItemPath;
}


namespace utl
{
	inline void SortDuplicateGroupItems( std::vector< CDuplicateFilesGroup* >& rDupGroupItems, bool ascending = true )
	{
		utl::SortPathItems<pred::TCompareOriginalItemPath>( rDupGroupItems, ascending );		// sort groups by original item path
	}
}


#endif // DuplicateFileItem_h
