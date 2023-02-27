
#include "pch.h"
#include "CompoundTextParser.h"
#include "CodeUtilities.h"
#include "Algorithms.h"
#include "CodeMessageBox.h"
#include "utl/Path.h"
#include "utl/RuntimeException.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCompoundTextParser implementation

/* Valid section constructs:
[[Copyright]]
[[EOS]]

[[Copyright]][[EOS]]

[[Copyright]]content[[EOS]]

[[Copyright]]
content[[EOS]]

[[Copyright]]line1
...
lineN[[EOS]]

[[Copyright]]
line1
...
lineN
[[EOS]]
*/

const std::tstring CCompoundTextParser::s_endOfSection = _T("EOS");

CCompoundTextParser::CCompoundTextParser( void )
	: m_sectionParser( _T("[[|;;"), _T("]]|\n"), false )
	, m_crossRefParser( _T("<<"), _T(">>"), false )
{
	m_sectionParser.GetSeparators().EnableLineComments();
}

CCompoundTextParser::~CCompoundTextParser()
{
}

void CCompoundTextParser::StoreFieldMappings( const std::map<std::tstring, std::tstring>& fieldMappings )
{
	m_fieldMappings.assign( fieldMappings.begin(), fieldMappings.end() );
}

void CCompoundTextParser::ParseFile( const fs::CPath& filePath ) throws_( std::exception )
{
	m_sectionMap.clear();
	m_pCtx.reset( new CParsingContext( filePath ) );

	std::ifstream input( filePath.GetPtr(), std::ios_base::in );	// | std::ios_base::binary
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to open text file %s"), filePath.GetPtr() ) );

	for ( std::string line; std::getline( input, line ); ++m_pCtx->m_lineNo )
		ParseLine( str::FromUtf8( line.c_str() ) );
}

void CCompoundTextParser::ParseLine( const std::tstring& line )
{
	TParser::TSepMatchPos sepMatchPos;
	TParser::TSpecPair specBounds = m_sectionParser.FindItemSpec( &sepMatchPos, line );

//	for ( TParser::TSpecPair specBounds( 0, 0 );
//		  ( specBounds = m_sectionParser.FindItemSpec( &sepMatchPos, line, specBounds.first ) ).first != utl::npos;		// a known spec?
//		  specBounds.first = specBounds.second; )

	if ( specBounds.first != utl::npos )			// a known spec?
	{
		Range<size_t> itemRange = m_sectionParser.GetItemRange( sepMatchPos, specBounds );
		std::tstring lineSuffix = line.substr( specBounds.second );

		if ( !InSection() )
		{
			if ( SectionSep == sepMatchPos )		// "[[tag]]"?
				EnterSection( m_sectionParser.MakeItem( itemRange, line ), lineSuffix );
		}
		else
		{
			if ( SectionSep == sepMatchPos )		// "[[tag]]"?
			{
				if ( str::EqualsSeqAt( line, itemRange.m_start, s_endOfSection ) )	// EOS or new_section tag?
				{
					AddTextContent( line.substr( 0, specBounds.first ) );			// append the leading text before the tag
					m_pCtx->m_pSectionContent = nullptr;							// exit section content parsing mode
				}
				else
					EnterSection( m_sectionParser.MakeItem( itemRange, line ), lineSuffix );
			}
			else
				ASSERT( LineCommentSep == sepMatchPos );		// just ignore comment
		}
	}
	else if ( InSection() )
		AddTextContent( line );		// append the entire line
	else
	{	// ignore stray line
	}
}

void CCompoundTextParser::EnterSection( const std::tstring& sectionName, const std::tstring& lineSuffix ) throws_( CRuntimeException )
{
	m_pCtx->m_pSectionContent = &m_sectionMap[ sectionName ];		// enter section content parsing mode

	if ( !m_pCtx->m_pSectionContent->empty() )						// not a new entry? => section collision
		throw CRuntimeException( str::Format( _T("Syntax error: duplicate section '%s' found in compound text file '%s'"),
											  sectionName.c_str(), m_pCtx->m_filePath.GetPtr() ) );

	if ( !lineSuffix.empty() )
		ParseLine( lineSuffix );		// allow inline text after the "[[section]]" tag
}

