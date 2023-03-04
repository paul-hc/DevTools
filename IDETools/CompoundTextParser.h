#ifndef CompoundTextParser_h
#define CompoundTextParser_h
#pragma once

#include "utl/RuntimeException.h"
#include "utl/StringParsing.h"
#include "CodeUtils.h"
#include <map>


class CCompoundTextParser
{
public:
	CCompoundTextParser( const TCHAR* pOutLineEnd = code::g_pLineEnd );
	~CCompoundTextParser();

	void StoreFieldMappings( const std::map<std::tstring, std::tstring>& fieldMappings );

	void ParseFile( const fs::CPath& filePath ) throws_( CRuntimeException );
	void ParseStream( std::istream& is ) throws_( CRuntimeException );

	std::tstring ExpandSection( const std::tstring& sectionName ) throws_( CRuntimeException );
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

	std::tstring* FindFieldValue( const std::tstring& fieldKey );			// e.g. "%TypeName%"
	std::tstring& LookupFieldValue( const std::tstring& fieldKey );			// e.g. "%TypeName%"
	size_t ExpandFieldMappings( std::tstring* pSectionContent _out_ );
	void UpdateFilePathFields( void );

	std::tstring* ExpandSectionRefs( const std::tstring& sectionName ) throws_( CRuntimeException );
	bool PromptConditionalSectionRef( std::tstring* pRefSectionName _in_out_ ) const;

	static bool CheckEatLineEnd( size_t* pLastPos _in_out_, const TCHAR* pContent );		// if '$' suffix => skip the following "\r\n" (or just "\n")


	struct CParsingContext
	{
		CParsingContext( const fs::CPath& filePath ) : m_filePath( filePath ), m_lineNo( 1 ), m_pSectionContent( nullptr ) {}
	public:
		const fs::CPath& m_filePath;
		size_t m_lineNo;
		std::tstring m_sectionName;				// current section being parsed
		std::tstring* m_pSectionContent;		// content of the current section
	};
private:
	typedef str::CEnclosedParser<TCHAR> TParser;
	typedef std::map<std::tstring, std::tstring> TSectionMap;

	std::tstring m_outLineEnd;				// for output formatting (may be different for testing)
	const TParser m_sectionParser;			// for "[[section]]" tags
	const TParser m_crossRefParser;			// for "<<section>>" tags (section cross-references)
	std::vector< std::pair<std::tstring, std::tstring> > m_fieldMappings;	// e.g. "%FileName%" -> "StripBar"
	std::auto_ptr<CParsingContext> m_pCtx;

	TSectionMap m_sectionMap;				// sections: name -> content

	static const std::tstring s_endOfSection;				// "EOS" => "[[EOS]]" tags in the compound text file
	static const TCHAR s_conditionalPrompt = _T('?');		// e.g. "<<?Section>>"
	static const TCHAR s_eatLineEnd = _T('$');				// e.g. "<<Section>>$\r\n" => eat next "\r\n"

	// predefined fields
	static const std::tstring s_year;		// "%YEAR%"  -> "2023"
	static const std::tstring s_month;		// "%MONTH%" -> "Feb-2023"
	static const std::tstring s_date;		// "%DATE%"  -> "27-Feb-2023"
};


#endif // CompoundTextParser_h
