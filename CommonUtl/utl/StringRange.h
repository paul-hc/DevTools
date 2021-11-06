#ifndef StringRange_h
#define StringRange_h
#pragma once

#include "Range.h"
#include "StringCompare.h"


namespace str
{
	namespace range
	{
		// range based algorithms based on string position pair: for fast and efficient parsing of text content

		template< typename CharT >
		inline bool InBounds( const Range<size_t>& pos, const std::basic_string<CharT>& text )
		{
			return pos.IsNormalized() && pos.m_start <= text.length() && pos.m_end <= text.length();
		}

		template< typename CharT >
		inline Range<size_t> MakeBounds( const std::basic_string<CharT>& text )
		{
			return Range<size_t>( 0, text.length() );
		}

		template< typename CharT >
		inline std::basic_string<CharT> Extract( const Range<size_t>& pos, const std::basic_string<CharT>& text )
		{
			ASSERT( range::InBounds( pos, text ) );
			return text.substr( pos.m_start, pos.GetSpan< size_t >() );
		}


		template< typename CharT >
		class CStringRange
		{
			typedef typename std::basic_string<CharT> StringT;
		public:
			explicit CStringRange( const StringT& text ) : m_text( text ), m_pos( MakeBounds( m_text ) ) {}
			CStringRange( const StringT& text, const Range<size_t>& pos ) : m_text( text ), m_pos( pos ) { ENSURE( InBounds() ); }

			bool InBounds( void ) const { return range::InBounds( m_pos, m_text ); }
			bool IsEmpty( void ) const { return 0 == GetLength(); }
			size_t GetLength( void ) const { ASSERT( InBounds() ); return m_pos.GetSpan< size_t >(); }

			const Range<size_t>& GetPos( void ) const { return m_pos; }
			Range<size_t>& RefPos( void ) { return m_pos; }
			void SetPos( const Range<size_t>& pos ) { m_pos = pos; ENSURE( InBounds() ); }

			const StringT& GetText( void ) const { return m_text; }
			const CharT* GetStartPtr( void ) const { REQUIRE( InBounds() ); return m_text.c_str() + m_pos.m_start; }
			const CharT* GetEndPtr( void ) const { REQUIRE( InBounds() ); return m_text.c_str() + m_pos.m_end; }
			CharT GetStartCh( void ) const { return *GetStartPtr(); }

			void Reset( void ) { m_pos = MakeBounds( m_text ); }

			StringT Extract( void ) const { return Extract( m_pos ); }
			StringT Extract( const Range<size_t>& pos ) const { return range::Extract( pos, m_text ); }
			StringT ExtractPrefix( void ) const { return range::Extract( utl::MakeRange( static_cast<size_t>( 0 ), m_pos.m_start ), m_text ); }
			StringT ExtractSuffix( void ) const { return range::Extract( utl::MakeRange( m_pos.m_end, m_text.length() ), m_text ); }
			StringT ExtractLead( size_t endPos ) const { return range::Extract( utl::MakeRange( m_pos.m_start, endPos ), m_text ); }
			StringT ExtractTrail( size_t startPos ) const { return range::Extract( utl::MakeRange( startPos, m_pos.m_end ), m_text ); }

			CStringRange MakeLead( size_t endPos ) const { return CStringRange( m_text, utl::MakeRange( m_pos.m_start, endPos ) ); }
			CStringRange MakeTrail( size_t startPos ) const { return CStringRange( m_text, utl::MakeRange( startPos, m_pos.m_end ) ); }

			void SplitPair( StringT& rLeading, StringT& rTrailing, const Range<size_t>& sepPos ) const
			{
				rLeading = ExtractLead( sepPos.m_start );
				rTrailing = ExtractTrail( sepPos.m_end );
			}

			std::pair< CStringRange, CStringRange > GetSplitPair( const Range<size_t>& sepPos ) const
			{
				return std::pair< CStringRange, CStringRange >(
					CStringRange( m_text, utl::MakeRange( m_pos.m_start, sepPos.m_start ) ),
					CStringRange( m_text, utl::MakeRange( sepPos.m_end, m_pos.m_end ) ) );
			}

