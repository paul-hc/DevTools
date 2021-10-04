
#include "StdAfx.h"
#include "IniFile.h"
#include "ContainerUtilities.h"
#include "Path.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace sep
{
	static const char sectionBegin = '[';
	static const char sectionEnd = ']';
	static const char entryValue = '=';
	static const char whitespace[] = " \t";
}


unsigned int CIniFile::s_parseLineNo = 1;

const CIniFile::SectionMap* CIniFile::FindSection( const std::tstring& sectionKey ) const
{
	std::map< std::tstring, SectionMap >::const_iterator itFound = m_sectionMap.find( sectionKey );
	return itFound != m_sectionMap.end() ? &itFound->second : NULL;
}

CIniFile::SectionMap& CIniFile::RetrieveSection( const std::tstring& sectionKey )
{
	ASSERT( !utl::Contains( sectionKey, sep::sectionBegin ) || !utl::Contains( sectionKey, sep::sectionEnd ) );
	ASSERT( !utl::Contains( m_orderedSections, sectionKey ) );
	m_orderedSections.push_back( sectionKey );
	return m_sectionMap[ sectionKey ];
}

bool CIniFile::SwapSectionProperties( const std::tstring& sectionKey, SectionMap& rProperties )
{
	if ( rProperties.empty() )
		return false;

	RetrieveSection( sectionKey ).swap( rProperties );
	rProperties.clear();
	return true;
}

void CIniFile::Save( const fs::CPath& filePath ) const throws_( std::exception )
{
	std::ofstream output( filePath.GetPtr() );
	if ( !output.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to open properties file %s."), filePath ) );

	Save( output );
	output.close();
}

void CIniFile::Load( const fs::CPath& filePath ) throws_( std::exception )
{
	std::ifstream input( filePath.GetPtr() );
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to open properties file %s."), filePath ) );

	Load( input );
	input.close();
}

void CIniFile::Save( std::ostream& rOutStream ) const throws_( std::exception )
{
	// save sections according to order stored in m_orderedSections

	for ( std::vector< std::tstring >::const_iterator itSectionKey = m_orderedSections.begin();
		  itSectionKey != m_orderedSections.end(); ++itSectionKey )
	{
		const SectionMap* pSection = FindSection( *itSectionKey );
		ASSERT_PTR( pSection );

		if ( !pSection->empty() )
		{
			rOutStream << sep::sectionBegin << ConvertToUtf8( itSectionKey->c_str() ) << sep::sectionEnd << std::endl;

			for ( SectionMap::const_iterator itEntry = pSection->begin();
				  itEntry != pSection->end(); ++itEntry )
				rOutStream << ConvertToUtf8( itEntry->first ) << sep::entryValue << ConvertToUtf8( itEntry->second ) << std::endl;

			rOutStream << std::endl;
		}
	}
}

void CIniFile::Load( std::istream& rInStream ) throws_( std::exception )
{
	SectionMap* pCurrentSection = NULL;

	for ( s_parseLineNo = 1; !rInStream.eof(); ++s_parseLineNo )
	{
		std::string line;
		std::getline( rInStream, line );

		std::tstring sectionKey;
		if ( ParseSection( sectionKey, line ) )
			pCurrentSection = &RetrieveSection( sectionKey );
		else
		{
			std::tstring propKey, propValue;
			if ( ParseProperty( propKey, propValue, line ) )
			{
				if ( NULL == pCurrentSection )
					pCurrentSection = &RetrieveSection( std::tstring() );	// root section is implicit

				(*pCurrentSection)[ propKey ] = propValue;					// store the property
			}
		}
	}
}

std::string CIniFile::ConvertToUtf8( const std::tstring& text )
{
	std::string utf8 = str::ToUtf8( text.c_str() );
	std::ostringstream output;

	for ( std::string::const_iterator itCh = utf8.begin(); itCh != utf8.end(); ++itCh )
		switch ( *itCh )
		{
			case '\a':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
			case '\\':
			case '[':
			case ']':
			case '=':
				output << '\\' << *itCh;
				break;
			default:
				output << *itCh;
				break;
		}

	return output.str();
}

std::tstring CIniFile::ParseFromUtf8( const std::string& text )
{
	std::ostringstream output;

	for ( std::string::const_iterator itCh = text.begin(); itCh != text.end(); ++itCh )
		if ( '\\' == *itCh )
			switch ( *++itCh )
			{
				case 'a': output << '\a'; break;
				case 'b': output << '\b'; break;
				case 'f': output << '\f'; break;
				case 'n': output << '\n'; break;
				case 'r': output << '\r'; break;
				case 't': output << '\t'; break;
				case 'v': output << '\v'; break;

				case '\\':
				case '[':
				case ']':
				case '=':
					output << *itCh;
					break;
				default:
					throw CRuntimeException( str::Format( _T("Unrecognized escape sequence %s on line %d."), str::FromUtf8( text.c_str() ), s_parseLineNo ) );
			}
		else
			output << *itCh;

	return str::FromUtf8( output.str().c_str() );
}

size_t CIniFile::FindSeparator( const std::string& line, char chSep, size_t offset /*= 0*/ )
{
	size_t posSep = offset;
	for ( ; ( posSep = line.find( chSep, posSep ) ) != std::string::npos; ++posSep )
		if ( 0 == posSep || line[ posSep - 1 ] != '\\' )		// not an escape sequence
			return posSep;

	return std::string::npos;
}

bool CIniFile::ParseSection( std::tstring& rSectionKey, const std::string& line )
{
	size_t posStart = FindSeparator( line, sep::sectionBegin );
	if ( posStart != std::string::npos )
	{
		size_t posEnd = FindSeparator( line, sep::sectionEnd, posStart + 1 );
		if ( posEnd != std::string::npos && ( ++posStart, posEnd >= posStart ) )
		{
			rSectionKey = str::FromUtf8( line.substr( posStart, posEnd - posStart ).c_str() );		// section key can be empty (root key)
			return true;
		}
	}
	return false;
}

bool CIniFile::ParseProperty( std::tstring& rPropKey, std::tstring& rPropValue, const std::string& line )
{
	rPropKey.clear();
	size_t posSep = FindSeparator( line, sep::entryValue );
	if ( posSep != std::string::npos )
	{
		rPropKey = ParseFromUtf8( line.substr( 0, posSep ) );
		rPropValue = ParseFromUtf8( line.substr( posSep + 1 ) );
	}
	return !rPropKey.empty();
}
