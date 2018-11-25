
#include "StdAfx.h"
#include "IniFileRegistrySection.h"
#include "PropertyLineReader.h"
#include "FileSystem.h"
#include "RuntimeException.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
	CIniFileRegistrySection represents a section of parameters in the configuration file with INI format.
	It reads the parameter list (key and element pairs) of the specific section from the input
	stream in a simple line-oriented format.

	Similar to .properties files, parameters are processed in terms of lines. Please see comments
	in the CProperties class for multi-line parameter format and distinction between natural lines
	and logical lines.

	A natural line that contains only white space characters is
	considered blank and is ignored.  A comment line has an ASCII
	';' as its first non-white space character; comment lines are also ignored and do not
	encode key-element information.

	Different from the .properties files, inline comments are also supported in the INI file,
	allowing comments to follow section specifications or parameter specifications.

	The key contains all of the characters in the line starting
	with the first non-white space character and up to, but not
	including, the first unescaped '=' or ':'. Different from the .properties file,
	whitespaces can be part of the key and therefore cannot be key/value separators.

	Parameters may be grouped into arbitrarily named sections. The section name appears
	on a line by itself, in square brackets. All parameters after the section declaration are associated
	with that section. There is no explicit "end of section" delimiter; sections end at the next section
	declaration, or the end of the file. Sections may not be nested. "Unnamed" sections are allowed and
	would appear at the beginning of the file. An empty section name "[]" is equivalent to unnamed section.

	Duplicate parameter in the same section overrides previous parameter with same key.

	The following is a example INI file:
		; last modified 1 October 2008 by John Doe
		[owner]
		name=John Doe
		organization=Acme Products

		[database]
		server=192.0.2.42	 ; use IP address in case network name resolution is not working
		port=143
		file = "acme payroll.dat"
*/

CIniFileRegistrySection::CIniFileRegistrySection( std::istream& rParameterStream, const std::tstring& section )
	: m_section( str::ToAnsi( section.c_str() ) )
{
	LoadSection( rParameterStream );
}

CIniFileRegistrySection::CIniFileRegistrySection( const fs::CPath& iniFilePath, const std::tstring& section )
	: m_section( str::ToAnsi( section.c_str() ) )
{
	if ( iniFilePath.FileExist() )
	{
		// only support ANSI INI filenames
		std::ifstream iniFileStream( iniFilePath.GetUtf8().c_str() );
		if ( !iniFileStream.is_open() )
			throw CRuntimeException( str::Format( _T("Unable to open configuration file %s."), iniFilePath.GetPtr() ) );

		LoadSection( iniFileStream );

		iniFileStream.close();
	}
}

size_t CIniFileRegistrySection::GetSize( void ) const
{
	return m_entries.size();
}

int CIniFileRegistrySection::GetIntParameter( const TCHAR entryName[], int defaultValue ) const
{
	std::map< std::string, std::string >::const_iterator iter = m_entries.find( str::ToAnsi( entryName ) );
	return iter != m_entries.end() ? atoi( iter->second.c_str() ) : defaultValue;
}

std::tstring CIniFileRegistrySection::GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue/* = NULL*/ ) const
{
	std::map< std::string, std::string>::const_iterator iter = m_entries.find( str::ToAnsi( entryName ) );
	if ( iter != m_entries.end() )
		return str::FromAnsi( iter->second.c_str() );

	return pDefaultValue != NULL ? pDefaultValue : std::tstring();
}

bool CIniFileRegistrySection::SaveParameter( const TCHAR entryName[], int value ) const
{
	// INI file is read-only for now
	entryName, value;
	return true;
}

bool CIniFileRegistrySection::SaveParameter( const TCHAR entryName[], const std::tstring& value ) const
{
	// INI file is read-only for now
	entryName, value;
	return true;
}


void CIniFileRegistrySection::LoadSection( std::istream& rParameterStream )
{
	CPropertyLineReader lineReader( rParameterStream );
	lineReader.SetCommentStyle( ";", true /* allow inline comments */ );

	if ( !LocateSection( lineReader ) )
		return;

	int lineSize = 0;
	while ( ( lineSize = lineReader.ReadLine() ) >= 0 )
	{
		std::string newSection;
		if ( ExtractSectionName( newSection, lineReader.m_lineBuffer.get(), lineSize ) )
			if ( m_section != newSection )
				break;		// end of current section
			else
				continue;	// skip repeated section line

		int valueStart = lineSize;
		bool hasSeperator = false;
		bool precedingBackslash = false;

		int keySize = 0;
		for ( ; keySize < lineSize; ++keySize )
		{
			char c = lineReader.m_lineBuffer.get()[keySize];

			if ( ( c == '=' || c == ':' ) && !precedingBackslash )
			{
				valueStart = keySize + 1;
				hasSeperator = true;
				break;
			}

			if (c == '\\')
				precedingBackslash = !precedingBackslash;
			else
				precedingBackslash = false;
		}

		std::string key( lineReader.m_lineBuffer.get(), keySize );
		key.erase ( key.find_last_not_of ( " \t" ) + 1 );

		std::string value( lineReader.m_lineBuffer.get() + valueStart, lineSize - valueStart );
		value.erase ( value.find_last_not_of ( " \t" ) + 1 );
		value.erase ( 0, value.find_first_not_of ( " \t" ) );

		// replace double slash with single slash
		for ( size_t index = 0; ( index = value.find( "\\\\" ) ) != std::string::npos; )
		{
			value.replace( index, 2, "\\" );
		}

		m_entries[key] = value;
	}
}

bool CIniFileRegistrySection::LocateSection( CPropertyLineReader& rLineReader ) const
{
	if ( m_section.empty() )
		return true;	// read non-section parameters if registry section is not specified

	// skip lines until the specified section
	int lineSize = 0;
	while ( ( lineSize = rLineReader.ReadLine() ) >= 0 )
	{
		std::string newSection;
		if ( ExtractSectionName( newSection, rLineReader.m_lineBuffer.get(), lineSize ) )
			if ( m_section == newSection )
				return true;
	}

	return false;
}

bool CIniFileRegistrySection::ExtractSectionName( std::string& rOutSectionName, const char* pLineBuffer, int lineSize ) const
{
	ASSERT( pLineBuffer != NULL );

	bool isSection = false;
	std::string newSection;

	if( lineSize >= 2 && '[' == pLineBuffer[0] )
	{
		for ( int sectionIndex = 1; sectionIndex < lineSize; ++sectionIndex )
		{
			char c = pLineBuffer[sectionIndex];

			if ( isSection && c != ' ' && c != '\t' && c != '\f' )
			{
				isSection = false;	// invalid section specification line if non white space characters follows section specification
				break;
			}

			if ( c == '[' )
				break;	// skip invalid section specification
			else if ( c == ']' )
			{
				isSection = true;

				newSection = std::string( pLineBuffer + 1, sectionIndex - 1 );
				newSection.erase ( newSection.find_last_not_of ( " \t" ) + 1 );
				newSection.erase ( 0, newSection.find_first_not_of ( " \t" ) );
			}
		}
	}

	if ( isSection )
		rOutSectionName = newSection;

	return isSection;
}
