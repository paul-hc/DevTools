#ifndef StringRange_h
#define StringRange_h
#pragma once

#include "Range.h"
#include "StringCompare.h"


namespace str
{
	template< typename CharType >
	bool ContainsAnyOf( const CharType* pCharSet, CharType ch )
	{
		ASSERT_PTR( pCharSet );
		for ( ; *pCharSet != 0; ++pCharSet )
			if ( ch == *pCharSet )
				return true;

		return false;
	}


	namespace range
	{
		template< typename CharType >
		inline bool InBounds( const Range< size_t >& pos, const std::basic_string< CharType >& text )
		{
			return pos.IsNormalized() && pos.m_start <= text.length() && pos.m_end <= text.length();
		}

		template< typename CharType >
		inline Range< size_t > MakeBounds( const std::basic_string< CharType >& text )
		{
			return Range< size_t >( 0, text.length() );
		}

		template< typename CharType >
		inline Range< size_t > MakeInitRange( const Range< size_t >& inputPos, const std::basic_string< CharType >& text )
		{
			return InBounds( inputPos, text ) ? inputPos : MakeBounds( text );
		}

		template< typename CharType >
		inline std::basic_string< CharType > Extract( const Range< size_t >& pos, const std::basic_string< CharType >& text )
		{
			ASSERT( range::InBounds( pos, text ) );
			return text.substr( pos.m_start, pos.GetSpan< size_t >() );
		}


		template< typename CharType >
		class CStringRange
		{
			typedef typename std::basic_string< CharType > StringType;
		public:
			explicit CStringRange( const StringType& text ) : m_text( text ), m_pos( MakeBounds( m_text ) ) {}
			CStringRange( const StringType& text, const Range< size_t >& pos ) : m_text( text ), m_pos( pos ) { ENSURE( InBounds() ); }

			bool InBounds( void ) const { return range::InBounds( m_pos, m_text ); }
			bool IsEmpty( void ) const { return 0 == GetLength(); }
			size_t GetLength( void ) const { ASSERT( InBounds() ); return m_pos.GetSpan< size_t >(); }

			const StringType& GetText( void ) const { return m_text; }

			const Range< size_t >& GetPos( void ) const { return m_pos; }
			Range< size_t >& RefPos( void ) { return m_pos; }
			void SetPos( const Range< size_t >& pos ) { m_pos = pos; ENSURE( InBounds() ); }

			void Reset( void ) { m_pos = MakeBounds( m_text ); }

			StringType Extract( void ) const { return Extract( m_pos ); }
			StringType Extract( const Range< size_t >& pos ) const { return range::Extract( pos, m_text ); }
			StringType ExtractLead( size_t endPos ) const { return range::Extract( utl::MakeRange( m_pos.m_start, endPos ), m_text ); }
			StringType ExtractTrail( size_t startPos ) const { return range::Extract( utl::MakeRange( startPos, m_pos.m_end ), m_text ); }

			CStringRange MakeLead( size_t endPos ) const { return CStringRange( m_text, utl::MakeRange( m_pos.m_start, endPos ) ); }
			CStringRange MakeTrail( size_t startPos ) const { return CStringRange( m_text, utl::MakeRange( startPos, m_pos.m_end ) ); }

			void SplitPair( StringType& rLeading, StringType& rTrailing, const Range< size_t >& sepPos ) const
			{
				rLeading = ExtractLead( sepPos.m_start );
				rTrailing = ExtractTrail( sepPos.m_end );
			}

			std::pair< CStringRange, CStringRange > GetSplitPair( const Range< size_t >& sepPos ) const
			{
				return std::pair< CStringRange, CStringRange >(
					CStringRange( m_text, utl::MakeRange( m_pos.m_start, sepPos.m_start ) ),
					CStringRange( m_text, utl::MakeRange( sepPos.m_end, m_pos.m_end ) ) );
			}

