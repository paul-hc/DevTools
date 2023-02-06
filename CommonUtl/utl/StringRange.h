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
			return text.substr( pos.m_start, pos.GetSpan<size_t>() );
		}


		template< typename CharT >
		class CStringRange
		{
			typedef typename std::basic_string<CharT> TString;
		public:
			explicit CStringRange( const TString& text ) : m_text( text ), m_pos( MakeBounds( m_text ) ) {}
			CStringRange( const TString& text, const Range<size_t>& pos ) : m_text( text ), m_pos( pos ) { ENSURE( InBounds() ); }

			bool InBounds( void ) const { return range::InBounds( m_pos, m_text ); }
			bool IsEmpty( void ) const { return 0 == GetLength(); }
			size_t GetLength( void ) const { ASSERT( InBounds() ); return m_pos.GetSpan<size_t>(); }

			const Range<size_t>& GetPos( void ) const { return m_pos; }
			Range<size_t>& RefPos( void ) { return m_pos; }
			void SetPos( const Range<size_t>& pos ) { m_pos = pos; ENSURE( InBounds() ); }

			const TString& GetText( void ) const { return m_text; }
			const CharT* GetStartPtr( void ) const { REQUIRE( InBounds() ); return m_text.c_str() + m_pos.m_start; }
			const CharT* GetEndPtr( void ) const { REQUIRE( InBounds() ); return m_text.c_str() + m_pos.m_end; }
			CharT GetStartCh( void ) const { return *GetStartPtr(); }

			void Reset( void ) { m_pos = MakeBounds( m_text ); }

			TString Extract( void ) const { return Extract( m_pos ); }
			TString Extract( const Range<size_t>& pos ) const { return range::Extract( pos, m_text ); }
			TString ExtractPrefix( void ) const { return range::Extract( utl::MakeRange( static_cast<size_t>( 0 ), m_pos.m_start ), m_text ); }
			TString ExtractSuffix( void ) const { return range::Extract( utl::MakeRange( m_pos.m_end, m_text.length() ), m_text ); }
			TString ExtractLead( size_t endPos ) const { return range::Extract( utl::MakeRange( m_pos.m_start, endPos ), m_text ); }
			TString ExtractTrail( size_t startPos ) const { return range::Extract( utl::MakeRange( startPos, m_pos.m_end ), m_text ); }

			CStringRange MakeLead( size_t endPos ) const { return CStringRange( m_text, utl::MakeRange( m_pos.m_start, endPos ) ); }
			CStringRange MakeTrail( size_t startPos ) const { return CStringRange( m_text, utl::MakeRange( startPos, m_pos.m_end ) ); }

			void SplitPair( TString& rLeading, TString& rTrailing, const Range<size_t>& sepPos ) const
			{
				rLeading = ExtractLead( sepPos.m_start );
				rTrailing = ExtractTrail( sepPos.m_end );
			}

			std::pair<CStringRange, CStringRange> GetSplitPair( const Range<size_t>& sepPos ) const
			{
				return std::pair<CStringRange, CStringRange>(
					CStringRange( m_text, utl::MakeRange( m_pos.m_start, sepPos.m_start ) ),
					CStringRange( m_text, utl::MakeRange( sepPos.m_end, m_pos.m_end ) ) );
			}

			bool HasPrefix( const CharT* pPrefix ) const { return _HasPrefix( MakeSequence( pPrefix ) ); }
			bool HasPrefix( CharT prefixCh ) const { return _HasPrefix( CSequence<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool HasSuffix( const CharT* pSuffix ) const { return _HasSuffix( MakeSequence( pSuffix ) ); }
			bool HasSuffix( CharT suffixCh ) const { return _HasSuffix( CSequence<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool StripPrefix( const CharT* pPrefix ) { return _StripPrefix( MakeSequence( pPrefix ) ); }
			bool StripPrefix( CharT prefixCh ) { return _StripPrefix( CSequence<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ) ); }

			bool StripSuffix( const CharT* pSuffix ) { return _StripSuffix( MakeSequence( pSuffix ) ); }
			bool StripSuffix( CharT suffixCh ) { return _StripSuffix( CSequence<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Strip( const CharT* pPrefix, const CharT* pSuffix ) { return _Strip( MakeSequence( pPrefix ), MakeSequence( pSuffix ) ); }
			bool Strip( CharT prefixCh, CharT suffixCh ) { return _Strip( CSequence<CharT>( &prefixCh, prefixCh != 0 ? 1 : 0 ), CSequence<CharT>( &suffixCh, suffixCh != 0 ? 1 : 0 ) ); }

			bool Find( Range<size_t>& rFoundPos, const CharT* pSeq, str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find<str::Case>( rFoundPos, MakeSequence( pSeq ) )
					: _Find<str::IgnoreCase>( rFoundPos, MakeSequence( pSeq ) );
			}

			bool Find( Range<size_t>& rFoundPos, CharT ch, str::CaseType caseType = str::Case ) const
			{
				return str::Case == caseType
					? _Find<str::Case>( rFoundPos, CSequence<CharT>( &ch, ch != 0 ? 1 : 0 ) )
					: _Find<str::IgnoreCase>( rFoundPos, CSequence<CharT>( &ch, ch != 0 ? 1 : 0 ) );
			}

			bool Equals( const CharT* pSeq, str::CaseType caseType = str::Case )
			{
				ASSERT( InBounds() );
				return str::EqualsN_ByCase( caseType, m_text.c_str() + m_pos.m_start, pSeq, GetLength() );
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
			bool _HasPrefix( const CSequence<CharT>& prefix ) const
			{
				ASSERT( InBounds() );
				return
					0 == prefix.m_length ||			// empty is always a match
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_start ], prefix.m_pSeq, prefix.m_length );
			}

			bool _HasSuffix( const CSequence<CharT>& suffix ) const
			{
				ASSERT( InBounds() );
				if ( 0 == suffix.m_length )
					return true;					// empty is always a match

				return
					m_pos.m_end >= suffix.m_length &&
					pred::Equal == CharTraits::CompareN( &m_text[ m_pos.m_end - suffix.m_length ], suffix.m_pSeq, suffix.m_length );
			}

			bool _StripPrefix( const CSequence<CharT>& prefix )
			{
				if ( !_HasPrefix( prefix ) )
					return false;

				m_pos.m_start += prefix.m_length;
				return true;		// changed
			}

			bool _StripSuffix( const CSequence<CharT>& suffix )
			{
				if ( !_HasSuffix( suffix ) )
					return false;

				m_pos.m_end -= suffix.m_length;
				return true;		// changed
			}

			bool _Strip( const CSequence<CharT>& prefix, const CSequence<CharT>& suffix )
			{
				bool hasPrefix = _StripPrefix( prefix ), hasSuffix = _StripSuffix( suffix );
				return hasPrefix && hasSuffix;
			}

			template< str::CaseType caseType >
			bool _Find( Range<size_t>& rFoundPos, const CSequence<CharT>& seq ) const
			{
				ASSERT( InBounds() );

				typedef const CharT* iterator;

				iterator itBegin = m_text.c_str() + m_pos.m_start, itEnd = m_text.c_str() + m_pos.m_end;
				iterator itFound = std::search( itBegin, itEnd, seq.Begin(), seq.End(), pred::CharEqual<caseType>());
				if ( itFound == itEnd )
					return false;

				rFoundPos.m_start = std::distance( m_text.c_str(), itFound );
				rFoundPos.m_end = rFoundPos.m_start + seq.m_length;
				return true;
			}
		private:
			const TString& m_text;
			Range<size_t> m_pos;
		};
	}


	typedef str::range::CStringRange<TCHAR> TStringRange;
}


#endif // StringRange_h
