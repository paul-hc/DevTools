
#include "stdafx.h"
#include "TextAlgorithms.h"
#include "GeneralOptions.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CEnumTags& GetTags_ChangeCase( void )
{
	static const CEnumTags tags( _T("filename.ext|FILENAME.EXT|filename.*|FILENAME.*|*.ext|*.EXT|* (no ext)") );
	return tags;
}


const std::tstring delim::s_defaultDelimiterSet = _T(".;-_ \t");

const std::tstring delim::s_dashes = _T("-\xad\x2012\x2013\x2014\x2015\x2212\x02d7\x0335\x0336");
const std::tstring delim::s_graveAccents = _T("`\x0218\x02cb");
const std::tstring delim::s_accuteAccents = _T("´\x02c8\x02ca");
const std::tstring delim::s_singleQuotes = _T("'`\x0219\x021b\x0232\x02b9\x02bb");			// also replaces ` with '
const std::tstring delim::s_doubleQuotes = _T("\"\x021c\x021d\x021e\x021f\x0233\x0234\x02ba\x02dd");
const std::tstring delim::s_commas = _T(",\x201a\x02cb\x02ce\02cf");
const std::tstring delim::s_underscores = _T("_\x02cd");
const std::tstring delim::s_overscores = _T("¯\x02c9");

const std::tstring& delim::GetAllDelimitersSet( void )
{
	static const std::tstring s_allDelimitersSet =
		s_defaultDelimiterSet +
		s_dashes +
		s_underscores +
		s_overscores;

	return s_allDelimitersSet;
}


namespace text_tool
{
	const std::vector< std::pair< std::tstring, std::tstring > >& GetStdUnicodeToAnsiPairs( void )
	{
		static std::vector< std::pair< std::tstring, std::tstring > > s_stdPairs;

		if ( s_stdPairs.empty() )
		{
			s_stdPairs.reserve( 16 );

			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_dashes ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_graveAccents ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_accuteAccents ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_singleQuotes ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_doubleQuotes ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_commas ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_underscores ) );
			s_stdPairs.push_back( MakeStdDelimsPair( delim::s_overscores ) );
		}
		return  s_stdPairs;
	}

	std::pair< std::tstring, std::tstring > MakeStdDelimsPair( const std::tstring& delimSet )
	{
		ASSERT( delimSet.length() >= 2 );
		return std::pair< std::tstring, std::tstring >( &delimSet[ 1 ], delimSet.substr( 0, 1 ) );
	}
}


namespace func
{
	void TrimFname( std::tstring& rFname )
	{
		if ( CGeneralOptions::Instance().m_trimFname )
			str::Trim( rFname );

		if ( CGeneralOptions::Instance().m_normalizeWhitespace )
			str::EnsureSingleSpace( rFname );
	}


	// MakeCase implementation

	void MakeCase::operator()( std::tstring& rDestText ) const
	{
		switch ( m_changeCase )
		{
			case LowerCase:			str::ToLower( rDestText ); break;
			case UpperCase:			str::ToUpper( rDestText ); break;
			case FnameLowerCase:	str::ToLower( rDestText ); break;
			case FnameUpperCase:	str::ToUpper( rDestText ); break;
		}

		TrimFname( rDestText );
	}

	void MakeCase::operator()( fs::CPathParts& rDestParts ) const
	{
		operator()( rDestParts.m_fname );

		switch ( m_changeCase )
		{
			case LowerCase:			str::ToLower( rDestParts.m_ext ); break;
			case UpperCase:			str::ToUpper( rDestParts.m_ext ); break;
			case ExtLowerCase:		str::ToLower( rDestParts.m_ext ); break;
			case ExtUpperCase:		str::ToUpper( rDestParts.m_ext ); break;
			case NoExt:				rDestParts.m_ext.clear(); break;
		}
	}


	// ReplaceMultiDelimiterSets implementation

	void ReplaceMultiDelimiterSets::operator()( std::tstring& rDestText ) const
	{
		for ( std::vector< std::pair< std::tstring, std::tstring > >::const_iterator itPair = m_pDelimsToNewPairs->begin(); itPair != m_pDelimsToNewPairs->end(); ++itPair )
			str::ReplaceDelimiters( rDestText, itPair->first.c_str(), itPair->second.c_str() );

		TrimFname( rDestText );
	}


	// ReplaceText implementation

	void ReplaceText::operator()( std::tstring& rDestText ) const
	{
		std::tstring newText; newText.reserve( rDestText.size() * 2 );

		for ( const TCHAR* pSource = rDestText.c_str(); *pSource != _T('\0'); )
			if ( str::EqualsN_ByCase( m_caseType, pSource, m_pattern.c_str(), m_patternLen ) )
			{
				newText += m_replaceWith;
				pSource += m_patternLen;
				++m_matchCount;
			}
			else
				newText += *pSource++;

		TrimFname( newText );

		if ( m_commit )
			rDestText = newText;
	}


	// ReplaceCharacters implementation

	void ReplaceCharacters::operator()( std::tstring& rDestText ) const
	{
		std::tstring newText;

		for ( const TCHAR* pSource = rDestText.c_str(); *pSource != _T('\0'); ++pSource )
			if ( IsOneOfFindCharSet( *pSource ) )
			{
				newText += m_replaceWith;
				++m_matchCount;
			}
			else
				newText += *pSource;

		TrimFname( newText );

		if ( m_commit )
			rDestText = newText;
	}

	bool ReplaceCharacters::IsOneOfFindCharSet( TCHAR ch ) const
	{
		for ( const TCHAR* pCursor = m_findCharSet.c_str(); *pCursor != NULL; ++pCursor )
			if ( m_matchCase ? ( *pCursor == ch ) : ( _totlower( *pCursor ) == _totlower( ch ) ) )
				return true;

		return false;
	}
}
