#ifndef DuplicateFileItem_h
#define DuplicateFileItem_h
#pragma once

#include "FileContent.h"
#include "FileStateItem.h"
#include <unordered_map>


namespace utl { interface IProgressService; }
class CDuplicateFilesGroup;


class CDuplicateFileItem : public CFileStateItem
{
public:
	CDuplicateFileItem( const fs::CFileState& fileState );

	fs::CFileContentKey GetContentKey( void ) const { return fs::CFileContentKey( GetState() ); }

	CDuplicateFilesGroup* GetParentGroup( void ) const { return m_pParentGroup; }
	void SetParentGroup( CDuplicateFilesGroup* pParentGroup ) { m_pParentGroup = pParentGroup; }

	bool IsOriginalItem( void ) const;				// first item in the group?
	bool IsDuplicateItem( void ) const { return !IsOriginalItem(); }
	bool MakeOriginalItem( void );
	bool MakeDuplicateItem( void );
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
	bool HasCrc32( void ) const { return m_contentKey.HasCrc32(); }

	const std::vector<CDuplicateFileItem*>& GetItems( void ) const { return m_items; }
	size_t GetDuplicatesCount( void ) const { ASSERT( !m_items.empty() ); return m_items.size() - 1; }		// excluding the original item

	CDuplicateFileItem* GetOriginalItem( void ) const { return !m_items.empty() ? m_items.front() : nullptr; }
	CDuplicateFileItem* FindItem( const fs::CPath& filePath ) const { return func::FindItemWithPath( m_items, filePath ); }
	bool ContainsItem( const fs::CPath& filePath ) const { return FindItem( filePath ) != nullptr; }

	void AddItem( CDuplicateFileItem* pDupItem );
	void SortDuplicates( void );		//  keep original first, sort duplicate items by path

	// lazy CRC32 evaluation and regrouping
	void ExtractChecksumDuplicates( std::vector<CDuplicateFilesGroup*>& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException );

	bool MakeOriginalItem( CDuplicateFileItem* pItem );
	bool MakeDuplicateItem( CDuplicateFileItem* pItem );

	template< typename CompareT >
	const CDuplicateFileItem* GetSortingItem( CompareT compare )
	{
		return *std::min_element( m_items.begin(), m_items.end(), pred::MakeLessPtr( compare ) );		// sort ascending - lowest value; sort descending: highest value
	}
private:
	fs::CFileContentKey m_contentKey;
	std::vector<CDuplicateFileItem*> m_items;
};


class CDuplicateGroupStore
{
public:
	CDuplicateGroupStore( void ) {}
	~CDuplicateGroupStore( void );

	size_t GetDuplicateItemCount( void ) const { return GetDuplicateItemCount( m_groups ); }
	static size_t GetDuplicateItemCount( const std::vector<CDuplicateFilesGroup*>& groups );

	CDuplicateFilesGroup* RegisterItem( CDuplicateFileItem* pDupItem );

	// extract groups with more than 1 item
	void ExtractDuplicateGroups( std::vector<CDuplicateFilesGroup*>& rDuplicateGroups, size_t& rIgnoredCount, utl::IProgressService* pProgressSvc ) throws_( CUserAbortedException );
private:
	std::unordered_map<fs::CFileContentKey, CDuplicateFilesGroup*> m_groupsMap;
	std::vector<CDuplicateFilesGroup*> m_groups;				// with ownership, in the order they were registered
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


namespace func
{
	struct SortGroupDuplicates
	{
		void operator()( CDuplicateFilesGroup* pDupGroup ) const
		{
			ASSERT_PTR( pDupGroup );
			pDupGroup->SortDuplicates();
		}
	};


	inline void SortDuplicateGroupItems( std::vector<CDuplicateFilesGroup*>& rDupGroupItems, bool ascending = true )
	{
		func::SortPathItems<pred::TCompareOriginalItemPath>( rDupGroupItems, ascending );		// sort groups by original item path
	}
}


#endif // DuplicateFileItem_h
