#ifndef CommonLength_h
#define CommonLength_h
#pragma once

#include "Range.h"


namespace utl
{
	// common length between 2 containers (usually strings):

	template< typename IterT, typename BinPredT >
	size_t FindCommonLength( IterT itStartLeft, IterT itEndLeft, IterT itStartRight, IterT itEndRight, BinPredT eqPred )
	{
		size_t commonLen = 0;

		for ( ; itStartLeft != itEndLeft && itStartRight != itEndRight; ++itStartLeft, ++itStartRight, ++commonLen )
			if ( !eqPred( *itStartLeft, *itStartRight ) )
				break;

		return commonLen;
	}


	template< typename ContainerT >
	inline size_t FindLeadCommonLength( const ContainerT& leftItems, const ContainerT& rightItems )
	{
		return FindCommonLength( leftItems.begin(), leftItems.end(), rightItems.begin(), rightItems.end(), pred::Equals() );
	}

	template< typename ContainerT, typename BinPredT >
	inline size_t FindLeadCommonLength( const ContainerT& leftItems, const ContainerT& rightItems, BinPredT eqPred )
	{
		return FindCommonLength( leftItems.begin(), leftItems.end(), rightItems.begin(), rightItems.end(), eqPred );
	}


	template< typename ContainerT >
	inline size_t FindTrailCommonLength( const ContainerT& leftItems, const ContainerT& rightItems )
	{
		return FindCommonLength( leftItems.rbegin(), leftItems.rend(), rightItems.rbegin(), rightItems.rend(), pred::Equals() );
	}

	template< typename ContainerT, typename BinPredT >
	inline size_t FindTrailCommonLength( const ContainerT& leftItems, const ContainerT& rightItems, BinPredT eqPred )
	{
		return FindCommonLength( leftItems.rbegin(), leftItems.rend(), rightItems.rbegin(), rightItems.rend(), eqPred );
	}


	// manage common length between 2 containers (usually strings) at both ends, with some extra utils
	//
	template< typename ContainerT, typename BinPredT = pred::Equals >
	struct CCommonLengths
	{
		CCommonLengths( const ContainerT& leftItems, const ContainerT& rightItems )
			: m_leftItems( leftItems )
			, m_rightItems( rightItems )
			, m_leadLen( FindLeadCommonLength( m_leftItems, m_rightItems, m_eqPred ) )
			, m_trailLen( EvalTrailCommonLen() )
		{
		}

		bool HasFullMatch( void ) const { return m_leftItems.size() == m_rightItems.size() && m_leadLen == m_leftItems.size() && 0 == m_trailLen; }
		bool HasPartialMatch( void ) const { return !HasFullMatch() && ( m_leadLen != 0 || m_trailLen != 0 ); }
		bool HasNoMatch( void ) const { return 0 == m_leadLen && 0 == m_trailLen; }

		template< typename ValueT >
		Range<ValueT> GetLeftMismatchRange( void ) const { return Range<ValueT>( static_cast<ValueT>( m_leadLen ), static_cast<ValueT>( m_leftItems.size() - m_trailLen ) ); }

		template< typename ValueT >
		Range<ValueT> GetRightMismatchRange( void ) const { return Range<ValueT>( static_cast<ValueT>( m_leadLen ), static_cast<ValueT>( m_rightItems.size() - m_trailLen ) ); }

		ContainerT MakeLeftMismatch( void ) const { return ContainerT( m_leftItems.begin() + m_leadLen, m_leftItems.end() - m_trailLen ); }
		ContainerT MakeRightMismatch( void ) const { return ContainerT( m_rightItems.begin() + m_leadLen, m_rightItems.end() - m_trailLen ); }
	private:
		size_t EvalTrailCommonLen( void ) const
		{
			size_t trailCommonLen = FindTrailCommonLength( m_leftItems, m_rightItems, m_eqPred );
			size_t minLen = std::min( m_leftItems.size(), m_rightItems.size() );

			// clamp the trailCommonLen to what's already consumed by m_leadLen
			return std::min( trailCommonLen, minLen - m_leadLen );
		}
	private:
		BinPredT m_eqPred;
		const ContainerT& m_leftItems;
		const ContainerT& m_rightItems;
	public:
		const size_t m_leadLen;
		const size_t m_trailLen;
	};
}


#endif // CommonLength_h
