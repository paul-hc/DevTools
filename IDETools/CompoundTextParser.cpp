
#include "pch.h"
#include "CompoundTextParser.h"
#include "CodeUtilities.h"
#include "Algorithms.h"
#include "CodeMessageBox.h"
#include "Application_fwd.h"
#include "ModuleSession.h"
#include "utl/FileSystem.h"
#include "utl/Path.h"
#include "utl/TimeUtils.h"
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
const std::tstring CCompoundTextParser::s_year = _T("%YEAR%");
const std::tstring CCompoundTextParser::s_month = _T("%MONTH%");
const std::tstring CCompoundTextParser::s_date = _T("%DATE%");

CCompoundTextParser::CCompoundTextParser( const TCHAR* pOutLineEnd /*= code::g_pLineEnd*/ )
	: m_outLineEnd( pOutLineEnd )
	, m_sectionParser( _T("[[|;;"), _T("]]|\n"), false )
	, m_crossRefParser( _T("<<"), _T(">>"), false )
{
	m_sectionParser.GetSeparators().EnableLineComments();

	CTime now = CTime::GetCurrentTime();
	m_fieldMappings.push_back( std::make_pair( s_year, time_utl::FormatTimestamp( now, _T("%Y") ) ) );
	m_fieldMappings.push_back( std::make_pair( s_month, time_utl::FormatTimestamp( now, _T("%b-%Y") ) ) );
	m_fieldMappings.push_back( std::make_pair( s_date, time_utl::FormatTimestamp( now, _T("%#d-%b-%Y") ) ) );
}

CCompoundTextParser::~CCompoundTextParser()
{
}

void CCompoundTextParser::StoreFieldMappings( const std::map<std::tstring, std::tstring>& fieldMappings )
{
	m_fieldMappings.insert( m_fieldMappings.end(), fieldMappings.begin(), fieldMappings.end() );

	UpdateFilePathFields();
}

void CCompoundTextParser::ParseFile( const fs::CPath& filePath ) throws_( CRuntimeException )
{
	m_pCtx.reset( new CParsingContext( filePath ) );

	std::ifstream input( filePath.GetPtr(), std::ios_base::in );	// | std::ios_base::binary
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to open text file %s"), filePath.GetPtr() ) );

	ParseStream( input );
}

void CCompoundTextParser::ParseStream( std::istream& is ) throws_( CRuntimeException )
{
	m_sectionMap.clear();

	if ( nullptr == m_pCtx.get() )
		m_pCtx.reset( new CParsingContext( fs::CPath() ) );

	for ( std::string line; std::getline( is, line ); ++m_pCtx->m_lineNo )
		ParseLine( str::FromUtf8( line.c_str() ) );
}

void CCompoundTextParser::ParseLine( const std::tstring& line )
{
	size_t pos = 0, length = line.length();

	do
	{
		TParser::TSepMatchPos sepMatchPos;
		TParser::TSpecPair specBounds = m_sectionParser.FindItemSpec( &sepMatchPos, line, pos );

		if ( specBounds.first != utl::npos )		// a known spec?
		{
			Range<size_t> itemRange = m_sectionParser.GetItemRange( sepMatchPos, specBounds );

			if ( !InSection() )
			{
				if ( SectionSep == sepMatchPos )	// "[[tag]]"?
					EnterSection( m_sectionParser.MakeItem( itemRange, line ) );
			}
			else
			{
				if ( SectionSep == sepMatchPos )	// "[[tag]]"?
				{
					if ( pos != specBounds.first )	// any leading text content?
						AddTextContent( line.begin() + pos, line.begin() + specBounds.first, false );	// keep this: append the leading text before the tag, e.g. "text[[EOS]]"

					if ( str::EqualsSeqAt( line, itemRange.m_start, s_endOfSection ) )			// EOS or new_section tag?
						ExitCurrentSection();
					else
						EnterSection( m_sectionParser.MakeItem( itemRange, line ) );			// will throw "previous section not ended"
				}
				else
					ASSERT( LineCommentSep == sepMatchPos );	// just ignore comment
			}
			pos = specBounds.second;		// continue parsing past tag suffix
		}
		else if ( InSection() )
		{
			if ( 0 == pos )
				AddTextContentLine( line );									// append the entire line
			else
				AddTextContent( line.begin() + pos, line.end(), false );	// append the remaining text

			pos = length;
		}
		else
			break;		// ignore stray line
	}
	while ( pos != length );
}

