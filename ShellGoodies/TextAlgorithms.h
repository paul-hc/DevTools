#ifndef TextAlgorithms_h
#define TextAlgorithms_h
#pragma once

#include "utl/Path.h"
#include "utl/StringUtilities.h"
#include "TitleCapitalizer.h"


class CEnumTags;


enum ChangeCase { LowerCase, UpperCase, FnameLowerCase, FnameUpperCase, ExtLowerCase, ExtUpperCase, NoExt };
const CEnumTags& GetTags_ChangeCase( void );


struct delim			// use as a namespace
{
	static const std::tstring& GetAllDelimitersSet( void );		// s_defaultDelimiterSet + Unicode equivalents
public:
	static const std::tstring s_defaultDelimiterSet;

	// note: first char is the standard, the rest are 'synonyms' that look similarly (usually Unicode)
	static const std::tstring s_dashes;
	static const std::tstring s_graveAccents;
	static const std::tstring s_accuteAccents;
	static const std::tstring s_singleQuotes;
	static const std::tstring s_doubleQuotes;
	static const std::tstring s_commas;
	static const std::tstring s_underscores;
	static const std::tstring s_overscores;
};


namespace text_tool
{
	const std::vector< std::pair< std::tstring, std::tstring > >& GetStdUnicodeToAnsiPairs( void );
	std::pair< std::tstring, std::tstring > MakeStdDelimsPair( const std::tstring& delimSet );		// note: first char is the replacement, the rest are to be replaced

	template< typename FuncType >
	inline void ExecuteTextTool( std::tstring& rText, const FuncType& toolFunc ) { toolFunc( rText ); }
}


namespace func
{
	void TrimFname( std::tstring& rFname );


	struct MakeCase
	{
		MakeCase( ChangeCase changeCase ) : m_changeCase( changeCase ) {}

		void operator()( std::tstring& rDestText ) const;
		void operator()( fs::CPathParts& rDestParts ) const;
	private:
		ChangeCase m_changeCase;
	};


	struct CapitalizeWords
	{
		CapitalizeWords( const CTitleCapitalizer* pCapitalizer ) : m_pCapitalizer( pCapitalizer ) { ASSERT_PTR( m_pCapitalizer ); }

		void operator()( std::tstring& rDestText ) const
		{
			m_pCapitalizer->Capitalize( rDestText );
			TrimFname( rDestText );
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			TrimFname( rDestParts.m_fname );
			operator()( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	private:
		const CTitleCapitalizer* m_pCapitalizer;
	};


	struct ReplaceDelimiterSet
	{
		ReplaceDelimiterSet( const std::tstring& delimiters, const std::tstring& newDelimiter )
			: m_delimiters( delimiters ), m_newDelimiter( newDelimiter ) { ASSERT( !m_delimiters.empty() ); }

		void operator()( std::tstring& rDestText ) const
		{
			str::ReplaceDelimiters( rDestText, m_delimiters.c_str(), m_newDelimiter.c_str() );
			TrimFname( rDestText );
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			operator()( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	private:
		const std::tstring& m_delimiters;
		const std::tstring& m_newDelimiter;
	};


	struct ReplaceMultiDelimiterSets
	{
		ReplaceMultiDelimiterSets( const std::vector< std::pair< std::tstring, std::tstring > >* pDelimsToNewPairs )
			: m_pDelimsToNewPairs( pDelimsToNewPairs )
		{
			ASSERT( m_pDelimsToNewPairs != NULL && !m_pDelimsToNewPairs->empty() );
		}

		void operator()( std::tstring& rDestText ) const;

		void operator()( fs::CPathParts& rDestParts ) const
		{
			operator()( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	private:
		const std::vector< std::pair< std::tstring, std::tstring > >* m_pDelimsToNewPairs;				// delimiters to replacement pairs
	};


	struct ReplaceText
	{
		ReplaceText( const std::tstring& pattern, const std::tstring& replaceWith, bool matchCase, bool commit = true )
			: m_pattern( pattern )
			, m_replaceWith( replaceWith )
			, m_caseType( matchCase ? str::Case : str::IgnoreCase )
			, m_commit( commit )
			, m_patternLen( static_cast<unsigned int>( m_pattern.size() ) )
			, m_matchCount( 0 )
		{
		}

		void operator()( std::tstring& rDestText ) const;
		void operator()( fs::CPathParts& rDestParts ) const { operator()( rDestParts.m_fname ); }
	private:
		const std::tstring& m_pattern;
		const std::tstring& m_replaceWith;
		str::CaseType m_caseType;
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

		void operator()( std::tstring& rDestText ) const;
		void operator()( fs::CPathParts& rDestParts ) const { operator()( rDestParts.m_fname ); }

		bool IsOneOfFindCharSet( TCHAR ch ) const;
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
		void operator()( std::tstring& rDestText ) const
		{
			str::EnsureSingleSpace( rDestText );
			TrimFname( rDestText );
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			operator()( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	};


	struct RemoveWhitespace
	{
		void operator()( std::tstring& rDestText ) const
		{
			str::ReplaceDelimiters( rDestText, _T(" \t"), _T("") );
			TrimFname( rDestText );
		}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			operator()( rDestParts.m_fname );
			str::ToLower( rDestParts.m_ext );
		}
	};
}


#endif // TextAlgorithms_h
