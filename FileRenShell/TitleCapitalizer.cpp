
#include "stdafx.h"
#include "TitleCapitalizer.h"
#include "utl/EnumTags.h"
#include "utl/TokenIterator.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("TitleCapitalization");
	static const TCHAR entry_wordBreakChars[] = _T("Word Break Symbols");
	static const TCHAR entry_wordBreakPrefixes[] = _T("Word Break Prefixes");
	static const TCHAR entry_alwaysPreserveWords[] = _T("Always Preserve Words");
	static const TCHAR entry_alwaysUppercaseWords[] = _T("Always Uppercase Words");
	static const TCHAR entry_alwaysLowercaseWords[] = _T("Always Lowercase Words");
	static const TCHAR entry_articles[] = _T("Articles");
	static const TCHAR entry_conjunctions[] = _T("Conjunctions");
	static const TCHAR entry_prepositions[] = _T("Prepositions");
	static const TCHAR entry_skipNumericPrefix[] = _T("Skip Numeric Prefix");
}


namespace cap
{
	const CEnumTags& GetTags_CaseModify( void )
	{
		static const CEnumTags& tags( str::Load( IDS_CASE_MODIFY_TAGS ) );
		return tags;
	}


	// CWordList implementation

	const TCHAR CWordList::m_listSep[] = _T(",");


	// CWordCaseRule implementation

	void CWordCaseRule::RegSave( const TCHAR* pSection, const TCHAR* pEntry ) const
	{
		AfxGetApp()->WriteProfileString( pSection, pEntry, str::Format( _T("%d:%s"), m_caseModify, GetFlatList().c_str() ).c_str() );
	}

	bool CWordCaseRule::RegLoad( const TCHAR* pSection, const TCHAR* pEntry )
	{
		enum Tokens { CaseChange, Words, TokenCount };
		std::vector< std::tstring > tokens;
		str::Split( tokens, (LPCTSTR)AfxGetApp()->GetProfileString( pSection, pEntry ), _T(":") );

		if ( TokenCount == tokens.size() )
		{
			int caseModify;
			if ( num::ParseNumber( caseModify, tokens[ CaseChange ] ) )
				if ( caseModify >= Preserve && caseModify <= LowerCase )
				{
					SetFlatList( tokens[ Words ].c_str() );
					m_caseModify = static_cast< CaseModify >( caseModify );
					return true;
				}
		}

		return false;
	}
}


// CCapitalizeOptions implementation

CCapitalizeOptions::CCapitalizeOptions( void )
	: m_wordBreakChars( _T(" \t()[]{}0123456789.~`!@#$%^&*-_+=|\\:;\"<>,?/") )
	, m_wordBreakPrefixes( _T("Mc,O',N',3D") )
	, m_alwaysPreserveWords( _T(""), cap::Preserve )
	, m_alwaysUppercaseWords( _T("ABBA,AC/DC,AC-DC,DJ,MC,MLTR,TLC,LL,II,III,UB40"), cap::UpperCase )
	, m_alwaysLowercaseWords( _T("w,with,feat,featuring,aka,vs"), cap::LowerCase )
	, m_articles( _T("the,a,an"), cap::LowerCase )
	, m_conjunctions( _T("so,or,nor,and,but,yet"), cap::LowerCase )
	, m_prepositions( _T("on,in,to,by,for,at,of,as"), cap::LowerCase )
	, m_skipNumericPrefix( true )
{
}

void CCapitalizeOptions::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();
	m_wordBreakChars = pApp->GetProfileString( reg::section, reg::entry_wordBreakChars, m_wordBreakChars.c_str() );

	m_wordBreakPrefixes.SetFlatList( pApp->GetProfileString( reg::section, reg::entry_wordBreakPrefixes, m_wordBreakPrefixes.GetFlatList().c_str() ) );
	m_alwaysPreserveWords.SetFlatList( pApp->GetProfileString( reg::section, reg::entry_alwaysPreserveWords, m_alwaysPreserveWords.GetFlatList().c_str() ) );
	m_alwaysUppercaseWords.SetFlatList( pApp->GetProfileString( reg::section, reg::entry_alwaysUppercaseWords, m_alwaysUppercaseWords.GetFlatList().c_str() ) );
	m_alwaysLowercaseWords.SetFlatList( pApp->GetProfileString( reg::section, reg::entry_alwaysLowercaseWords, m_alwaysLowercaseWords.GetFlatList().c_str() ) );
	m_articles.RegLoad( reg::section, reg::entry_articles );
	m_conjunctions.RegLoad( reg::section, reg::entry_conjunctions );
	m_prepositions.RegLoad( reg::section, reg::entry_prepositions );
	m_skipNumericPrefix = pApp->GetProfileInt( reg::section, reg::entry_skipNumericPrefix, m_skipNumericPrefix ) != FALSE;
}

