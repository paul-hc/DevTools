#ifndef CompoundTextParser_h
#define CompoundTextParser_h
#pragma once

#include <map>


class SectionParser
{
public:
	SectionParser( LPCTSTR _sectionName, LPCTSTR _tagOpen = _T("[["), LPCTSTR _tagClose = _T("]]"),
				   LPCTSTR _tagCoreEOS = _T("EOS") );
	virtual ~SectionParser();

	bool isBefore( void ) const { return zone == Before; }
	bool isInside( void ) const { return zone == Inside; }
	bool isAfter( void ) const { return zone == After; }

	void reset( void );
	bool processLine( LPCTSTR line );
	bool extractSection( LPCTSTR line, LPCTSTR sectionFilter = NULL, str::CaseType caseType = str::IgnoreCase, LPCTSTR sep = _T("\r\n") );
protected:
	CString extractTag( LPCTSTR line, int& rTagStartPos, int& rLength ) const;
public:
	enum SectionZone { Before, Inside, After };
protected:
	LPCTSTR sectionName;		// The name of the target section.
	LPCTSTR tagOpen, tagClose, tagCoreEOS;
	int tagOpenLen, tagCloseLen;

	SectionZone zone;			// The current zone of parsing relative to the desired section.
public:
	CString textContent;		// The resulting text content of the section.
};


class CompoundTextParser
{
public:
	enum TokenSemantic
	{
		tok_Comment,
		tok_Section,
		tok_EndOfSection,
		tok_ContentLine,
		tok_SectionReference
	};

	enum Exception { EndOfFile, SyntaxError };

	typedef std::map< CString, CString > TMapSectionToContent;
	typedef std::pair<const TCHAR*, const TCHAR*> TokenDescr;
public:
	CompoundTextParser( const TCHAR* _textFilePath,
						const std::map< CString, CString >& _fieldReplacements );
	~CompoundTextParser();

	bool parseFile( void );
	void makeFieldReplacements( void );
	CString getSectionContent( const CString& sectionName );
protected:
	TokenSemantic getNextLine( void ) throws_( Exception );

	static bool isCharOneOf( TCHAR chr, const TCHAR* stringSet );
public:
	CString textFilePath;
	const std::map< CString, CString >& fieldReplacements;
protected:
	CStdioFile textFile;
	CString line;
	int m_lineNo;
	CString tokenCore;
	TMapSectionToContent mapSections;
public:
	class Section
	{
	public:
		Section( CompoundTextParser& _parser, TMapSectionToContent::const_iterator itSection );
		~Section();

		CString extractContent( const TCHAR* insertorReplacer = NULL ) const;
		int bindAllReferences( void ) throws_( CString );
	private:
		CompoundTextParser&	parser;
	protected:
		CString name;
		CString content;
	};
	friend class Section;
public:
	static const TCHAR chConditionalPrompt;
	static const TCHAR chNoEndOfLineIfEmpty;
	static const TCHAR whitespaces[];
	static const TCHAR singleLineComment[];
	static const TCHAR endOfSection[];
	static const TCHAR embeddedInsertPoint[];
	static const TokenDescr tokenSection;
	static const TokenDescr tokenSectionRef;
};


#endif // CompoundTextParser_h
