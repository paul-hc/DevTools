#ifndef LongestCommonSubsequence_h
#define LongestCommonSubsequence_h
#pragma once

#include "StringCompare.h"


namespace lcs
{
	template< typename T >
	inline bool IsEmpty( const T* pObject )
	{
		return NULL == pObject || T() == *pObject;
	}

	template<>
	inline bool IsEmpty< std::tstring >( const std::tstring* pString )
	{
		return NULL == pString || 0 == pString->length();
	}

	template< typename T >
	inline bool IsEqualTo( const T* pLeft, const T* pRight )
	{
		return
			pLeft == pRight ||
			( pLeft != NULL && pRight != NULL && *pLeft == *pRight );
	}


	/**
		basic data provider template class for integral types
		example:
			const TCHAR* str = "a string to compare";
			lcs::CDataBlock< TCHAR > compare_data1( str, strlen( str ) );
	*/
	template< typename T >
	class CDataBlock
	{
	public:
		CDataBlock( const T* pData, size_t size ) : m_pData( pData ), m_size( static_cast< UINT >( size ) ) { ASSERT_PTR( m_pData ); }

		const T* Get( void ) const { return m_pData; }
		UINT GetSize( void ) const { return m_size; }

		const T* GetAt( UINT index ) const
		{
			ASSERT( index <= m_size );		// might get past end one position -> return NULL
			return index < m_size ? &m_pData[ index ] : NULL;
		}

		bool operator==( const CDataBlock& rRight ) const
		{
			if ( m_pData == rRight.m_pData )
				return true;

			return
				m_size == rRight.m_size &&
				0 == memcmp( m_pData, rRight.m_pData, m_size * sizeof( T ) );
		}

		bool operator!=( const CDataBlock& rRight ) const { return !operator==( rRight ); }
	private:
		const T* m_pData;	// pointer to the data block
		UINT m_size;		// length of the data block
	};


	enum MatchType
	{
		Equal,				// same element in both
		EqualDiffCase,		// same element in both, different case
		Insert,				// element in the destination, but not the source
		Remove				// element in the source, but not the destination
	};


	template< typename T >
	struct CResult
	{
		CResult( void ) : m_index( UINT_MAX ), m_matchType( Equal ), m_srcValue(), m_destValue() {}
		CResult( UINT index, MatchType matchType, const T& value ) : m_index( index ), m_matchType( matchType ), m_srcValue( value ), m_destValue( value ) {}
		CResult( UINT index, MatchType matchType, const T& srcValue, const T& destValue ) : m_index( index ), m_matchType( matchType ), m_srcValue( srcValue ), m_destValue( destValue ) {}
	public:
		UINT m_index;
		MatchType m_matchType;
		T m_srcValue;
		T m_destValue;
	};


	// LCS: Longest Common Subsequence
	// it does the comparison between source and destination

	template< typename T, typename MatchFunc >
	class Comparator
	{
	public:
		Comparator( const T* pSrc, size_t srcSize, const T* pDest, size_t destSize ) : m_src( pSrc, srcSize ), m_dest( pDest, destSize ) {}

		template< typename StringType >
		explicit Comparator( const StringType& src, const StringType& dest ) : m_src( src.c_str(), src.length() ), m_dest( dest.c_str(), dest.length() ) {}

		void Process( std::vector< CResult< T > >& rOutSeq );
		size_t GetLcsLength( void ) const { return static_cast< size_t >( LcsAt( 0, 0 ) ); }
	private:
		short& LcsAt( UINT col, UINT row );
		const short& LcsAt( UINT col, UINT row ) const { return const_cast< Comparator* >( this )->LcsAt( col, row ); }

		void QueryResults( std::vector< CResult< T > >& rOutSeq ) const;

		template< typename T >
		str::Match GetValueMatch( const T* pLeft, const T* pRight ) const
		{
			if ( pLeft == pRight )
				return str::MatchEqual;							// works for NULL pointers

			if ( pLeft != NULL && pRight != NULL )
				return m_getMatchFunc( *pLeft, *pRight );		// first character matching

			return str::MatchNotEqual;
		}
	private:
		CDataBlock< T > m_src;
		CDataBlock< T > m_dest;
		MatchFunc m_getMatchFunc;
		std::vector< short > m_lcsArray;		// LCS working array
	};
}


