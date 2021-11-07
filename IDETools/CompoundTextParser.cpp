
#include "stdafx.h"
#include "CompoundTextParser.h"
#include "IdeUtilities.h"
#include "StringUtilitiesEx.h"
#include "CodeMessageBox.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// SectionParser implementation

SectionParser::SectionParser( LPCTSTR _sectionName, LPCTSTR _tagOpen /*= _T("[[")*/,
							  LPCTSTR _tagClose /*= _T("]]")*/, LPCTSTR _tagCoreEOS /*= _T("EOS")*/ )
	: sectionName( _sectionName )
	, tagOpen( _tagOpen )
	, tagClose( _tagClose )
	, tagCoreEOS( _tagCoreEOS )
	, tagOpenLen( str::GetLength( tagOpen ) )
	, tagCloseLen( str::GetLength( tagClose ) )

	, zone( Before )
	, textContent()
{
}

SectionParser::~SectionParser()
{
}

void SectionParser::reset( void )
{
	zone = Before;
	textContent.Empty();
}

// Process the line by switching to Inside zone if desired section is detected,
// and appending the line to content if zone is Inside.
// Returns false if After state is reached, otherwise true.
// NOTE: section content starts on the line next to section begin tag, and
// EOS tag (end of section) may be on the same line with a section content line.
// If EOS tag is not present, a section ends on the next section start tag.
bool SectionParser::processLine( LPCTSTR line )
{
	if ( !isAfter() )
	{	// Parsing not finished, process the line:
		int				tagStartPos = -1, tagLength = 0;
		CString			tagCore = extractTag( line, tagStartPos, tagLength );

		if ( isBefore() )
		{
			if ( !tagCore.IsEmpty() && tagCore.CompareNoCase( sectionName ) == 0 )
				zone = Inside;
		}
		else
		{
			ASSERT( isInside() );
			if ( tagCore.CompareNoCase( tagCoreEOS ) == 0 || !tagCore.IsEmpty() )
				zone = After;
			if ( zone == After )
				textContent += CString( line, tagStartPos );	// Append the line until the tag start.
			else
				textContent += line;							// Append the entire line.
		}
	}

	return !isAfter();
}

// Parse the line, search for section names, and if found it appends it as a line to
// the current content and returns True, otherwise does nothing and returns false.
// NOTE: EOS sections are not added!
bool SectionParser::extractSection( LPCTSTR line, LPCTSTR sectionFilter /*= NULL*/,
									str::CaseType caseType /*= str::IgnoreCase*/, LPCTSTR sep /*= _T("\r\n")*/ )
{
	int tagStartPos = -1, tagLength = 0;
	CString sectionCore = extractTag( line, tagStartPos, tagLength );

	if ( sectionCore.IsEmpty() || sectionCore.CompareNoCase( tagCoreEOS ) == 0 )
		return false;

	// if section name filter is specified -> check for filter match:
	if ( sectionFilter != NULL && *sectionFilter != 0 )
		if ( !str::findStringPos( sectionCore, sectionFilter, caseType ).IsValid() )
			return false;

	if ( !textContent.IsEmpty() )
		textContent += ( CString( sep ) + sectionCore );
	return true;
}

// extract and localize the tag into string (if any), and returns the tag core string + start and end pos.

CString SectionParser::extractTag( LPCTSTR line, int& rTagStartPos, int& rTagLength ) const
{
	rTagStartPos = str::findStringPos( line, tagOpen ).m_start;
	if ( rTagStartPos != -1 )
	{
		int endPos = str::findStringPos( line, tagClose, rTagStartPos + tagOpenLen ).m_start;
		if ( endPos != -1 )
		{
			rTagLength = endPos + tagCloseLen - rTagStartPos;
			return CString( line + rTagStartPos + tagOpenLen, rTagLength - tagOpenLen - tagCloseLen );
		}
	}
	return CString();
}


// CompoundTextParser implementation

const TCHAR CompoundTextParser::chConditionalPrompt = _T('?');
const TCHAR CompoundTextParser::chNoEndOfLineIfEmpty = _T('$');
const TCHAR CompoundTextParser::whitespaces[] = _T(" \t\r\n");
const TCHAR CompoundTextParser::singleLineComment[] = _T(";");
const TCHAR CompoundTextParser::endOfSection[] = _T("EOS");
const TCHAR CompoundTextParser::embeddedInsertPoint[] = _T("%CORE%");
const CompoundTextParser::TokenDescr CompoundTextParser::tokenSection = CompoundTextParser::TokenDescr( _T("[["), _T("]]") );
const CompoundTextParser::TokenDescr CompoundTextParser::tokenSectionRef = CompoundTextParser::TokenDescr( _T("<<"), _T(">>") );

CompoundTextParser::CompoundTextParser( const TCHAR* _textFilePath,
										const std::map< CString, CString >& _fieldReplacements )
	: textFilePath( _textFilePath )
	, fieldReplacements( _fieldReplacements )
	, textFile()
	, line()
	, m_lineNo( 0 )
	, tokenCore()
	, mapSections()
{
}

CompoundTextParser::~CompoundTextParser()
{
}

