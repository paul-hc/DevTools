#ifndef RandomUtilities_h
#define RandomUtilities_h
#pragma once

#include "Range.h"
#include "StringBase.h"
#include "StdHashValue.h"


namespace utl
{
	inline void SetRandomSeed( size_t seed = ::GetTickCount() )
	{
		::srand( utl::GetHashValue( seed ) % RAND_MAX );
	}

	template< typename ValueType >
	ValueType GetRandomValue( ValueType minValue, ValueType maxValue )
	{
		ASSERT( minValue <= maxValue );
		return minValue + static_cast<ValueType>( (double)rand() * ( 1.0 / ( RAND_MAX + 1.0 ) ) * ( maxValue - minValue ) );
	}

	template< typename ValueType >
	inline ValueType GetRandomValue( ValueType maxValue )
	{
		return GetRandomValue( ValueType(), maxValue );
	}
}


namespace func
{
	template< typename CharType >
	struct RandomChar
	{
		RandomChar( const Range<CharType>& charRange ) : m_charRange( charRange ) { ASSERT( m_charRange.IsNormalized() ); }
		RandomChar( CharType minChar, CharType maxChar ) : m_charRange( minChar, maxChar ) { ASSERT( m_charRange.IsNormalized() ); }

		CharType operator()( void ) const
		{
			return utl::GetRandomValue<CharType>( m_charRange.m_start, m_charRange.m_end + 1 );		// add 1 to include maxChar
		}
	private:
		Range<CharType> m_charRange;
	};
}


namespace utl
{
	template< typename CharType >
	inline Range<CharType> GetRangeLowerLetters( void ) { return Range<CharType>( 'a', 'z' ); }

	template< typename CharType >
	std::basic_string<CharType> MakeRandomString( size_t length, const Range<CharType>& charRange = GetRangeLowerLetters() )
	{
		#pragma warning( disable: 4996 )	// warning C4996: 'std::generate_n': Function call with parameters that may be unsafe

		std::vector<CharType> buffer( length + 1, '\0' );

		std::generate_n( &buffer.front(), length, func::RandomChar<CharType>( charRange ) );
		return &buffer.front();
	}

	template< typename CharType >
	void GenerateRandomStrings( std::vector< std::basic_string<CharType> >& rItems, size_t count, size_t maxLen, const Range<CharType>& charRange = GetRangeLowerLetters() )
	{
		rItems.resize( count );

		for ( typename std::vector< std::basic_string<CharType> >::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			*itItem = utl::MakeRandomString( utl::GetRandomValue<size_t>( maxLen ), charRange );		// random string of random length
	}

	template< typename CharType >
	void InsertFragmentRandomly( std::vector< std::basic_string<CharType> >& rItems, const CharType fragment[] )
	{
		for ( typename std::vector< std::basic_string<CharType> >::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			itItem->insert( utl::GetRandomValue<size_t>( itItem->length() ), fragment );
	}
}


#endif // RandomUtilities_h