void CCapitalizeOptions::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileString( reg::section, reg::entry_wordBreakChars, m_wordBreakChars.c_str() );
	pApp->WriteProfileString( reg::section, reg::entry_wordBreakPrefixes, m_wordBreakPrefixes.GetFlatList().c_str() );
	pApp->WriteProfileString( reg::section, reg::entry_alwaysPreserveWords, m_alwaysPreserveWords.GetFlatList().c_str() );
	pApp->WriteProfileString( reg::section, reg::entry_alwaysUppercaseWords, m_alwaysUppercaseWords.GetFlatList().c_str() );
	pApp->WriteProfileString( reg::section, reg::entry_alwaysLowercaseWords, m_alwaysLowercaseWords.GetFlatList().c_str() );
	m_articles.RegSave( reg::section, reg::entry_articles );
	m_conjunctions.RegSave( reg::section, reg::entry_conjunctions );
	m_prepositions.RegSave( reg::section, reg::entry_prepositions );
	pApp->WriteProfileInt( reg::section, reg::entry_skipNumericPrefix, m_skipNumericPrefix );
}


// CTitleCapitalizer implementation

CTitleCapitalizer::CTitleCapitalizer( void )
	: m_locale( str::GetUserLocale() )
{
	m_options.LoadFromRegistry();
}

CTitleCapitalizer::~CTitleCapitalizer()
{
}

std::tstring CTitleCapitalizer::GetCapitalized( const std::tstring& text ) const
{
	std::tostringstream oss;
	enum WordStatus { TitleStart, WordBreak, WordStart, WordCore } prevStatus = TitleStart;		// was WordBreak
	const pred::IsWordBreakChar wordBreakPred( m_options.m_wordBreakChars );

	str::CTokenIterator< pred::CompareNoCase > it( text );

	if ( m_options.m_skipNumericPrefix )
		if ( !it.AtEnd() && _istdigit( *it ) )
			while ( !it.AtEnd() && pred::IsNumericPrefix( *it ) )
			{
				oss << *it;
				++it;
			}

	for ( ; !it.AtEnd(); )
		if ( wordBreakPred( *it ) )
		{
			prevStatus = WordBreak;
			oss << *it;
			++it;
		}
		else if ( TitleStart == prevStatus || WordBreak == prevStatus )
		{
			bool isTitleStart = TitleStart == prevStatus;
			prevStatus = WordStart;

			std::tstring token;

			size_t foundPos;
			if ( ( foundPos = m_options.m_wordBreakPrefixes.FindMatch( it ) ) != std::tstring::npos )
			{
				m_options.m_wordBreakPrefixes.FormatAt( token, foundPos );
				prevStatus = WordBreak;
			}
			else if ( ( foundPos = m_options.m_alwaysPreserveWords.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_alwaysPreserveWords.FormatWordAt( token, foundPos, it, m_locale );
			else if ( ( foundPos = m_options.m_alwaysUppercaseWords.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_alwaysUppercaseWords.FormatWordAt( token, foundPos, it, m_locale );
			else if ( ( foundPos = m_options.m_alwaysLowercaseWords.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_alwaysLowercaseWords.FormatWordAt( token, foundPos, it, m_locale );
			else if ( ( foundPos = m_options.m_articles.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_articles.FormatWordAt( token, foundPos, it, m_locale );
			else if ( ( foundPos = m_options.m_conjunctions.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_conjunctions.FormatWordAt( token, foundPos, it, m_locale );
			else if ( ( foundPos = m_options.m_prepositions.FindWordMatch( it, wordBreakPred ) ) != std::tstring::npos )
				m_options.m_prepositions.FormatWordAt( token, foundPos, it, m_locale );
			else
			{
				token = func::toupper( *it, m_locale );
				++it;
			}

			if ( !token.empty() && isTitleStart )
				token[ 0 ] = func::toupper( token[ 0 ], m_locale );			// always capitalize first letter in the title, on top of other case rules

			oss << token;
		}
		else
		{
			prevStatus = WordCore;
			oss << func::tolower( *it, m_locale );
			++it;
		}

	return oss.str();
}