void CCompoundTextParser::EnterSection( const std::tstring& sectionName ) throws_( CRuntimeException )
{
	if ( !m_pCtx->m_sectionName.empty() )						// not a new entry? => section collision
		throw CRuntimeException( str::Format( _T("Syntax error: previous session '%s' was not ended while starting a new section '%s' in compound text file '%s' at line %d"),
											  m_pCtx->m_sectionName.c_str(), sectionName.c_str(), m_pCtx->m_filePath.GetPtr(), m_pCtx->m_lineNo ) );

	m_pCtx->m_sectionName = sectionName;
	m_pCtx->m_pSectionContent = &m_sectionMap[ sectionName ];		// enter section content parsing mode

	if ( !m_pCtx->m_pSectionContent->empty() )						// not a new entry? => section collision
		throw CRuntimeException( str::Format( _T("Syntax error: duplicate section '%s' found in compound text file '%s' at line %d"),
											  sectionName.c_str(), m_pCtx->m_filePath.GetPtr(), m_pCtx->m_lineNo ) );
}

void CCompoundTextParser::ExitCurrentSection( void ) throws_( CRuntimeException )
{
	if ( m_pCtx->m_sectionName.empty() )		// stray [[EOS]]
		throw CRuntimeException( str::Format( _T("Syntax error: ending a section that was not entered in compound text file '%s' at line %d"),
											  m_pCtx->m_filePath.GetPtr(), m_pCtx->m_lineNo ) );

	m_pCtx->m_sectionName.clear();
	m_pCtx->m_pSectionContent = nullptr;		// exit section content parsing mode
}

void CCompoundTextParser::AddTextContent( TConstIterator itFirst, TConstIterator itLast, bool fullLine )
{
	ASSERT( InSection() );

	m_pCtx->m_pSectionContent->insert( m_pCtx->m_pSectionContent->end(), itFirst, itLast );		// append text content

	if ( fullLine )
		*m_pCtx->m_pSectionContent += m_outLineEnd;
}


// expand embedded sections:

std::tstring CCompoundTextParser::ExpandSection( const std::tstring& sectionName ) throws_( CRuntimeException )
{
	if ( std::tstring* pTextContent = ExpandSectionRefs( sectionName ) )
	{
		ExpandFieldMappings( pTextContent );
		return *pTextContent;
	}

	return str::GetEmpty();
}

std::tstring* CCompoundTextParser::FindFieldValue( const std::tstring& fieldKey )
{
	for ( std::vector< std::pair<std::tstring, std::tstring> >::iterator itField = m_fieldMappings.begin(); itField != m_fieldMappings.end(); ++itField )
		if ( fieldKey == itField->first )
			return &itField->second;

	return NULL;
}

std::tstring& CCompoundTextParser::LookupFieldValue( const std::tstring& fieldKey )
{
	std::tstring* pFieldValue = FindFieldValue( fieldKey );

	if ( nullptr == pFieldValue )
	{
		m_fieldMappings.push_back( std::make_pair( fieldKey, str::GetEmpty() ) );
		pFieldValue = &m_fieldMappings.back().second;
	}

	return *pFieldValue;
}

size_t CCompoundTextParser::ExpandFieldMappings( std::tstring* pSectionContent _out_ )
{
	ASSERT_PTR( pSectionContent );
	UpdateFilePathFields();

	size_t count = 0;

	for ( std::vector< std::pair<std::tstring, std::tstring> >::const_iterator itField = m_fieldMappings.begin(); itField != m_fieldMappings.end(); ++itField )
		count += str::Replace( *pSectionContent, itField->first.c_str(), itField->second.c_str() );

	return count;
}

void CCompoundTextParser::UpdateFilePathFields( void )
{
	// store document path dependent fields
	fs::CPath pchFilePath( _T("stdafx.h") );

	if ( std::tstring* pFilePath = FindFieldValue( _T("%FilePath%") ) )		// was the field set by the VBA macro?
	{
		fs::CPath filePath( *pFilePath );
		std::tstring typeName = code::ExtractFilenameIdentifier( filePath );

		LookupFieldValue( _T("%FileName%") ) = filePath.GetFname();
		LookupFieldValue( _T("%TypeName%") ) = app::GetModuleSession().m_classPrefix + typeName;

		std::vector<fs::TDirPath> searchPaths;
		searchPaths.push_back( filePath.GetParentPath() );				// same directory
		searchPaths.push_back( searchPaths.back().GetParentPath() );		// parent directory (for test/TestCase.h)

		fs::LocateExistingFile( &pchFilePath, searchPaths, _T("pch.h|stdafx.h") );
	}

	LookupFieldValue( _T("%PCH%") ) = pchFilePath.GetFname();
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
