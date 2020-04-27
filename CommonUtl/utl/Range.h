#ifndef Range_h
#define Range_h
#pragma once


template< typename ValueT >
struct Range
{
	typedef ValueT Value_Type;
public:
	Range( void ) {}
	Range( ValueT start, ValueT end ) : m_start( start ), m_end( end ) {}
	explicit Range( ValueT startAndEnd ) : m_start( startAndEnd ), m_end( startAndEnd ) {}		// empty range constructor

	template< typename U >
	Range( const Range< U >& right ) : m_start( static_cast< ValueT >( right.m_start ) ), m_end( static_cast< ValueT >( right.m_end ) ) {}

	void SetRange( ValueT start, ValueT end ) { m_start = start; m_end = end; }
	void SetEmptyRange( ValueT value ) { m_start = m_end = value; }

	void SwapBounds( void ) { std::swap( m_start, m_end ); }

	bool operator==( const Range& right ) const { return m_start == right.m_start && m_end == right.m_end; }
	bool operator!=( const Range& right ) const { return !operator==( right ); }

	bool IsEmpty( void ) const { return m_start == m_end; }
	bool IsNonEmpty( void ) const { return !IsEmpty() && IsNormalized(); /*m_start < m_end*/ }

	bool IsNormalized( void ) const { return m_start <= m_end; }

	Range< ValueT > GetNormalized( void ) const
	{
		if ( !IsNormalized() )
			return Range< ValueT >( m_end, m_start );
		return *this;
	}

	bool Normalize( void )
	{
		if ( m_start <= m_end )
			return false;
		SwapBounds();
		return true;
	}

	void Extend( const Range& right )
	{
		m_start = (std::min)( m_start, right.m_start );
		m_end = (std::max)( m_end, right.m_end );
	}

	bool Truncate( const Range& limit );

	bool SafeTruncate( const Range& limit )
	{
		if ( !Intersects( limit ) )
			return false;
		Truncate( limit );
		return true;
	}

	bool ShiftInBounds( const Range< ValueT >& bounds )
	{
		REQUIRE( bounds.IsNormalized() );
		REQUIRE( Intersects( bounds ) );

		const Range< ValueT > orgRange = *this;

		if ( m_start < bounds.m_start )
		{
			m_start += bounds.m_start - m_start;
			m_end += bounds.m_start - m_start;
		}
		else if ( m_end > bounds.m_end )
		{
			m_start -= m_end - bounds.m_end;
			m_end -= m_end - bounds.m_end;
		}
		Truncate( bounds );
		return *this != orgRange;
	}


	// value relationships

	bool InBucket( ValueT value ) const		// value in [m_start, m_end) - start inclusive, end exclusive
	{
		ASSERT( IsNormalized() );
		return value >= m_start && ( value < m_end || m_start == m_end );	// works for empty buckets
	}

	bool Constrain( ValueT& rValue ) const
	{
		REQUIRE( IsNormalized() );

		const ValueT oldValue = rValue;

		if ( rValue < m_start )
			rValue = m_start;
		else if ( rValue > m_end )
			rValue = m_end;

		return rValue != oldValue;
	}


	// range relationships

	bool Contains( ValueT value, bool canTouchBounds = true ) const
	{
		ASSERT( IsNormalized() );
		if ( canTouchBounds )
			return value >= m_start && value <= m_end;
		else
			return value > m_start && value < m_end;
	}

	bool Includes( const Range& right, bool canTouchBounds = true ) const
	{
		ASSERT( IsNormalized() && right.IsNormalized() );
		if ( canTouchBounds )
			return right.m_start >= m_start && right.m_end <= m_end;
		else
			return right.m_start > m_start && right.m_end < m_end;
	}

	bool Intersects( const Range& right, bool onlyNonEmptyIntersection = true ) const
	{
		ASSERT( IsNormalized() && right.IsNormalized() );
		ValueT intersectStart = (std::max)( m_start, right.m_start );
		ValueT intersectEnd = (std::min)( m_end, right.m_end );

		if ( onlyNonEmptyIntersection )
			return intersectStart < intersectEnd;
		else
			return intersectStart <= intersectEnd;
	}

	template< typename SpanType >
	SpanType GetSpan( void ) const { return m_end - m_start; }

	template< typename SpanType >
	void GetSpan( SpanType* pSpan ) const { ASSERT_PTR( pSpan ); *pSpan = m_end - m_start; }
public:
	ValueT m_start;
	ValueT m_end;
};


namespace utl
{
	template< typename ValueT >
	inline Range< ValueT > MakeRange( ValueT start, ValueT end ) { return Range< ValueT >( start, end ); }

	template< typename ValueT >
	inline Range< ValueT > MakeEmptyRange( ValueT startAndEnd ) { return Range< ValueT >( startAndEnd, startAndEnd ); }
}


namespace pred
{
	struct LessRangeStart
	{
		template< typename ValueT >
		bool operator()( const Range< ValueT >& left, const Range< ValueT >& right ) const
		{
			return ( left.m_start < right.m_start ) != 0; // keep it friendly with COleDateTime
		}
	};

	struct LessRangeEnd
	{
		template< typename ValueT >
		bool operator()( const Range< ValueT >& left, const Range< ValueT >& right ) const
		{
			return ( left.m_end < right.m_end ) != 0; // keep it friendly with COleDateTime
		}
	};
}


// template implementation

template< typename ValueT >
bool Range< ValueT >::Truncate( const Range& limit )
{
	// Please don't modify this implementation!
	//
	// We assume that the two intervals have a valid intersection, otherwise this method has no meaning
	// and no valid result for this range.
	ASSERT( Intersects( limit, false ) );

	if ( limit.Includes( *this ) )
		return false; // this range is within the bounds of limit -> no truncation required

	if ( m_start < limit.m_start )
		m_start = limit.m_start;

	if ( m_end > limit.m_end )
		m_end = limit.m_end;

	ASSERT( IsNormalized() );
	return true;				// truncated
}


#include <iosfwd>

template< typename ValueT >
std::ostream& operator<<( std::ostream& os, const Range< ValueT >& range )
{
	return os << "[" << range.m_start << "," << range.m_end << "]";
}

template< typename ValueT >
std::wostream& operator<<( std::wostream& os, const Range< ValueT >& range )
{
	return os << L"[" << range.m_start << L"," << range.m_end << L"]";
}


#endif // Range_h