namespace lcs
{
	// Comparator< T, MatchFunc > template code

	template< typename T, typename MatchFunc >
	inline short& Comparator< T, MatchFunc >::LcsAt( UINT col, UINT row )
	{
		UINT index = ( row * (UINT)m_src.GetSize() ) + col;
		ASSERT( index < m_lcsArray.size() );
		return m_lcsArray[ index ];
	}

	// we calculate the LCS array and return the LCS length
	template< typename T, typename MatchFunc >
	void Comparator< T, MatchFunc >::Process( std::vector< CResult< T > >& rOutSeq )
	{
		if ( m_src == m_dest )
		{
			rOutSeq.resize( m_dest.GetSize() );
			for ( UINT i = 0; i != rOutSeq.size(); ++i )
				rOutSeq[ i ] = CResult< T >( i, Equal, *m_dest.GetAt( i ) );

			return;
		}

		// calculate the size of the LCS working array
		UINT lcsSize = ( 1 + m_src.GetSize() ) * ( 1 + m_dest.GetSize() );

		m_lcsArray.resize( lcsSize, -1 );	// initialise to -1
		if ( lcsSize > 1 )
		{
			// work through the array, right to left, bottom to top
			for ( int col = (int)m_src.GetSize(); col >= 0; --col )
				for ( int row = (int)m_dest.GetSize(); row >= 0; --row )
				{	// get the data at the current col, row for each data source
					const T* pSrcData = m_src.GetAt( col );
					const T* pDestData = m_dest.GetAt( row );

					if ( lcs::IsEmpty( pSrcData ) || lcs::IsEmpty( pDestData ) )
					{
						// if either data is null, set the array entry to zero
						LcsAt( col, row ) = 0;
					}
					else if ( lcs::IsEqualTo( pSrcData, pDestData ) )
					{
						// if the data for each source is equal, then add one
						// to the value at the previous diagonal location - to
						// the right and below - and store it in the current location
						LcsAt( col, row ) = LcsAt( col + 1, row + 1 ) + 1;
					}
					else
					{
						// if the data is not null and not equal, then copy
						// the maximum value from the two cells to the right
						// and below, into the current location
						LcsAt( col, row ) = std::max( LcsAt( col + 1, row ), LcsAt( col, row + 1 ) );
					}
				}
		}

		QueryResults( rOutSeq );
	}


	template< typename T, typename MatchFunc >
	void Comparator< T, MatchFunc >::QueryResults( std::vector< CResult< T > >& rOutSeq ) const
	{
		for ( UINT col = 0, row = 0; col < m_src.GetSize() || row < m_dest.GetSize(); )
		{
			const T* pSrcData = m_src.GetAt( col );
			const T* pDestData = m_dest.GetAt( row );

			str::Match match = GetValueMatch( pSrcData, pDestData );

			if ( str::MatchEqual == match || str::MatchEqualDiffCase == match )
			{
				// if the data is equal, then mark the record to be kept
				rOutSeq.push_back( CResult< T >( ++col, str::MatchEqual == match ? lcs::Equal : lcs::EqualDiffCase, *pSrcData, *pDestData ) );
				++row;
			}
			else if ( !lcs::IsEmpty( pSrcData ) && ( lcs::IsEmpty( pDestData ) || LcsAt( col + 1, row ) >= LcsAt( col, row + 1 ) ) )
			{
				// if the value to the right is greater than or equal to the
				// value below, and the data is not null, then mark the first
				// data to be deleted
				rOutSeq.push_back( CResult< T >( ++col, lcs::Remove, *pSrcData ) );
			}
			else
			{
				// if the value below is greater than the value to the right,
				// then mark the first data to be deleted the data should not null
				ASSERT( !lcs::IsEmpty( pDestData ) );

				rOutSeq.push_back( CResult< T >( ++row, lcs::Insert, *pDestData ) );
			}
		}
	}

} // namespace lcs


#endif // LongestCommonSubsequence_h
