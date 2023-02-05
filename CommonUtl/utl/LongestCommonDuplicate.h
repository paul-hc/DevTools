#ifndef LongestCommonDuplicate_h
#define LongestCommonDuplicate_h
#pragma once

#include "StringCompare.h"
#include "ComparePredicates.h"


namespace lcs
{
	// Use a suffix array to find the Longest Common Duplicate (LCD).
	// The sorted suffix array is iterated as a balanced suffix tree using Compare.

	template< typename CharType, typename Compare = pred::TCompareCase >
	class CSuffixTree
	{
		typedef std::basic_string<CharType> TString;
	public:
		CSuffixTree( Compare compareStr = Compare() ) : m_compareStr( compareStr ) {}

		CSuffixTree( const TString& source, Compare compareStr = Compare() )
			: m_compareStr( compareStr )
			, m_source( source )
		{
			BuildSuffixTree();
		}

		TString FindLongestDuplicate( size_t dupTimes = 1 ) const
		{
			ASSERT( dupTimes != 0 );
			ENSURE( m_source.size() == m_suffixes.size() );

			if ( m_source.empty() || dupTimes >= m_suffixes.size() )
				return TString();

			size_t maxPos = 0, maxLen = 0;

			for ( size_t i = 0, count = m_suffixes.size() - dupTimes; i != count; ++i )
			{
				size_t commonLen = GetCommonLength( m_suffixes[ i ], m_suffixes[ i + dupTimes ] );
				if ( commonLen > maxLen )
				{
					maxLen = commonLen;
					maxPos = i;
				}
			}
			return TString( m_suffixes[ maxPos ], maxLen );
		}

		TString FindLongestCommonSubstring( const std::vector< std::basic_string<CharType> >& items )
		{
			ASSERT( m_source.empty() && m_suffixes.empty() );		// instantiated with default constructor
			switch ( items.size() )
			{
				case 0:	return TString();							// no common substring
				case 1:	return items.front();						// single item: the entire string is common
			}

			const CharType empty[] = { '\0' };
			m_source = str::Join( items, empty );					// concatenate all items (no separator) and find multiple occurences
			BuildSuffixTree();

			// find longest common prefix
			str::CPart<CharType> lcPrefix( empty );				// start empty

			if ( !m_suffixes.empty() )
				for ( size_t i = 0; i != m_suffixes.size() - 1; ++i )
				{
					const CharType* pPrefix = m_suffixes[ i ];
					const CharType* pNextPrefix = m_suffixes[ i + 1 ];

					if ( size_t commonLen = GetCommonLength( pPrefix, pNextPrefix ) )
						if ( commonLen > lcPrefix.m_count )
						{
							str::CPart<CharType> prefix( pPrefix, commonLen );
							if ( str::AllContain( items, prefix, m_compareStr ) )
								lcPrefix = prefix;
						}
				}

			return lcPrefix.ToString();
		}
	private:
		void BuildSuffixTree( void )
		{
			m_suffixes.reserve( m_source.length() );
			for ( const CharType* pSuffix = m_source.c_str(); *pSuffix != 0; ++pSuffix )
				m_suffixes.push_back( pSuffix );

			std::sort( m_suffixes.begin(), m_suffixes.end(), LessRefPtr( m_compareStr ) );		// sort the suffix tree
		}

		size_t GetCommonLength( const CharType* pLeft, const CharType* pRight ) const
		{
			return str::GetCommonLength( pLeft, pRight, m_compareStr );
		}

		struct LessRefPtr
		{
			LessRefPtr( Compare compareStr = Compare() ) : m_compareStr( compareStr ) {}

			bool operator()( const CharType*& rpLeft, const CharType*& rpRight ) const
			{
				return pred::Less == m_compareStr( rpLeft, rpRight );
			}
		private:
			Compare m_compareStr;
		};
	private:
		Compare m_compareStr;
		TString m_source;								// keep a copy of source
		std::vector<const CharType*> m_suffixes;		// suffix tree as sorted array: pointers to each character in the source string

		friend class CLcsTests;
	};
}


namespace str
{
	// single string multiple occurence

	template< typename CharType >
	std::basic_string<CharType> FindLongestDuplicatedString( const std::basic_string<CharType>& source, size_t dupTimes = 1 )
	{
		lcs::CSuffixTree<CharType> suffixTree( source );
		return suffixTree.FindLongestDuplicate( dupTimes );
	}

	template< typename CharType, typename Compare >
	std::basic_string<CharType> FindLongestDuplicatedString( const std::basic_string<CharType>& source, Compare compareStr, size_t dupTimes = 1 )
	{
		lcs::CSuffixTree<CharType, Compare> suffixTree( source, compareStr );
		return suffixTree.FindLongestDuplicate( dupTimes );
	}


	// string set common occurence

	template< typename CharType >
	std::basic_string<CharType> FindLongestCommonSubstring( const std::vector< std::basic_string<CharType> >& items )
	{
		lcs::CSuffixTree<CharType> suffixTree;
		return suffixTree.FindLongestCommonSubstring( items );
	}

	template< typename CharType, typename Compare >
	std::basic_string<CharType> FindLongestCommonSubstring( const std::vector< std::basic_string<CharType> >& items, Compare compareStr )
	{
		lcs::CSuffixTree<CharType, Compare> suffixTree( compareStr );
		return suffixTree.FindLongestCommonSubstring( items );
	}
}


#endif // LongestCommonDuplicate_h
