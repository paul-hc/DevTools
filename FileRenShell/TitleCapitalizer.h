#ifndef WordCapitalizer_h
#define WordCapitalizer_h
#pragma once

#include <vector>
#include "utl/StringUtilities.h"


class CEnumTags;


namespace cap
{
	enum CaseModify { Preserve, UpperCase, LowerCase };
	const CEnumTags& GetTags_CaseModify( void );


	struct CWordList
	{
		CWordList( const TCHAR* pWords ) { SetFlatList( pWords ); }

		std::tstring GetFlatList( void ) const { return str::Unsplit( m_words, m_listSep ); }
		void SetFlatList( const TCHAR* pWords ) { str::Split( m_words, pWords, m_listSep ); }

		template< typename TokenIterator >
		size_t FindMatch( TokenIterator& rIter ) const
		{
			return rIter.FindMatch( m_words );
		}

		void FormatAt( std::tstring& rToken, size_t pos ) const
		{
			ASSERT( pos < m_words.size() );
			rToken = m_words[ pos ];
		}
	public:
		std::vector< std::tstring > m_words;
		static const TCHAR m_listSep[];
	};


	struct CWordCaseRule : public CWordList
	{
		CWordCaseRule( const TCHAR* pWords, CaseModify caseModify ) : CWordList( pWords ), m_caseModify( caseModify ) {}

		void RegSave( const TCHAR* pSection, const TCHAR* pEntry ) const;
		bool RegLoad( const TCHAR* pSection, const TCHAR* pEntry );

		template< typename TokenIterator, typename Pred >
		size_t FindWordMatch( TokenIterator& rIter, const Pred& wordBreakPred ) const
		{
			return rIter.FindWordMatch( m_words, wordBreakPred );
		}

		template< typename TokenIterator >
		void FormatWordAt( std::tstring& rToken, size_t pos, const TokenIterator& it, const std::locale& loc ) const
		{
			ASSERT( pos < m_words.size() );
			rToken = it.MakePrevToken( m_words[ pos ].length() );
			switch ( m_caseModify )
			{
				case UpperCase: str::ToUpper( rToken, loc ); break;
				case LowerCase: str::ToLower( rToken, loc ); break;
			}
		}
	public:
		CaseModify m_caseModify;
		static const TCHAR m_valueSep[];
	};
}


struct CCapitalizeOptions
{
	CCapitalizeOptions( void );

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;
public:
	std::tstring m_wordBreakChars;
	cap::CWordList m_wordBreakPrefixes;
	cap::CWordCaseRule m_alwaysPreserveWords;
	cap::CWordCaseRule m_alwaysUppercaseWords;
	cap::CWordCaseRule m_alwaysLowercaseWords;
	cap::CWordCaseRule m_articles;
	cap::CWordCaseRule m_conjunctions;				// coordonating conjunctions
	cap::CWordCaseRule m_prepositions;
	bool m_skipNumericPrefix;
};


class CTitleCapitalizer
{
public:
	CTitleCapitalizer( void );
	~CTitleCapitalizer();

	std::tstring GetCapitalized( const std::tstring& text ) const;
	void Capitalize( std::tstring& rText ) const { rText = GetCapitalized( rText ); }
private:
	const std::locale& m_locale;
	CCapitalizeOptions m_options;
};


namespace pred
{
	inline bool IsNumericPrefix( TCHAR ch )
	{
		switch ( ch )
		{
			case _T(' '):
			case _T('-'):
				return true;
			default:
				return _istdigit( ch ) != FALSE;
		}
	}


	struct IsWordBreakChar
	{
		IsWordBreakChar( const std::tstring& wordBreakChars ) : m_wordBreakChars( wordBreakChars ) {}

		bool operator()( TCHAR ch ) const
		{
			return _T('\0') == ch || m_wordBreakChars.find( ch ) != std::tstring::npos;
		}
	private:
		const std::tstring& m_wordBreakChars;
	};
}


#endif // WordCapitalizer_h