bool CompoundTextParser::parseFile( void )
{
	mapSections.clear();
	m_lineNo = 0;
	if ( !textFile.Open( textFilePath, CFile::modeRead | CFile::typeText ) )
		return false;

	CString			errMessage;
	CString			currSection, currContent;

	try
	{
		for ( ;; )
			switch ( getNextLine() )
			{
				case tok_Comment:
					break;
				case tok_Section:
					if ( !currSection.IsEmpty() || tokenCore.IsEmpty() )
						throw SyntaxError;
					currSection = tokenCore;
					break;
				case tok_EndOfSection:
					if ( currSection.IsEmpty() )
						throw SyntaxError;
					// Special care for sections with inline content: add the prefix of the line containing the EndOfSection tag
					currContent += line;
					mapSections[ currSection ] = currContent;
					currSection.Empty();
					currContent.Empty();
					break;
				case tok_ContentLine:
					if ( !currSection.IsEmpty() )
						currContent += line + _T("\r\n");
					break;
				default:
					throw SyntaxError;
			}
	}
	catch ( Exception exc )
	{
		if ( exc == SyntaxError )
		{
			TRACE( _T("CompoundTextParser::parseFile(): syntax error at line: %d \"%s\"\n"), m_lineNo, (LPCTSTR)line );
			return false;
		}
	}

	if ( !currSection.IsEmpty() )
	{
		AfxFormatString1( errMessage, IDS_ERR_SECTIONNOTCLOSED, (LPCTSTR)currSection );
		AfxMessageBox( errMessage );
		return false;
	}
	return true;
}

void CompoundTextParser::makeFieldReplacements( void )
{
	for ( TMapSectionToContent::iterator sectionIt = mapSections.begin(); sectionIt != mapSections.end(); ++sectionIt )
		for ( std::map< CString, CString >::const_iterator fieldIt = fieldReplacements.begin();
			  fieldIt != fieldReplacements.end(); ++fieldIt )
			( *sectionIt ).second.Replace( ( *fieldIt ).first, ( *fieldIt ).second );
}

CString CompoundTextParser::getSectionContent( const CString& sectionName )
{
	CString textContent;
	TMapSectionToContent::const_iterator itSection = mapSections.find( sectionName );

	if ( itSection != mapSections.end() )
	{
		Section section( *this, itSection );

		try
		{
			section.bindAllReferences();
			textContent = section.extractContent();
		}
		catch ( const CString& exc )
		{
			AfxMessageBox( exc );
		}
	}
	return textContent;
}

CompoundTextParser::TokenSemantic CompoundTextParser::getNextLine( void ) throws_( Exception )
{
	line.Empty();
	tokenCore.Empty();
	++m_lineNo;
	try
	{	// read a new line string from the file
		if ( textFile.ReadString( line ) )
		{
			static std::pair<int, int> tokenSectionLen( str::GetLength( tokenSection.first ), str::GetLength( tokenSection.second ) );
			int tokenStart = 0, tokenEnd = 0;

			if ( !line.IsEmpty() )
				if ( _tcsncmp( line, singleLineComment, _tcslen( singleLineComment ) ) == 0 )
					return tok_Comment;
				else if ( ( tokenStart = line.Find( tokenSection.first, tokenEnd ) ) != -1 )
					if ( ( tokenEnd = line.Find( tokenSection.second, tokenStart += tokenSectionLen.first ) ) != -1 )
					{
						tokenCore = line.Mid( tokenStart, tokenEnd - tokenStart );
						// truncate the line at section begin position in order to separate inline content from the tag
						line.SetAt( tokenStart - tokenSectionLen.first, _T('\0') );
						if ( tokenCore == endOfSection )
							return tok_EndOfSection;
						else
							return tok_Section;
					}
					else
						throw SyntaxError;
			return tok_ContentLine;
		}
	}
	catch ( CException* exc )
	{
		exc->Delete();
	}
	throw EndOfFile;
}

bool CompoundTextParser::isCharOneOf( TCHAR chr, const TCHAR* stringSet )
{
	if ( chr == _T('\0') )
		return false;

	const TCHAR*	found = _tcschr( stringSet, chr );

	return found != NULL && *found != _T('\0');
}


// CompoundTextParser::Section implementation

CompoundTextParser::Section::Section( CompoundTextParser& _parser, CompoundTextParser::TMapSectionToContent::const_iterator itSection )
	: parser( _parser )
	, name( ( *itSection ).first )
	, content( ( *itSection ).second )
{
}

CompoundTextParser::Section::~Section()
{
}

