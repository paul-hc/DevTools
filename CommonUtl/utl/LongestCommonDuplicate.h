#ifndef LongestCommonDuplicate_h
#define LongestCommonDuplicate_h
#pragma once


namespace lcs
{
	// use suffix trees to find the Longest Common Duplicate (LCD)

	template< typename CharType, typename Compare = pred::CompareCase() >
	class CSuffixTree
	{
		typedef std::basic_string< CharType > StringType;
	public:
		CSuffixTree( const StringType& source, Compare compareStr )
			: m_compareStr( compareStr )
			, m_source( source )
		{
			BuildSortedSuffixTree();
		}

		StringType FindLongestDuplicate( size_t dupTimes = 1 ) const
		{
			ASSERT( dupTimes != 0 );
			ENSURE( m_source.size() == m_suffixes.size() );

			if ( m_source.empty() || dupTimes >= m_suffixes.size() )
				return StringType();

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
			return StringType( m_suffixes[ maxPos ], maxLen );
		}
	private:
		void BuildSortedSuffixTree( void )
		{
			m_suffixes.reserve( m_source.length() );
			for ( size_t pos = 0; pos != m_source.length(); ++pos )
				m_suffixes.push_back( &m_source[ pos ] );

			std::sort( m_suffixes.begin(), m_suffixes.end(), LessRefPtr( m_compareStr ) );		// sort the suffix tree
		}

		size_t GetCommonLength( const CharType* pLeft, const CharType* pRight ) const
		{
			size_t len = 0;
			while ( *pLeft != 0 && pred::Equal == m_compareStr( *pLeft++, *pRight++ ) )
				len++;

			return len;
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
		const StringType& m_source;
		std::vector< const CharType* > m_suffixes;			// suffix tree as sorted array: pointers to each character in the source string
	};
}


namespace str
{
	// single string multiple occurence

	template< typename CharType >
	std::basic_string< CharType > FindLongestDuplicatedString( const std::basic_string< CharType >& source, size_t dupTimes = 1 )
	{
		lcs::CSuffixTree< CharType > suffixTree( source, compareStr );
		return suffixTree.FindLongestDuplicate( dupTimes );
	}

	template< typename CharType, typename Compare >
	std::basic_string< CharType > FindLongestDuplicatedString( const std::basic_string< CharType >& source, Compare compareStr, size_t dupTimes = 1 )
	{
		lcs::CSuffixTree< CharType, Compare > suffixTree( source, compareStr );
		return suffixTree.FindLongestDuplicate( dupTimes );
	}


	// string set common occurence

	template< typename CharType >
	std::basic_string< CharType > FindLongestCommonSubstring( const std::vector< std::basic_string< CharType > >& srcItems )
	{
		switch ( srcItems.size() )
		{
			case 0:	return std::basic_string< CharType >();		// no common substring
			case 1:	return srcItems.front();					// single item: the entire string is common
		}

		const CharType sep[] = { 0 };
		return FindLongestDuplicatedString( str::Join( srcItems, sep ) );		// concatenate all items and find multiple occurences
	}

	template< typename CharType, typename Compare >
	std::basic_string< CharType > FindLongestCommonSubstring( const std::vector< std::basic_string< CharType > >& srcItems, Compare compareStr )
	{
		switch ( srcItems.size() )
		{
			case 0:	return std::basic_string< CharType >();		// no common substring
			case 1:	return srcItems.front();					// single item: the entire string is common
		}

		const CharType sep[] = { 0 };
		return FindLongestDuplicatedString( str::Join( srcItems, sep ), compareStr );
	}
}


#endif // LongestCommonDuplicate_h
