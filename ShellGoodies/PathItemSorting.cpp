
#include "stdafx.h"
#include "PathItemSorting.h"
#include "RenameItem.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pred
{
	CompareRenameItem::CompareRenameItem( const ren::TSortingPair& sortingPair )
		: m_sortingPair( sortingPair )
		, m_pComparator( ren::CRenameItemCriteria::Instance()->GetComparator( m_sortingPair.first ) )
		, m_pRecordComparator( ren::CRenameItemCriteria::Instance()->GetComparator( ren::RecordDefault ) )
	{
		ASSERT_PTR( m_pComparator );
		ASSERT_PTR( m_pRecordComparator );
	}

	CompareResult CompareRenameItem::operator()( const CRenameItem* pLeft, const CRenameItem* pRight ) const
	{
		CompareResult result = pred::GetResultInOrder( m_pComparator->CompareObjects( pLeft, pRight ), m_sortingPair.second );

		if ( Equal == result && m_pComparator != m_pRecordComparator )
			result = m_pRecordComparator->CompareObjects( pLeft, pRight );

		return result;
	}
}


namespace ren
{
	// CRenameItemCriteria implementation

	CRenameItemCriteria::CRenameItemCriteria( void )
	{
		m_comparators.push_back( pred::NewComparatorAs<CRenameItem>( pred::TComparePathItemDirsFirst() ) );	// RecordDefault (-1)
		m_comparators.push_back( pred::NewPropertyComparator<CRenameItem>( func::AsSrcPath() ) );			// SrcPath
		m_comparators.push_back( pred::NewPropertyComparator<CRenameItem>( func::AsFileSize() ) );			// SrcSize
		m_comparators.push_back( pred::NewPropertyComparator<CRenameItem>( func::AsModifyTime() ) );		// SrcDateModify
		m_comparators.push_back( pred::NewPropertyComparator<CRenameItem>( func::AsDestPath() ) );			// DestPath
	}

	CRenameItemCriteria* CRenameItemCriteria::Instance( void )
	{
		static CRenameItemCriteria s_comparators;
		return &s_comparators;
	}

	void CRenameItemCriteria::Clear( void )
	{
		utl::ClearOwningContainer( m_comparators );
	}


	namespace ui
	{
		const CEnumTags& GetTags_UiSortBy( void )
		{
			static const CEnumTags s_tags(
				_T("Default|File Path|File Size|File Date Modified|Destination Path|(File Path)|(File Size)|(File Date Modified)|(Destination Path)"),
				// use XML tags for combo tooltips
				L"Sort by source file path, directories-first|Sort by file path|Sort by file size|Sort by file last modify date|Sort by destination file path|"
				L"Sort descending by file path|Sort descending by file size|Sort descending by file last modify date|Sort descending by destination file path"
			);
			return s_tags;
		}

		UiSortBy FromSortingPair( const ren::TSortingPair& sortingPair )
		{
			switch ( sortingPair.first )
			{
				default:
					ASSERT( false );
				case ren::RecordDefault:
					return Default;
				case ren::SrcPath:
					return sortingPair.second ? SrcPathAsc : SrcPathDesc;
				case ren::SrcSize:
					return sortingPair.second ? SrcSizeAsc : SrcSizeDesc;
				case ren::SrcDateModify:
					return sortingPair.second ? SrcDateModifyAsc : SrcDateModifyDesc;
				case ren::DestPath:
					return sortingPair.second ? DestPathAsc : DestPathDesc;
			}
		}

		ren::TSortingPair ToSortingPair( UiSortBy uiSortBy )
		{
			switch ( uiSortBy )
			{
				default:
					ASSERT( false );
				case Default:
					return ren::TSortingPair( ren::RecordDefault, true );
				case SrcPathAsc:
					return ren::TSortingPair( ren::SrcPath, true );
				case SrcSizeAsc:
					return ren::TSortingPair( ren::SrcSize, true );
				case SrcDateModifyAsc:
					return ren::TSortingPair( ren::SrcDateModify, true );
				case DestPathAsc:
					return ren::TSortingPair( ren::DestPath, true );
				case SrcPathDesc:
					return ren::TSortingPair( ren::SrcPath, false );
				case SrcSizeDesc:
					return ren::TSortingPair( ren::SrcSize, false );
				case SrcDateModifyDesc:
					return ren::TSortingPair( ren::SrcDateModify, false );
				case DestPathDesc:
					return ren::TSortingPair( ren::DestPath, false );
			}
		}
	}
}