			bool HasPrefix( const CharT prefix[] ) const { return _HasPrefix( MakePart( prefix ) ); }
			bool HasPrefix( CharT prefixCh ) const { return _HasPrefix( CPart<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool HasSuffix( const CharT suffix[] ) const { return _HasSuffix( MakePart( suffix ) ); }
			bool HasSuffix( CharT suffixCh ) const { return _HasSuffix( CPart<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool StripPrefix( const CharT prefix[] ) { return _StripPrefix( MakePart( prefix ) ); }
			bool StripPrefix( CharT prefixCh ) { return _StripPrefix( CPart<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool StripSuffix( const CharT suffix[] ) { return _StripSuffix( MakePart( suffix ) ); }
			bool StripSuffix( CharT suffixCh ) { return _StripSuffix( CPart<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Strip( const CharT prefix[], const CharT suffix[] ) { return _Strip( MakePart( prefix ), MakePart( suffix ) ); }
			bool Strip( CharT prefixCh, CharT suffixCh ) { return _Strip( CPart<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ), CPart<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Find( Range<size_t>& rFoundPos, const CharT part[], str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find< str::Case >( rFoundPos, MakePart( part ) )
					: _Find< str::IgnoreCase >( rFoundPos, MakePart( part ) );
			}

			bool Find( Range<size_t>& rFoundPos, CharT ch, str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find< str::Case >( rFoundPos, CPart<CharT>( &ch, ch != 0 ? 1 : 0 ) )
					: _Find< str::IgnoreCase >( rFoundPos, CPart<CharT>( &ch, ch != 0 ? 1 : 0 ) );
			}

			bool Equals( const CharT part[], str::CaseType caseType = str::Case )
			{
				ASSERT( InBounds() );
				return str::EqualsN_ByCase( caseType, m_text.c_str() + m_pos.m_start, part, GetLength() );
			}

			bool TrimLeft( const CharT* pWhiteSpace = StdWhitespace<CharT>() )
			{
				ASSERT( InBounds() );
				// trim re-entrantly
				size_t oldStart = m_pos.m_start;

				while ( m_pos.m_start < m_pos.m_end && ContainsAnyOf( pWhiteSpace, m_text[ m_pos.m_start ] ) )
					++m_pos.m_start;

				ENSURE( m_pos.IsNormalized() );
				return m_pos.m_start != oldStart;		// any change?
			}

			bool TrimRight( const CharT* pWhiteSpace = StdWhitespace<CharT>() )
			{
				ASSERT( InBounds() );
				// trim re-entrantly
				size_t oldEnd = m_pos.m_end;

				while ( m_pos.m_start < m_pos.m_end && ContainsAnyOf( pWhiteSpace, m_text[ m_pos.m_end - 1 ] ) )
					--m_pos.m_end;

				ENSURE( m_pos.IsNormalized() );
				return m_pos.m_end != oldEnd;		// any change?
			}

			bool Trim( const CharT* pWhiteSpace = StdWhitespace<CharT>() )
			{
				Range<size_t> oldPos = m_pos;
				TrimLeft( pWhiteSpace );
				TrimRight( pWhiteSpace );
				return m_pos != oldPos;
			}
		private:
			bool _HasPrefix( const CPart<CharT>& prefix ) const
			{
				ASSERT( InBounds() );
				return
					0 == prefix.m_count ||			// empty is always a match
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_start ], prefix.m_pString, prefix.m_count );
			}

			bool _HasSuffix( const CPart<CharT>& suffix ) const
			{
				ASSERT( InBounds() );
				if ( 0 == suffix.m_count )
					return true;					// empty is always a match

				return
					m_pos.m_end >= suffix.m_count &&
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_end - suffix.m_count ], suffix.m_pString, suffix.m_count );
			}

			bool _StripPrefix( const CPart<CharT>& prefix )
			{
				if ( !_HasPrefix( prefix ) )
					return false;

				m_pos.m_start += prefix.m_count;
				return true;		// changed
			}

			bool _StripSuffix( const CPart<CharT>& suffix )
			{
				if ( !_HasSuffix( suffix ) )
					return false;

				m_pos.m_end -= suffix.m_count;
				return true;		// changed
			}

			bool _Strip( const CPart<CharT>& prefix, const CPart<CharT>& suffix )
			{
				bool hasPrefix = _StripPrefix( prefix ), hasSuffix = _StripSuffix( suffix );
				return hasPrefix && hasSuffix;
			}

			template< str::CaseType caseType >
			bool _Find( Range<size_t>& rFoundPos, const CPart<CharT>& part ) const
			{
				ASSERT( InBounds() );

				typedef typename const CharT* iterator;

				iterator itBegin = m_text.c_str() + m_pos.m_start, itEnd = m_text.c_str() + m_pos.m_end;
				iterator itFound = std::search( itBegin, itEnd, part.m_pString, part.m_pString + part.m_count, pred::CharEqual< caseType >() );
				if ( itFound == itEnd )
					return false;

				rFoundPos.m_start = std::distance( m_text.c_str(), itFound );
				rFoundPos.m_end = rFoundPos.m_start + part.m_count;
				return true;
			}
		private:
			const StringT& m_text;
			Range<size_t> m_pos;
		};
	}


	typedef str::range::CStringRange<TCHAR> TStringRange;
}


#endif // StringRange_h
