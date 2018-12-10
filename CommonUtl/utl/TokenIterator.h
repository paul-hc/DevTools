#ifndef TokenIterator_h
#define TokenIterator_h
#pragma once

#include "Range.h"
#include "StringCompare.h"


namespace str
{
	template< typename Compare = pred::TCompareCase, typename CharType = TCHAR >
	struct CTokenIterator
	{
		typedef std::basic_string< CharType > StringT;

		explicit CTokenIterator( const StringT& text, size_t pos = 0, Compare compare = Compare() )
			: m_text( text )
			, m_length( m_text.length() )
			, m_pos( pos )
			, m_compare( compare )
		{
			ASSERT( m_pos <= m_length );
			SetWhiteSpace( str::StdWhitespace< CharType >() );
		}

		bool IsEmpty( void ) const { ASSERT( m_pos <= m_length ); return 0 == m_length; }

		CharType Front( void ) const { ASSERT( !IsEmpty() ); return m_text[ 0 ]; }
		CharType Back( void ) const { ASSERT( !IsEmpty() ); return m_text[ m_length - 1 ]; }

		bool AtEnd( void ) const { ASSERT( m_pos <= m_length ); return m_length == m_pos; }
		bool AtFront( void ) const { ASSERT( m_pos <= m_length ); return 0 == m_pos; }
		bool AtBack( void ) const { return !IsEmpty() && m_length - 1 == m_pos; }

		CharType operator*( void ) const { return Current(); }

		CharType Current( void ) const { ASSERT( !AtEnd() ); return m_text[ m_pos ]; }
		const CharType* CurrentPtr( void ) const { ASSERT( !AtEnd() ); return &m_text[ m_pos ]; }

		CharType Previous( void ) const { ASSERT( !IsEmpty() && !AtFront() ); return m_text[ m_pos - 1 ]; }
		CharType Next( void ) const { ASSERT( !IsEmpty() ); return !AtEnd() ? m_text[ m_pos - 1 ] : _T('\0'); }

		StringT GetLeadSubstr( void ) const { return m_text.substr( 0, m_pos ); }
		StringT GetCurrentSubstr( void ) const { return m_text.substr( m_pos ); }
		StringT MakePrevToken( size_t tokenLen ) const { ASSERT( tokenLen <= m_pos ); return m_text.substr( m_pos - tokenLen, tokenLen ); }
		StringT MakeToken( size_t tokenLen ) const { ASSERT( tokenLen <= m_pos ); return m_text.substr( m_pos, tokenLen ); }

		const CharType* GetWhiteSpace( void ) const { return m_whiteSpace.m_start; }
		void SetWhiteSpace( const CharType whiteSpace[] ) { ASSERT( !str::IsEmpty( whiteSpace ) ); m_whiteSpace.SetRange( whiteSpace, str::end( whiteSpace ) ); }


		// advance

		CTokenIterator& operator++( void )		// pre increment
		{
			ASSERT( m_pos < m_length );
			++m_pos;
			return *this;
		}


		// search

		CTokenIterator& SkipWhiteSpace( void )
		{
			while ( !AtEnd() && std::find( m_whiteSpace.m_start, m_whiteSpace.m_end, Current() ) != m_whiteSpace.m_end )
				++m_pos;
			return *this;
		}

		bool FindToken( const CharType* pToken, bool goPastToken = true )
		{
			if ( !AtEnd() )
			{
				size_t tokenLen = str::GetLength( pToken );
				typename StringT::const_iterator itFound = std::search( m_text.begin() + m_pos, m_text.end(), pToken, pToken + tokenLen, m_compare );
				if ( itFound != m_text.end() )
				{
					m_pos = std::distance( m_text.begin(), itFound );
					if ( goPastToken )
						m_pos += tokenLen;
					return true;
				}
			}
			return false;
		}

		// if there is a match it skips current token

		bool Matches( CharType token )
		{
			if ( m_compare( m_text[ m_pos ], token ) != pred::Equal )
				return false;
			++m_pos;
			return true;
		}

		bool Matches( const CharType* pToken )
		{
			ASSERT_PTR( pToken );
			size_t tokenLen = str::GetLength( pToken );
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, pToken, tokenLen ) != pred::Equal )
				return false;
			m_pos += tokenLen;
			return true;
		}

		bool Matches( const StringT& token )
		{
			size_t tokenLen = token.length();
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, token.c_str(), tokenLen ) != pred::Equal )
				return false;
			m_pos += tokenLen;
			return true;
		}

		template< typename Container >
		size_t FindMatch( const Container& tokens )
		{
			for ( size_t pos = 0; pos != tokens.size(); ++pos )
				if ( Matches( tokens[ pos ] ) )
					return pos;

			return StringT::npos;
		}

		template< typename Container >
		bool MatchesAny( const Container& tokens )
		{
			return FindMatch( tokens ) != StringT::npos;
		}


		// word matching

		template< typename WordBreakPred >
		bool MatchesWord( const StringT& token, WordBreakPred isWordBreak )
		{
			size_t tokenLen = token.length();
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, token.c_str(), tokenLen ) != pred::Equal )
				return false;
			if ( !isWordBreak( m_text.c_str()[ m_pos + tokenLen ] ) )		// check for token-end word breaks, etc
				return false;
			m_pos += tokenLen;
			return true;
		}

		template< typename Container, typename WordBreakPred >
		size_t FindWordMatch( const Container& tokens, WordBreakPred isWordBreak )
		{
			for ( size_t pos = 0; pos != tokens.size(); ++pos )
				if ( MatchesWord( tokens[ pos ], isWordBreak ) )
					return pos;

			return StringT::npos;
		}
	private:
		const StringT& m_text;
		Range< const CharType* > m_whiteSpace;
	public:
		const size_t m_length;
		size_t m_pos;
		Compare m_compare;
	};
}


#endif // TokenIterator_h
