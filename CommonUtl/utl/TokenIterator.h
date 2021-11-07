#ifndef TokenIterator_h
#define TokenIterator_h
#pragma once

#include "Range.h"
#include "StringCompare.h"


namespace str
{
	template< typename CompareT = pred::TCompareCase, typename CharType = TCHAR >
	struct CTokenIterator
	{
		typedef std::basic_string< CharType > TString;

		explicit CTokenIterator( const TString& text, size_t pos = 0, CompareT compare = CompareT() )
			: m_text( text )
			, m_pCurrent( m_text.c_str() )
			, m_pos( 0 )
			, m_length( m_text.length() )
			, m_compare( compare )
			, m_equals( compare )
		{
			SetWhiteSpace( str::StdWhitespace< CharType >() );
			SetPos( pos );
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

		size_t GetPos( void ) const { REQUIRE( m_pos <= m_length ); return m_pos; }

		void SetPos( size_t pos )
		{
			m_pos = pos;
			m_pCurrent = m_text.c_str() + m_pos;
			ENSURE( m_pos <= m_length );
		}

		const CharType* GetWhiteSpace( void ) const { return m_whiteSpace.m_start; }
		void SetWhiteSpace( const CharType whiteSpace[] ) { ASSERT( !str::IsEmpty( whiteSpace ) ); m_whiteSpace.SetRange( whiteSpace, str::end( whiteSpace ) ); }


		TString GetLeadSubstr( void ) const { return m_text.substr( 0, m_pos ); }
		TString GetCurrentSubstr( void ) const { return m_text.substr( m_pos ); }
		TString MakePrevToken( size_t tokenLen ) const { ASSERT( tokenLen <= m_pos ); return m_text.substr( m_pos - tokenLen, tokenLen ); }
		TString MakeToken( size_t tokenLen ) const { ASSERT( tokenLen <= m_pos ); return m_text.substr( m_pos, tokenLen ); }

		bool ExtractEnclosedText( TString& rText, const CharType openSep[], const CharType closeSep[], Range<size_t>* pTextRange = &s_posRange )
		{
			ASSERT_PTR( pTextRange );
			if ( Matches( openSep ) )
			{
				pTextRange->SetEmptyRange( m_pos );
				if ( FindToken( closeSep ) )
				{
					pTextRange->m_end = m_pos - str::GetLength( closeSep );
					rText = m_text.substr( pTextRange->m_start, pTextRange->GetSpan<size_t>() );
					return true;
				}
			}
			return false;
		}

		bool ExtractEnclosedText( TString& rText, const CharType sep[], Range<size_t>* pTextRange = &s_posRange ) { return ExtractEnclosedText( rText, sep, sep, pTextRange ); }

		void ReplaceToken( const Range<size_t>& tokenRange, const CharType* pNewText )
		{
			REQUIRE( tokenRange.m_start <= m_length );
			REQUIRE( tokenRange.m_end <= m_length );
			ASSERT_PTR( pNewText );

			size_t tokenLen = tokenRange.GetSpan<size_t>();
			ptrdiff_t deltaLen = str::GetLength( pNewText ) - tokenLen;

			TString& rText = const_cast<TString&>( m_text );		// writeable string data-member

			rText.replace( tokenRange.m_start, tokenLen, pNewText );
			m_length = m_text.length();

			if ( m_pos >= tokenRange.m_end )			// at or past the token end
				SetPos( m_pos + deltaLen );				// shift current pos by the difference in length

			// "abc" -> "something"		delta=9-3=6
		}


		// advance

		CTokenIterator& operator++( void )		// pre increment
		{
			ASSERT( m_pos < m_length );
			SetPos( m_pos + 1 );
			return *this;
		}


		// search

		CTokenIterator& SkipWhiteSpace( void )
		{
			while ( !AtEnd() && std::find( m_whiteSpace.m_start, m_whiteSpace.m_end, Current() ) != m_whiteSpace.m_end )
				SetPos( m_pos + 1 );

			return *this;
		}

		CTokenIterator& SkipDelim( CharType delim )
		{
			Matches( delim );
			return *this;
		}

		CTokenIterator& SkipDelims( const CharType delims[] )
		{
			const char* delimsEnd = str::end( delims );
			while ( !AtEnd() && std::find( delims, delimsEnd, Current() ) != delimsEnd )
				SetPos( m_pos + 1 );

			return *this;
		}

		bool FindTokenStart( const CharType* pToken ) { return FindToken( pToken, false ); }

		bool FindToken( const CharType* pToken, bool goPastToken = true )
		{
			if ( !AtEnd() )
			{
				size_t tokenLen = str::GetLength( pToken );
				typename TString::const_iterator itFound = std::search( m_text.begin() + m_pos, m_text.end(), pToken, pToken + tokenLen, m_equals );
				if ( itFound != m_text.end() )
				{
					size_t newPos = std::distance( m_text.begin(), itFound );
					if ( goPastToken )
						newPos += tokenLen;

					SetPos( newPos );
					return true;
				}
			}
			return false;
		}

		// if there is a match it skips current token

		bool Matches( CharType token )
		{
			if ( !m_equals( m_text[ m_pos ], token ) )
				return false;
			SetPos( m_pos + 1 );
			return true;
		}

		bool Matches( const CharType* pToken )
		{
			ASSERT_PTR( pToken );
			size_t tokenLen = str::GetLength( pToken );
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, pToken, tokenLen ) != pred::Equal )
				return false;

			SetPos( m_pos + tokenLen );
			return true;
		}

		bool Matches( const TString& token )
		{
			size_t tokenLen = token.length();
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, token.c_str(), tokenLen ) != pred::Equal )
				return false;

			SetPos( m_pos + tokenLen );
			return true;
		}

		template< typename Container >
		size_t FindMatch( const Container& tokens )
		{
			for ( size_t pos = 0; pos != tokens.size(); ++pos )
				if ( Matches( tokens[ pos ] ) )
					return pos;

			return TString::npos;
		}

		template< typename Container >
		bool MatchesAny( const Container& tokens )
		{
			return FindMatch( tokens ) != TString::npos;
		}


		// word matching

		template< typename WordBreakPred >
		bool MatchesWord( const TString& token, WordBreakPred isWordBreak )
		{
			size_t tokenLen = token.length();
			if ( 0 == tokenLen || m_compare( m_text.c_str() + m_pos, token.c_str(), tokenLen ) != pred::Equal )
				return false;
			if ( !isWordBreak( m_text.c_str()[ m_pos + tokenLen ] ) )		// check for token-end word breaks, etc
				return false;

			SetPos( m_pos + tokenLen );
			return true;
		}

		template< typename Container, typename WordBreakPred >
		size_t FindWordMatch( const Container& tokens, WordBreakPred isWordBreak )
		{
			for ( size_t pos = 0; pos != tokens.size(); ++pos )
				if ( MatchesWord( tokens[ pos ], isWordBreak ) )
					return pos;

			return TString::npos;
		}
	private:
		const TString& m_text;
		const CharType* m_pCurrent;				// self-encapsulated
		size_t m_pos;							// self-encapsulated
		size_t m_length;
		Range<const CharType*> m_whiteSpace;

		static Range<size_t> s_posRange;		// default parameter for extract methods
	public:
		CompareT m_compare;
		pred::IsEqual< CompareT > m_equals;
	};


	// CTokenIterator template code

	template< typename CompareT, typename CharType >
	Range<size_t> CTokenIterator< CompareT, CharType >::s_posRange( 0 );		// static s_posRange object definition
}


#endif // TokenIterator_h
