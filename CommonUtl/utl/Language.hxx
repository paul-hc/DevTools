#ifndef Language_hxx
#define Language_hxx
#pragma once

#include "Language.h"


namespace code
{
	// CEscaper template methods

	template< typename StringT >
	StringT CEscaper::Decode( const StringT& srcLiteral ) const
	{
		StringT actualCode;
		std::back_insert_iterator<StringT> itOut = std::back_inserter( actualCode );

		actualCode.reserve( srcLiteral.length() );

		for ( typename StringT::const_iterator it = srcLiteral.begin(); it != srcLiteral.end(); ++itOut )
			*itOut = DecodeCharAdvance( &it );

		return actualCode;
	}

	template< typename StringT >
	StringT CEscaper::Encode( const StringT& actualCode, bool enquote, const char* pRetain /*= nullptr*/ ) const
	{
		StringT literalText;
		std::back_insert_iterator<StringT> itOut = std::back_inserter( literalText );

		literalText.reserve( actualCode.length() * 2 );

		if ( enquote )
			*itOut++ = '"';

		for ( typename StringT::const_iterator it = actualCode.begin(); it != actualCode.end(); ++it )
			AppendEncodedChar( itOut, *it, enquote, pRetain );

		if ( enquote )
			*itOut = '"';

		return literalText;
	}


	template< typename CharT >
	typename CharT CEscaper::DecodeCharAdvance( const CharT** ppSrc _in_out_ ) const
	{
		ASSERT( ppSrc != nullptr && *ppSrc != nullptr );
		if ( '\0' == **ppSrc )
		{
			ASSERT( **ppSrc != '\0' );
			return **ppSrc;
		}

		CharT actualChar = *(*ppSrc)++;				// interpreted character

		if ( m_escSeqLead != '\0' )					// not a no-op escaper
			if ( m_escSeqLead == actualChar )		// the escaper
			{
				CharT tokenChar = **ppSrc;			// escaped token character ('t', 'r', 'n', etc)

				if ( m_escSeqLead == tokenChar )	// double escaper
				{
					actualChar = tokenChar;
					++*ppSrc;
				}
				else
				{	// escape sequence:
					size_t pos = m_escaped.find( static_cast<char>( tokenChar ) );

					if ( pos != std::string::npos )
					{
						actualChar = m_actual[ pos ];
						++*ppSrc;
					}
					else
					{
						CharT* pNumEnd;

						if ( m_hexPrefix != '\0' && m_hexPrefix == tokenChar )
							actualChar = static_cast<CharT>( num::StringToUnsignedInteger<unsigned int>( ++*ppSrc, &pNumEnd, 16 ) );
						else
							actualChar = static_cast<CharT>( num::StringToUnsignedInteger<unsigned int>( *ppSrc, &pNumEnd, 8 ) );

						if ( pNumEnd == *ppSrc )
							actualChar = static_cast<CharT>( s_unknownChr );				// output unknown placeholder
						else
							std::advance( *ppSrc, pNumEnd - *ppSrc );		// skip literal digits
					}
				}
			}

		return actualChar;
	}

	template< typename CharT >
	typename CharT CEscaper::DecodeChar( const CharT* pSrc, size_t* pLength /*= nullptr*/ _out_ ) const
	{
		const CharT* pNext = pSrc;
		CharT actualChar = DecodeCharAdvance( &pNext );
		utl::AssignPtr( pLength, utl::Distance<size_t>( pSrc, pNext ) );
		return actualChar;
	}


	template< typename IteratorT >
	inline typename IteratorT::value_type CEscaper::DecodeCharAdvance( IteratorT* pIt _in_out_ ) const
	{
		ASSERT_PTR( pIt );
		size_t length;
		typename IteratorT::value_type actualChar = DecodeChar( &**pIt, &length );

		std::advance( *pIt, length );
		return actualChar;
	}

	template< typename IteratorT >
	typename IteratorT::value_type CEscaper::DecodeChar( IteratorT it, size_t* pLength /*= nullptr*/ _out_ ) const
	{
		IteratorT itNext = it;
		typename IteratorT::value_type actualChar = DecodeCharAdvance( &itNext );
		utl::AssignPtr( pLength, utl::Distance<size_t>( it, itNext ) );
		return actualChar;
	}


	template< typename OutIteratorT, typename CharT >
	void CEscaper::AppendEncodedChar( OutIteratorT itLiteral _out_, CharT actualCh, bool enquote, const char* pRetain /*= nullptr*/ ) const
	{
		if ( '\0' == m_escSeqLead )					// a no-op escaper
			*itLiteral++ = actualCh;
		else if ( m_escSeqLead == actualCh )		// the actual escaper?
		{	// double it up
			*itLiteral++ = actualCh;
			*itLiteral++ = actualCh;
		}
		else
		{
			size_t pos = m_actual.find( static_cast<char>( actualCh ) );

			if ( pos != std::string::npos )
			{
				if ( nullptr == pRetain )
					pRetain = m_encRetain.c_str();		// use default retained sequences

				if ( str::IsAnyOf( static_cast<char>( actualCh ), pRetain ) )			// retain actualCh as is?
					*itLiteral++ = actualCh;
				else
				{
					*itLiteral++ = static_cast<CharT>( m_escSeqLead );
					*itLiteral++ = static_cast<CharT>( m_escaped[ pos ] );

					if ( enquote && '\n' == actualCh )
						if ( !m_encLineContinue.empty() )
							std::copy( m_encLineContinue.begin(), m_encLineContinue.end(), itLiteral );		// break string literal and continue on next line
				}
			}
			else if ( pred::IsPrint()( actualCh ) )		// a straight printable?
				*itLiteral++ = actualCh;
			else
			{
				std::basic_string<CharT> hexDigits = FormatHexCh( actualCh );
				std::copy( hexDigits.begin(), hexDigits.end(), itLiteral );
			}
		}
	}
}


#endif // Language_hxx
