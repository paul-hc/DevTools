#ifndef CodeSnippetsParser_h
#define CodeSnippetsParser_h
#pragma once

#include "utl/Path.h"
#include "utl/RuntimeException.h"
#include "utl/StringParsing.h"
#include "CodeUtils.h"
#include <map>


class CCodeSnippetsParser
{
public:
	CCodeSnippetsParser( const TCHAR* pOutLineEnd = code::g_pLineEnd );
	~CCodeSnippetsParser();

	bool IsEmpty( void ) const { return m_sectionMap.empty(); }
	void Reset( void );

	void LoadFile( const fs::CPath& filePath ) throws_( CRuntimeException );
	void ParseStream( std::istream& is ) throws_( CRuntimeException );

	// literals: pairs of keys to values, e.g. "%FileName%" -> "StripBar"
	const std::tstring* FindLiteralValue( const std::tstring& literalKey ) const;	// e.g. "%FileName%"
	std::tstring& LookupLiteralValue( const std::tstring& literalKey );				// for registration

	template< typename ContainerT >
	void StoreLiterals( const ContainerT& literals ) { m_literals.insert( m_literals.end(), literals.begin(), literals.end() ); UpdateFilePathLiterals(); }

	// sections:
	bool HasSection( const std::tstring& sectionName ) const { return FindSection( sectionName ) != nullptr; }
	const std::tstring* FindSection( const std::tstring& sectionName ) const;
	std::tstring ExpandSection( const std::tstring& sectionName ) throws_( CRuntimeException );
	bool ExpandSection( OUT std::tstring* pSectionContent, const std::tstring& sectionName ) throws_( CRuntimeException );
private:
	typedef std::tstring::const_iterator TConstIterator;

	enum SepMatch
	{
		SectionSep,				// "[[...]]" TSepMatchPos of 0 in m_sectionParser
		LineCommentSep			// ";;..." TSepMatchPos of 1 in m_sectionParser
	};

	void ParseLine( const std::tstring& line ) throws_( CRuntimeException );
	bool InSection( void ) const { ASSERT_PTR( m_pCtx.get() ); return m_pCtx->m_pSectionContent != nullptr; }
	void EnterSection( const std::tstring& sectionName ) throws_( CRuntimeException );
	void ExitCurrentSection( void ) throws_( CRuntimeException );
	void AddTextContent( TConstIterator itFirst, TConstIterator itLast, bool fullLine );
	void AddTextContentLine( const std::tstring& line ) { AddTextContent( line.begin(), line.end(), true ); }

	size_t ExpandLiterals( OUT std::tstring* pSectionContent );
	void UpdateFilePathLiterals( void );

	std::tstring* ExpandSectionRefs( const std::tstring& sectionName ) throws_( CRuntimeException );
	bool PromptConditionalSectionRef( IN OUT std::tstring* pRefSectionName ) const;

	static bool CheckEatLineEnd( IN OUT size_t* pLastPos, const TCHAR* pContent );		// if '$' suffix => skip the following "\r\n" (or just "\n")


	struct CParsingContext
	{
		CParsingContext( void ) : m_lineNo( 1 ), m_pSectionContent( nullptr ) {}
	public:
		size_t m_lineNo;
		std::tstring m_sectionName;				// current section being parsed
		std::tstring* m_pSectionContent;		// content of the current section
	};
private:
	typedef str::CEnclosedParser<TCHAR> TParser;
	typedef std::pair<std::tstring, std::tstring> TLiteralPair;		// <name, value>
	typedef std::map<std::tstring, std::tstring> TSectionMap;

	std::tstring m_outLineEnd;				// for output formatting (may be different for testing)
	const TParser m_sectionParser;			// for "[[section]]" tags
	const TParser m_crossRefParser;			// for "<<section>>" tags (section cross-references)
	std::vector<TLiteralPair> m_literals;	// e.g. "%FileName%" -> "StripBar"
	std::auto_ptr<CParsingContext> m_pCtx;

	fs::CPath m_filePath;
	TSectionMap m_sectionMap;				// sections: name -> content
private:
	static const std::tstring s_endOfSection;				// "EOS" => "[[EOS]]" tags in the compound text file
	static const TCHAR s_conditionalPrompt = _T('?');		// e.g. "<<?Section>>"
	static const TCHAR s_eatLineEnd = _T('$');				// e.g. "<<Section>>$\r\n" => eat next "\r\n"

	// predefined literals
	static const std::tstring s_year;		// "%YEAR%"  -> "2023"
	static const std::tstring s_month;		// "%MONTH%" -> "Feb-2023"
	static const std::tstring s_date;		// "%DATE%"  -> "27-Feb-2023"
};


#endif // CodeSnippetsParser_h