void CCompoundTextParser::AddTextContent( const std::tstring& text )
{
	ASSERT( InSection() );

	if ( !m_pCtx->m_pSectionContent->empty() )
		*m_pCtx->m_pSectionContent += code::lineEnd;

	*m_pCtx->m_pSectionContent += text;
}


// expand embedded sections:

std::tstring CCompoundTextParser::ExpandSectionContent( const std::tstring& sectionName ) throws_( CRuntimeException )
{
	if ( std::tstring* pTextContent = ExpandSectionRefs( sectionName ) )
	{
		ExpandFieldMappings( pTextContent );
		return *pTextContent;
	}

	return str::GetEmpty();
}

size_t CCompoundTextParser::ExpandFieldMappings( std::tstring* pSectionContent _out_ )
{
	ASSERT_PTR( pSectionContent );

	size_t count = 0;

	for ( std::vector< std::pair<std::tstring, std::tstring> >::const_iterator itField = m_fieldMappings.begin(); itField != m_fieldMappings.end(); ++itField )
		count += str::Replace( *pSectionContent, itField->first.c_str(), itField->second.c_str() );

	return count;
}

std::tstring* CCompoundTextParser::ExpandSectionRefs( const std::tstring& sectionName ) throws_( CRuntimeException )
{	// expand embedded sections such as "<<Section X>>", returns the expanded section content
	std::tstring* pTextContent = utl::FindValuePtr( m_sectionMap, sectionName );

	if ( nullptr == pTextContent )
		return nullptr;

	TParser::TSepMatchPos sepMatchPos;

	for ( TParser::TSpecPair specBounds( 0, 0 );
		  ( specBounds = m_crossRefParser.FindItemSpec( &sepMatchPos, *pTextContent, specBounds.second ) ).first != std::tstring::npos;
		  specBounds.first = specBounds.second )
	{	// found an embedded "<<Section>>" spec
		std::tstring refSectionName = m_crossRefParser.ExtractItem( sepMatchPos, specBounds, *pTextContent );

		const std::tstring* pRefContent = PromptConditionalSectionRef( &refSectionName )		// it strips the '?' prefix if present (conditional sections)
			? ExpandSectionRefs( refSectionName )
			: &str::GetEmpty();

		if ( nullptr == pRefContent )
			throw CRuntimeException( str::Format( _T("Cross-referenced section '%s' in section '%s' not found in compound text file '%s'"),
												  refSectionName.c_str(), sectionName.c_str(), m_pCtx->m_filePath.GetPtr() ) );

		CheckEatLineEnd( &specBounds.second, pTextContent->c_str() );		// do we have a '$' suffix, such as "<<Section>>$"?

		// replace the entire spec "<<RefSection>>" with the embedded section content
		pTextContent->replace( specBounds.first, specBounds.second - specBounds.first, *pRefContent );

		specBounds.second = specBounds.first + pRefContent->length();		// adjust span to the new expanded content
	}
	return pTextContent;
}

bool CCompoundTextParser::CheckEatLineEnd( size_t* pLastPos _in_out_, const TCHAR* pContent )
{	// if there is an '$' suffix (such as "<<Section>>$") => eat the following "\r\n" (or just "\n")
	ASSERT_PTR( pContent );
	ASSERT( *pLastPos <= str::GetLength( pContent ) );

	if ( pContent[ *pLastPos ] != s_eatLineEnd )
		return false;		// no '$' suffix

	++*pLastPos;			// skip the '$' suffix, such as "<<Section>>$"?

	// skip ONE following line-end:
	if ( '\r' == pContent[ *pLastPos ] )
		++*pLastPos;

	if ( '\n' == pContent[ *pLastPos ] )
		++*pLastPos;

	return true;
}

