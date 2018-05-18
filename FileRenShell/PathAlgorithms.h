#ifndef PathAlgorithms_h
#define PathAlgorithms_h
#pragma once

#include "utl/Path.h"
#include "utl/StringUtilities.h"
#include "TitleCapitalizer.h"


class CEnumTags;


enum ChangeCase { LowerCase, UpperCase, FnameLowerCase, FnameUpperCase, ExtLowerCase, ExtUpperCase, NoExt };
const CEnumTags& GetTags_ChangeCase( void );


namespace func
{
	struct MakeCase
	{
		MakeCase( ChangeCase changeCase ) : m_changeCase( changeCase ) {}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			switch ( m_changeCase )
			{
				case LowerCase:			str::ToLower( rDestParts.m_fname ); str::ToLower( rDestParts.m_ext ); break;
				case UpperCase:			str::ToUpper( rDestParts.m_fname ); str::ToUpper( rDestParts.m_ext ); break;
				case FnameLowerCase:	str::ToLower( rDestParts.m_fname ); break;
				case FnameUpperCase:	str::ToUpper( rDestParts.m_fname ); break;
				case ExtLowerCase:		str::ToLower( rDestParts.m_ext ); break;
				case ExtUpperCase:		str::ToUpper( rDestParts.m_ext ); break;
				case NoExt:				rDestParts.m_ext.clear(); break;
			}
		}
	private:
		ChangeCase m_changeCase;
	};


	struct CapitalizeWords
	{
		CapitalizeWords( const CTitleCapitalizer* pCapitalizer ) : m_pCapitalizer( pCapitalizer ) { ASSERT_PTR( m_pCapitalizer ); }

		void operator()( fs::CPathParts& rDestParts ) const
		{
			m_pCapitalizer->Capitalize( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	private:
		const CTitleCapitalizer* m_pCapitalizer;
	};


	struct ReplaceDelimiterSet
	{
		ReplaceDelimiterSet( const std::tstring& delimiters, const std::tstring& newDelimiter )
			: m_delimiters( delimiters ), m_newDelimiter( newDelimiter ) { ASSERT( !m_delimiters.empty() ); }

		void operator()( fs::CPathParts& rDestParts ) const
		{
			str::ReplaceDelimiters( rDestParts.m_fname, m_delimiters.c_str(), m_newDelimiter.c_str() );
			str::EnsureSingleSpace( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	private:
		const std::tstring& m_delimiters;
		const std::tstring& m_newDelimiter;
	};


	struct ReplaceText
	{
		ReplaceText( const std::tstring& pattern, const std::tstring& replaceWith, bool matchCase, bool commit = true )
			: m_pattern( pattern )
			, m_replaceWith( replaceWith )
			, m_matchCase( matchCase )
			, m_commit( commit )
			, m_patternLen( static_cast< unsigned int >( m_pattern.size() ) )
			, m_matchCount( 0 )
		{
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			std::tstring fname; fname.reserve( rDestParts.m_fname.size() * 2 );

			for ( const TCHAR* pSource = rDestParts.m_fname.c_str(); *pSource != _T('\0'); )
				if ( str::EqualsN( pSource, m_pattern.c_str(), m_patternLen, m_matchCase ) )
				{
					fname += m_replaceWith;
					pSource += m_patternLen;
					++m_matchCount;
				}
				else
					fname += *pSource++;

			if ( m_commit )
				rDestParts.m_fname = fname;
		}
	private:
		const std::tstring& m_pattern;
		const std::tstring& m_replaceWith;
		bool m_matchCase;
		bool m_commit;
		unsigned int m_patternLen;
	public:
		mutable unsigned int m_matchCount;
	};


	struct ReplaceCharacters
	{
		ReplaceCharacters( const std::tstring& findCharSet, const std::tstring& replaceWith, bool matchCase, bool commit = true )
			: m_findCharSet( findCharSet )
			, m_replaceWith( replaceWith )
			, m_matchCase( matchCase )
			, m_commit( commit )
			, m_matchCount( 0 )
		{
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			std::tstring fname;

			for ( const TCHAR* pSource = rDestParts.m_fname.c_str(); *pSource != _T('\0'); ++pSource )
				if ( IsOneOfFindCharSet( *pSource ) )
				{
					fname += m_replaceWith;
					++m_matchCount;
				}
				else
					fname += *pSource;

			if ( m_commit )
				rDestParts.m_fname = fname;
		}

		bool IsOneOfFindCharSet( TCHAR ch ) const
		{
			for ( const TCHAR* pCursor = m_findCharSet.c_str(); *pCursor != NULL; ++pCursor )
				if ( m_matchCase ? ( *pCursor == ch ) : ( _totlower( *pCursor ) == _totlower( ch ) ) )
					return true;

			return false;
		}
	private:
		const std::tstring& m_findCharSet;
		const std::tstring& m_replaceWith;
		bool m_matchCase;
		bool m_commit;
	public:
		mutable int m_matchCount;
	};


	struct SingleWhitespace
	{
		void operator()( fs::CPathParts& rDestParts ) const
		{
			str::EnsureSingleSpace( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	};


	struct RemoveWhitespace
	{
		void operator()( fs::CPathParts& rDestParts ) const
		{
			str::ReplaceDelimiters( rDestParts.m_fname, _T(" \t"), _T("") );
			str::ToLower( rDestParts.m_ext );
		}
	};
}


#endif // PathAlgorithms_h
