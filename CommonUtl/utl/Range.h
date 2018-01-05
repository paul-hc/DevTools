#ifndef Range_h
#define Range_h
#pragma once


template< typename ValueType >
struct Range
{
	typedef ValueType Value_Type;
public:
	Range( void ) {}
	Range( ValueType start, ValueType end ) : m_start( start ), m_end( end ) {}
	explicit Range( ValueType startAndEnd ) : m_start( startAndEnd ), m_end( startAndEnd ) {}		// empty range constructor

	template< typename U >
	Range( const Range< U >& right ) : m_start( static_cast< ValueType >( right.m_start ) ), m_end( static_cast< ValueType >( right.m_end ) ) {}

	void SetRange( ValueType start, ValueType end ) { m_start = start; m_end = end; }
	void SetEmptyRange( ValueType value ) { m_start = m_end = value; }

	void SwapBounds( void ) { std::swap( m_start, m_end ); }

	bool operator==( const Range& right ) const { return m_start == right.m_start && m_end == right.m_end; }
	bool operator!=( const Range& right ) const { return !operator==( right ); }

	bool IsEmpty( void ) const { return m_start == m_end; }
	bool IsNonEmpty( void ) const { return !IsEmpty() && IsNormalized(); /*m_start < m_end*/ }

	bool IsNormalized( void ) const { return m_start <= m_end; }

	Range< ValueType > GetNormalized( void ) const
	{
		if ( !IsNormalized() )
			return Range< ValueType >( m_end, m_start );
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

	bool ShiftInBounds( const Range< ValueType >& bounds )
	{
		REQUIRE( bounds.IsNormalized() );
		REQUIRE( Intersects( bounds ) );

		const Range< ValueType > orgRange = *this;

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

	bool InBucket( ValueType value ) const		// value in [m_start, m_end) - start inclusive, end exclusive
	{
		ASSERT( IsNormalized() );
		return value >= m_start && ( value < m_end || m_start == m_end );	// works for empty buckets
	}

	bool Constrain( ValueType& rValue ) const
	{
		REQUIRE( IsNormalized() );

		const ValueType oldValue = rValue;

		if ( rValue < m_start )
			rValue = m_start;
		else if ( rValue > m_end )
			rValue = m_end;

		return rValue != oldValue;
	}


	// range relationships

	bool Contains( ValueType value, bool canTouchBounds = true ) const
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
		ValueType intersectStart = (std::max)( m_start, right.m_start );
		ValueType intersectEnd = (std::min)( m_end, right.m_end );

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
	ValueType m_start;
	ValueType m_end;
};


namespace utl
{
	template< typename ValueType >
	inline Range< ValueType > MakeRange( ValueType start, ValueType end ) { return Range< ValueType >( start, end ); }

	template< typename ValueType >
	inline Range< ValueType > MakeEmptyRange( ValueType startAndEnd ) { return Range< ValueType >( startAndEnd, startAndEnd ); }
}


namespace pred
{
	struct LessRangeStart
	{
		template< typename ValueType >
		bool operator()( const Range< ValueType >& left, const Range< ValueType >& right ) const
		{
			return ( left.m_start < right.m_start ) != 0; // keep it friendly with COleDateTime
		}
	};

	struct LessRangeEnd
	{
		template< typename ValueType >
		bool operator()( const Range< ValueType >& left, const Range< ValueType >& right ) const
		{
			return ( left.m_end < right.m_end ) != 0; // keep it friendly with COleDateTime
		}
	};
}


// template implementation

template< typename ValueType >
bool Range< ValueType >::Truncate( const Range& limit )
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

template< typename ValueType >
std::ostream& operator<<( std::ostream& os, const Range< ValueType >& range )
{
	return os << "[" << range.m_start << "," << range.m_end << "]";
}

template< typename ValueType >
std::wostream& operator<<( std::wostream& os, const Range< ValueType >& range )
{
	return os << L"[" << range.m_start << L"," << range.m_end << L"]";
}


#endif // Range_h