			bool HasPrefix( const CharType prefix[] ) const { return _HasPrefix( MakePart( prefix ) ); }
			bool HasPrefix( CharType prefixCh ) const { return _HasPrefix( CPart< CharType >( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool HasSuffix( const CharType suffix[] ) const { return _HasSuffix( MakePart( suffix ) ); }
			bool HasSuffix( CharType suffixCh ) const { return _HasSuffix( CPart< CharType >( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool StripPrefix( const CharType prefix[] ) { return _StripPrefix( MakePart( prefix ) ); }
			bool StripPrefix( CharType prefixCh ) { return _StripPrefix( CPart< CharType >( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool StripSuffix( const CharType suffix[] ) { return _StripSuffix( MakePart( suffix ) ); }
			bool StripSuffix( CharType suffixCh ) { return _StripSuffix( CPart< CharType >( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Strip( const CharType prefix[], const CharType suffix[] ) { return _Strip( MakePart( prefix ), MakePart( suffix ) ); }
			bool Strip( CharType prefixCh, CharType suffixCh ) { return _Strip( CPart< CharType >( &prefixCh, prefixCh != 0 ? 1 : 0 ), CPart< CharType >( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Find( Range< size_t >& rFoundPos, const CharType part[], str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find< str::Case >( rFoundPos, MakePart( part ) )
					: _Find< str::IgnoreCase >( rFoundPos, MakePart( part ) );
			}

			bool Find( Range< size_t >& rFoundPos, CharType ch, str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find< str::Case >( rFoundPos, CPart< CharType >( &ch, ch != 0 ? 1 : 0 ) )
					: _Find< str::IgnoreCase >( rFoundPos, CPart< CharType >( &ch, ch != 0 ? 1 : 0 ) );
			}

			bool Equals( const CharType part[], str::CaseType caseType = str::Case )
			{
				ASSERT( InBounds() );
				return str::EqualsN( m_text.c_str() + m_pos.m_start, part, GetLength(), str::Case == caseType );
			}

			bool TrimLeft( const CharType* pWhiteSpace = StdWhitespace< CharType >() )
			{
				ASSERT( InBounds() );
				// trim re-entrantly
				size_t oldStart = m_pos.m_start;

				while ( m_pos.m_start < m_pos.m_end && ContainsAnyOf( pWhiteSpace, m_text[ m_pos.m_start ] ) )
					++m_pos.m_start;

				ENSURE( m_pos.IsNormalized() );
				return m_pos.m_start != oldStart;		// any change?
			}

			bool TrimRight( const CharType* pWhiteSpace = StdWhitespace< CharType >() )
			{
				ASSERT( InBounds() );
				// trim re-entrantly
				size_t oldEnd = m_pos.m_end;

				while ( m_pos.m_start < m_pos.m_end && ContainsAnyOf( pWhiteSpace, m_text[ m_pos.m_end - 1 ] ) )
					--m_pos.m_end;

				ENSURE( m_pos.IsNormalized() );
				return m_pos.m_end != oldEnd;		// any change?
			}

			bool Trim( const CharType* pWhiteSpace = StdWhitespace< CharType >() )
			{
				Range< size_t > oldPos = m_pos;
				TrimLeft( pWhiteSpace );
				TrimRight( pWhiteSpace );
				return m_pos != oldPos;
			}
		private:
			bool _HasPrefix( const CPart< CharType >& prefix ) const
			{
				ASSERT( InBounds() );
				return
					0 == prefix.m_count ||			// empty is always a match
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_start ], prefix.m_pString, prefix.m_count );
			}

			bool _HasSuffix( const CPart< CharType >& suffix ) const
			{
				ASSERT( InBounds() );
				if ( 0 == suffix.m_count )
					return true;					// empty is always a match

				return
					m_pos.m_end >= suffix.m_count &&
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_end - suffix.m_count ], suffix.m_pString, suffix.m_count );
			}

			bool _StripPrefix( const CPart< CharType >& prefix )
			{
				if ( !_HasPrefix( prefix ) )
					return false;

				m_pos.m_start += prefix.m_count;
				return true;		// changed
			}

			bool _StripSuffix( const CPart< CharType >& suffix )
			{
				if ( !_HasSuffix( suffix ) )
					return false;

				m_pos.m_end -= suffix.m_count;
				return true;		// changed
			}

			bool _Strip( const CPart< CharType >& prefix, const CPart< CharType >& suffix )
			{
				bool hasPrefix = _StripPrefix( prefix ), hasSuffix = _StripSuffix( suffix );
				return hasPrefix && hasSuffix;
			}

			template< str::CaseType caseType >
			bool _Find( Range< size_t >& rFoundPos, const CPart< CharType >& part ) const
			{
				ASSERT( InBounds() );

				typedef typename const CharType* iterator;

				iterator itBegin = m_text.c_str(), itEnd = itBegin + m_text.length();
				iterator itFound = std::search( itBegin + m_pos.m_start, itEnd, part.m_pString, part.m_pString + part.m_count, pred::CharEqual< caseType >() );
				if ( itFound == itEnd )
					return false;

				rFoundPos.m_start = std::distance( itBegin, itFound );
				rFoundPos.m_end = rFoundPos.m_start + part.m_count;
				return true;
			}
		private:
			const StringType& m_text;
			Range< size_t > m_pos;
		};
	}


	typedef str::range::CStringRange< TCHAR > TStringRange;
}


#endif // StringRange_h