bool CCompoundTextParser::PromptConditionalSectionRef( std::tstring* pRefSectionName _in_out_ ) const
{
	ASSERT_PTR( pRefSectionName );

	if ( s_conditionalPrompt == str::CharAt( *pRefSectionName, 0 ) )
	{
		pRefSectionName->erase( pRefSectionName->begin() );		// strip the '?'

		std::tstring message = str::Format( _T("Do you want to include section '%s'?"), pRefSectionName->c_str() );
		std::tstring refContent = utl::FindValue( m_sectionMap, *pRefSectionName );

		ide::CScopedWindow scopedIDE;
		CCodeMessageBox dlg( message, refContent, MB_OKCANCEL | MB_ICONQUESTION, scopedIDE.GetMainWnd() );

		if ( dlg.DoModal() != IDOK )
			return false;
	}

	return true;
}


// CSectionParser implementation

CSectionParser::CSectionParser( const TCHAR* pOpenDelim /*= _T("[[")*/, const TCHAR* pCloseDelim /*= _T("]]")*/, const TCHAR* pTagEOS /*= _T("EOS")*/ )
	: m_parser( pOpenDelim, pCloseDelim, false )
	, m_tagEOS( pTagEOS )
{
	Reset();
}

CSectionParser::~CSectionParser()
{
}

void CSectionParser::Reset( void )
{
	m_sectionStage = Before;
	m_sectionCount = 0;
	m_textContent.clear();
}

bool CSectionParser::LoadFileSection( const fs::CPath& compoundFilePath, const std::tstring& sectionName ) throws_( std::exception )
{
	Reset();

	std::ifstream input( compoundFilePath.GetPtr(), std::ios_base::in );	// | std::ios_base::binary
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to open text file %s"), compoundFilePath.GetPtr() ) );

	for ( std::string line; std::getline( input, line ); )
		if ( !ParseLine( str::FromUtf8( line.c_str() ), sectionName ) )
			break;

	return m_sectionCount != 0;
}

/*
Process the line by switching to InSection stage if desired section is detected,
and appending the line to content if zone is InSection.

Note:
- section content starts on the next line;
- EOS tag (end of section) may be on the same line with a section content line.
- if EOS tag is not present, a section ends on the next section start tag.
*/
bool CSectionParser::ParseLine( const std::tstring& line, const std::tstring& sectionName )
{
	ASSERT( m_sectionStage != Done );

	TParser::TSepMatchPos sepMatchPos;
	TParser::TSpecPair specBounds = m_parser.FindItemSpec( &sepMatchPos, line );

	if ( specBounds.first != utl::npos )
	{
		Range<size_t> tagRange = m_parser.GetItemRange( sepMatchPos, specBounds );

		if ( Before == m_sectionStage )
		{
			if ( sectionName.empty() || str::EqualsSeqAt( line, tagRange.m_start, sectionName ) )
			{
				++m_sectionCount;
				m_sectionStage = InSection;		// enter text content parsing mode
			}
		}
		else
		{
			ASSERT( InSection == m_sectionStage );

			if ( !tagRange.IsEmpty() )			// new-section tag or EOS?
			{
				if ( !sectionName.empty() )
					m_sectionStage = Done;
				else
					m_sectionStage = Before;	// no section filter: keep loading all sections

				if ( str::EqualsSeqAt( line, tagRange.m_start, m_tagEOS ) )
					AddTextContent( line.substr( 0, specBounds.first ) );		// append the leading text before the tag
			}
		}
	}
	else
		AddTextContent( line );		// append the entire line

	return m_sectionStage != Done;
}

void CSectionParser::AddTextContent( const std::tstring& text )
{
	if ( !m_textContent.empty() )
		m_textContent += code::lineEnd;

	m_textContent += text;
}
