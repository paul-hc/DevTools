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

		std::tstring GetFlatList( void ) const { return str::Join( m_words, s_listSep ); }
		void SetFlatList( const TCHAR* pWords ) { str::Split( m_words, pWords, s_listSep ); }

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

		bool operator==( const CWordList& right ) const { return m_words == right.m_words; }
	public:
		std::vector<std::tstring> m_words;
		static const TCHAR s_listSep[];
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

		bool operator==( const CWordCaseRule& right ) const { return CWordList::operator==( right ) && m_caseModify == right.m_caseModify; }
	public:
		CaseModify m_caseModify;
	};
}


#include "utl/Subject.h"


struct CCapitalizeOptions : public TSubject
{
	CCapitalizeOptions( void );

	static CCapitalizeOptions& Instance( void );	// shared instance

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;
	void PostApply( void ) const { SaveToRegistry(); }

	bool operator==( const CCapitalizeOptions& right ) const;
	bool operator!=( const CCapitalizeOptions& right ) const { return !operator==( right ); }
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
	CTitleCapitalizer( const CCapitalizeOptions* pOptions = &CCapitalizeOptions::Instance() );
	~CTitleCapitalizer();

	std::tstring GetCapitalized( const std::tstring& text ) const;
	void Capitalize( std::tstring& rText ) const { rText = GetCapitalized( rText ); }
private:
	const std::locale& m_locale;
	const CCapitalizeOptions* m_pOptions;
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
