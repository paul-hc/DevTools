#ifndef PathItemSorting_h
#define PathItemSorting_h
#pragma once

#include "utl/SubjectPredicates.h"
#include "utl/FileSystem.h"
#include "utl/PathItemBase.h"
#include "Application_fwd.h"


class CRenameItem;


namespace pred
{
	typedef CompareAdapterPtr<TComparePathDirsFirst, CPathItemBase::ToFilePath> TComparePathItemDirsFirst;


	struct CompareRenameItem
	{
		CompareRenameItem( const ren::TSortingPair& sortingPair );

		pred::CompareResult operator()( const CRenameItem* pLeft, const CRenameItem* pRight ) const;
	public:
		ren::TSortingPair m_sortingPair;
		const pred::IComparator* m_pComparator;
		const pred::IComparator* m_pRecordComparator;
	};
}


namespace ren
{
	class CRenameItemCriteria		// container of comparators for each sort order, including RecordDefault record order; shared by file model and list-ctrls
	{
		CRenameItemCriteria( void );
		~CRenameItemCriteria() { Clear(); }

		const pred::IComparator* GetAtPos( size_t orderPos ) const { ASSERT( orderPos < m_comparators.size() ); return m_comparators[ orderPos ]; }
	public:
		static CRenameItemCriteria* Instance( void );

		void Clear( void );

		const pred::IComparator* GetComparator( SortBy sortBy ) const
		{
			return GetAtPos( static_cast<size_t>( sortBy + 1 ) );	// offset by 1 to account for -1 position of RecordDefault
		}
	private:
		std::vector< pred::IComparator* > m_comparators;
	};


	namespace ui
	{
		enum UiSortBy { Default, SrcPathAsc, SrcSizeAsc, SrcDateModifyAsc, DestPathAsc, SrcPathDesc, SrcSizeDesc, SrcDateModifyDesc, DestPathDesc };	// combo enumeration

		const CEnumTags& GetTags_UiSortBy( void );

		UiSortBy FromSortingPair( const ren::TSortingPair& sortingPair );
		ren::TSortingPair ToSortingPair( UiSortBy uiSortBy );
	}
}


#endif // PathItemSorting_h
