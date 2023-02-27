#ifndef CompoundTextParser_h
#define CompoundTextParser_h
#pragma once

#include "utl/StringParsing.h"
#include <map>


class CCompoundTextParser
{
public:
	CCompoundTextParser( void );
	~CCompoundTextParser();

	void StoreFieldMappings( const std::map<std::tstring, std::tstring>& fieldMappings );

	void ParseFile( const fs::CPath& filePath ) throws_( std::exception );

	std::tstring ExpandSectionContent( const std::tstring& sectionName ) throws_( CRuntimeException );
private:
	enum SepMatch
	{
		SectionSep,				// "[[...]]" TSepMatchPos of 0 in m_sectionParser
		LineCommentSep			// ";;..." TSepMatchPos of 1 in m_sectionParser
	};

	struct CParsingContext
	{
		CParsingContext( const fs::CPath& filePath ) : m_filePath( filePath ), m_lineNo( 1 ), m_pSectionContent( nullptr ) {}
	public:
		const fs::CPath& m_filePath;
		size_t m_lineNo;
		std::tstring* m_pSectionContent;		// content of the current section being parsed
	};

	void ParseLine( const std::tstring& line ) throws_( CRuntimeException );
	bool InSection( void ) const { ASSERT_PTR( m_pCtx.get() ); return m_pCtx->m_pSectionContent != nullptr; }
	void EnterSection( const std::tstring& sectionName, const std::tstring& lineSuffix ) throws_( CRuntimeException );
	void AddTextContent( const std::tstring& text );

	size_t ExpandFieldMappings( std::tstring* pSectionContent _out_ );
	std::tstring* ExpandSectionRefs( const std::tstring& sectionName ) throws_( CRuntimeException );
	bool PromptConditionalSectionRef( std::tstring* pRefSectionName _in_out_ ) const;

	static bool CheckEatLineEnd( size_t* pLastPos _in_out_, const TCHAR* pContent );		// if '$' suffix => skip the following "\r\n" (or just "\n")
private:
	typedef str::CEnclosedParser<TCHAR> TParser;
	typedef std::map<std::tstring, std::tstring> TSectionMap;

	const TParser m_sectionParser;			// for "[[section]]" tags
	const TParser m_crossRefParser;			// for "<<section>>" tags (section cross-references)
	std::vector< std::pair<std::tstring, std::tstring> > m_fieldMappings;	// e.g. "%FileName%" -> "StripBar"
	std::auto_ptr<CParsingContext> m_pCtx;

	TSectionMap m_sectionMap;				// sections: name -> content

	static const std::tstring s_endOfSection;				// "EOS" => "[[EOS]]" tags in the compound text file
	static const TCHAR s_conditionalPrompt = _T('?');		// e.g. "<<?Section>>"
	static const TCHAR s_eatLineEnd = _T('$');				// e.g. "<<Section>>$\r\n" => eat next "\r\n"
};


class CSectionParser		// rough cut with little functionality (no embedded sections) => better use CCompoundTextParser!
{
public:
	CSectionParser( const TCHAR* pOpenDelim = _T("[["), const TCHAR* pCloseDelim = _T("]]"), const TCHAR* pTagEOS = _T("EOS") );
	virtual ~CSectionParser();

	bool LoadFileSection( const fs::CPath& compoundFilePath, const std::tstring& sectionName ) throws_( std::exception );

	const std::tstring& GetTextContent( void ) const { return m_textContent; }
	size_t GetSectionCount( void ) const { return m_sectionCount; }
private:
	void Reset( void );
	bool ParseLine( const std::tstring& line, const std::tstring& sectionName );
	void AddTextContent( const std::tstring& text );
private:
	enum SectionStage { Before, InSection, Done };
	typedef str::CEnclosedParser<TCHAR> TParser;

	const TParser m_parser;
	std::tstring m_tagEOS;			// "EOS": end of section

	SectionStage m_sectionStage;	// the current zone of parsing relative to the desired section.
	size_t m_sectionCount;

	std::tstring m_textContent;		// resulting text content of the section
};


#endif // CompoundTextParser_h