CString CompoundTextParser::Section::extractContent( const TCHAR* insertorReplacer /*= NULL*/ ) const
{
	bool isEmptyReplacer = insertorReplacer == NULL || *insertorReplacer == _T('\0');
	CString outcomeContent = content;
	int insertorStart = outcomeContent.Find( embeddedInsertPoint );

	if ( insertorStart != -1 )
	{
		// Replace embedded insertion point with referredContent
		const TCHAR* contentPtr = outcomeContent;
		int insertorEnd = insertorStart + static_cast<int>( str::GetLength( embeddedInsertPoint ) );

		// If post token tag line remover exists, extend the token after the next "\r\n"
		if ( contentPtr[ insertorEnd ] == chNoEndOfLineIfEmpty )
		{
			++insertorEnd;
			// Cut trailing "\r\n" if the replacer is empty
			if ( isEmptyReplacer )
				while ( contentPtr[ insertorEnd ] == _T('\r') || contentPtr[ insertorEnd ] == _T('\n') )
					++insertorEnd;
		}
		outcomeContent.Delete( insertorStart, insertorEnd - insertorStart );
		if ( !isEmptyReplacer )
			outcomeContent.Insert( insertorStart, insertorReplacer );
	}

	return outcomeContent;
}

int CompoundTextParser::Section::bindAllReferences( void ) throws_( CString )
{
	static std::pair<int, int> tokenPairLen( str::GetLength( CompoundTextParser::tokenSectionRef.first ),
											 str::GetLength( CompoundTextParser::tokenSectionRef.second ) );
	int tokenStart = 0, tokenEnd = 0, boundCount = 0;
	CString errMessage;

	do
		if ( ( tokenStart = content.Find( CompoundTextParser::tokenSectionRef.first, tokenStart ) ) != -1 )
			if ( ( tokenEnd = content.Find( CompoundTextParser::tokenSectionRef.second, tokenStart + tokenPairLen.first ) ) != -1 )
			{
				tokenEnd += tokenPairLen.second;

				CString refSectionName = content.Mid( tokenStart + tokenPairLen.first,
													  tokenEnd - tokenStart - tokenPairLen.first - tokenPairLen.second );
				CString referredContent;

				if ( !refSectionName.IsEmpty() )
				{	// First bind the referred section
					bool isConditional = refSectionName[ 0 ] == CompoundTextParser::chConditionalPrompt;

					if ( isConditional )
						refSectionName = refSectionName.Mid( 1 );

					TMapSectionToContent::const_iterator itRefSection = parser.mapSections.find( refSectionName );

					if ( itRefSection != parser.mapSections.end() )
					{	// Bind the referred section as well (if not already)
						Section refSection( parser, itRefSection );

						refSection.bindAllReferences();
						referredContent = refSection.content;
						if ( isConditional )
						{	// Prompt token found -> query for conditional reference inclusion
							CString promptMessage;

							promptMessage.Format( IDS_PROMPTINCLUDESECTION, (LPCTSTR)refSectionName );

							ide::CScopedWindow scopedIDE;
							if ( CCodeMessageBox( promptMessage, referredContent, MB_OKCANCEL | MB_ICONQUESTION,
												  MAKEINTRESOURCE( IDS_CODE_GENERATOR_CAPTION ), scopedIDE.GetMainWnd() ).DoModal() != IDOK )
								referredContent.Empty();
						}
					}
					else
					{
						errMessage.Format( _T("Invalid reference in section \"%s\": section named \"%s\" couldn't be found !"),
										   (LPCTSTR)name, (LPCTSTR)refSectionName );
						throw errMessage;
					}
				}

				// if trailing chNoEndOfLineIfEmpty $ exist, extend the section reference tag to include the trailing "\r\n"
				// In this case:
				//	<<Section1>><<Section2>>[[EOS]]
				// is equivalent with:
				//	<<Section1>>$
				//	<<Section2>>$
				//	[[EOS]]
				//
				LPCTSTR contentPtr = content;

				if ( contentPtr[ tokenEnd ] == chNoEndOfLineIfEmpty )
				{	// Include trailing CR+LF in the section reference tag
					do
						++tokenEnd;
					while ( contentPtr[ tokenEnd ] == _T('\r') || contentPtr[ tokenEnd ] == _T('\n') );
				}

				// delete section reference tag
				content.Delete( tokenStart, tokenEnd - tokenStart );

				int insertorStart = content.Find( embeddedInsertPoint );

				if ( insertorStart != -1 )
				{
					// replace embedded insertion point with referredContent
					int insertorEnd = insertorStart + str::GetLength( embeddedInsertPoint );

					contentPtr = content;
					// If post token tag line remover exists, extend the token after the next "\r\n"
					if ( contentPtr[ insertorEnd ] == chNoEndOfLineIfEmpty )
					{
						++insertorEnd;
						// Cut trailing "\r\n" if referred content is empty
						if ( referredContent.IsEmpty() )
							while ( contentPtr[ insertorEnd ] == _T('\r') || contentPtr[ insertorEnd ] == _T('\n') )
								++insertorEnd;
					}
					content.Delete( insertorStart, insertorEnd - insertorStart );
					content.Insert( insertorStart, referredContent );
				}
				else
					// Append the content: replace the tag (section reference) with it's content
					content.Insert( tokenStart, referredContent );
				++boundCount;
			}
			else
			{
				errMessage.Format( _T("Syntax error in section \"%s\": section tag not closed !"), (LPCTSTR)name );
				throw errMessage;
			}
	while ( tokenStart != -1 );

	// if any substitution done, update parser's section map
	if ( boundCount > 0 )
		parser.mapSections[ name ] = content;

	return boundCount;
}
