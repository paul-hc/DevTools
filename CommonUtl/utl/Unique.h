#ifndef Unique_h
#define Unique_h
#pragma once

#include <iterator>
#include <unordered_set>
#include <type_traits>		// for std::hash
#include "StdHashValue.h"
#include "StringCompare.h"


namespace utl
{
	template< typename ValueT, typename HasherT = std::hash<ValueT>, typename KeyEqualT = std::equal_to<ValueT> >
	class CUniqueIndex : private utl::noncopyable
	{
	public:
		CUniqueIndex( void ) {}

		std::vector<ValueT>& GetDuplicates( void ) { return m_duplicates; }

		bool IsEmpty( void ) const
		{
			REQUIRE( m_duplicates.empty() || m_uniqueIndex.empty() );		// consistent?
			return m_uniqueIndex.empty();
		}

		void Clear( void )
		{
			m_uniqueIndex.clear();
			m_duplicates.clear();
		}

		template< typename ContainerT >
		bool Augment( IN OUT ContainerT* pDestItems, const ValueT& item )
		{
			ASSERT_PTR( pDestItems );

			if ( !m_uniqueIndex.insert( item ).second )		// item is not unique?
			{
				m_duplicates.push_back( item );
				return false;
			}

			pDestItems->insert( pDestItems->end(), item );
			return true;
		}

		template< typename ContainerT, typename SrcIteratorT >
		size_t AugmentItems( IN OUT ContainerT* pDestItems, SrcIteratorT itStart, SrcIteratorT itEnd )
		{
			size_t addedCount = 0;

			while ( itStart != itEnd )
				if ( Augment( pDestItems, *itStart++ ) )
					++addedCount;

			return addedCount;
		}

		template< typename ContainerT, typename SrcContainerT >
		inline size_t AugmentItems( IN OUT ContainerT* pDestItems, const SrcContainerT& newItems )
		{
			return AugmentItems( pDestItems, newItems.begin(), newItems.end() );
		}

		template< typename ContainerT >
		size_t Uniquify( OUT ContainerT* pDestItems )
		{
			ASSERT_PTR( pDestItems );

			// assume this index is maintained consistently!
			//ASSERT( IsEmpty() );		// prevent inconsistent state: should start fresh

			if ( pDestItems->empty() )
				return 0;				// no items to uniquify

			ContainerT uniqueItems;

			for ( typename ContainerT::const_iterator itItem = pDestItems->begin(), itEnd = pDestItems->end(); itItem != itEnd; ++itItem )
				Augment( &uniqueItems, *itItem );

			pDestItems->swap( uniqueItems );
			return m_duplicates.size();
		}
	private:
		std::unordered_set<ValueT, HasherT, KeyEqualT> m_uniqueIndex;
		std::vector<ValueT> m_duplicates;
	};
}


namespace utl
{
	// uniquify algorithms:

	template< typename ContainerT, typename SetT >
	size_t Uniquify( IN OUT ContainerT& rItems, IN OUT SetT& rUniqueSet, OUT ContainerT* pRemovedDups = static_cast<ContainerT*>( nullptr ) )
	{
		typedef typename ContainerT::value_type TValue;

		// note: we need to have a "struct std::hash<TValue>" specialized for TValue
		ContainerT uniqueItems;
		size_t duplicateCount = 0;

		for ( typename ContainerT::const_iterator itItem = rItems.begin(), itEnd = rItems.end(); itItem != itEnd; ++itItem )
			if ( rUniqueSet.insert( *itItem ).second )		// item is unique?
				uniqueItems.push_back( *itItem );
			else
			{
				++duplicateCount;

				if ( pRemovedDups != nullptr )
					pRemovedDups->insert( pRemovedDups->end(), *itItem );	// for owning container of pointers: allow client to delete the removed duplicates
			}

		rItems.swap( uniqueItems );
		return duplicateCount;
	}

	// using a standard hash set std::unordered_set<typename ContainerT::value_type>
	//
	template< typename ContainerT >
	size_t Uniquify( IN OUT ContainerT& rItems, OUT ContainerT* pRemovedDups = static_cast<ContainerT*>( nullptr ) )
	{
		typedef typename ContainerT::value_type TValue;

		std::unordered_set<TValue> uniqueSet;		// using a standard hash set
		return Uniquify( rItems, uniqueSet, pRemovedDups );
	}

	// using custom hash and equal_to functors
	//
	template< typename HasherT, typename KeyEqualT, typename ContainerT >
	size_t Uniquify( IN OUT ContainerT& rItems, OUT ContainerT* pRemovedDups = static_cast<ContainerT*>( nullptr ) )
	{
		typedef typename ContainerT::value_type TValue;

		std::unordered_set<TValue, HasherT, KeyEqualT> uniqueSet;		// using a custom hash set
		return Uniquify( rItems, uniqueSet, pRemovedDups );
	}
}


namespace str
{
	namespace ignore_case
	{
		// case-insensitive hashing
		typedef std::unordered_set<std::string, Hash, EqualTo> TUnorderedSet_String;
		typedef std::unordered_set<std::wstring, Hash, EqualTo> TUnorderedSet_WString;
	}
}


namespace path
{
	template< typename StrContainerT >
	inline size_t Uniquify( IN OUT StrContainerT& rItems, OUT StrContainerT* pRemovedDups = static_cast<StrContainerT*>( nullptr ) )
	{	// StrContainerT::value_type could be std::tstring, fs::CPath, fs::CFlexPath, etc.
		return utl::Uniquify< std::hash<fs::CPath>, std::equal_to<fs::CPath> >( rItems, pRemovedDups );		// apply fs::CPath hashing to std::tstring
	}
}


#endif // Unique_h
