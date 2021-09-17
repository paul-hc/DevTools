
#include "stdafx.h"
#include "ResourceFile.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileUtils.hxx"


const str::CPart<char> CResourceFile::s_buildTimestampName = str::MakePart( "\"BuildTimestamp\"" );
const char CResourceFile::s_doubleQuote[] = "\"";

CResourceFile::CResourceFile( const fs::CPath& rcFilePath ) throws_( CRuntimeException )
	: m_rcFilePath( rcFilePath )
	, m_origFileState( fs::CFileState::ReadFromFile( m_rcFilePath ) )
	, m_posBuildTimestamp( utl::npos )
	, m_parseLineNo( 1 )
{
	CVersionInfoParser parserCallback( this );
	CTextFileParser<std::string> parser( &parserCallback );

	parser.ParseFile( m_rcFilePath );
}

CResourceFile::~CResourceFile()
{
}

CTime CResourceFile::StampBuildTime( const CTime& buildTimestamp )
{
	REQUIRE( HasBuildTimestamp() );
	m_newBuildTimestamp = buildTimestamp;

	CVersionInfoParser::ReplaceBuildTimestamp( m_lines[ m_posBuildTimestamp ], m_newBuildTimestamp );
	return m_origBuildTimestamp;
}

void CResourceFile::Save( void ) const throws_( std::exception, CException* )
{
	utl::WriteLinesToFile( m_rcFilePath, m_lines );		// save all lines
	m_origFileState.WriteToFile();						// restore the original file access times
}

void CResourceFile::Report( std::ostream& os ) const
{
	REQUIRE( HasBuildTimestamp() );

	// c:\dev\DevTools\Slider\Slider.rc(920) : stamped build time to '17-09-2021 14:02:39'  (last was '17-09-2021 13:58:03').
	//
	os << m_rcFilePath << "(" << GetBuildTimestampLineNum() << ") : stamped build time to '" << time_utl::FormatTimestamp( m_newBuildTimestamp ) << "'";

	if ( time_utl::IsValid( m_origBuildTimestamp ) )
		os << "  (last was '" << time_utl::FormatTimestamp( m_origBuildTimestamp ) << "')";

	os << "." << std::endl;
}


// CResourceFile::CVersionInfoParser implementation

bool CResourceFile::CVersionInfoParser::OnParseLine( const std::string& line, unsigned int lineNo )
{
	m_pOwner->m_lines.push_back( line );

	if ( !HasFlag( m_viFlags, Found_VersionInfo ) )
		if ( ContainsToken( line.c_str(), "VS_VERSION_INFO" ) )
			SetFlag( m_viFlags, Found_VersionInfo );

	if ( HasFlag( m_viFlags, Found_VersionInfo ) && !HasFlag( m_viFlags, Done_VersionInfo ) )		// parsing in VS_VERSION_INFO?
	{
		if ( ContainsToken( line.c_str(), "BEGIN" ) )
			++m_viDepthLevel;
		else if ( ContainsToken( line.c_str(), "END" ) )
		{
			--m_viDepthLevel;

			if ( 0 == m_viDepthLevel )
				SetFlag( m_viFlags, Done_VersionInfo );			// final END, close VS_VERSION_INFO parsing
		}
		else if ( !m_pOwner->HasBuildTimestamp() )
		{
			static const str::CPart<char> s_VALUE = str::MakePart( "VALUE" );
			size_t foundPos = FindToken( line.c_str(), s_VALUE.m_pString );

			if ( foundPos != std::string::npos )
			{
				const char* pCore = str::SkipWhitespace( line.c_str() + foundPos + s_VALUE.m_count );

				if ( str::EqualsPart( pCore, s_buildTimestampName ) )
				{	// found entry: VALUE "BuildTimestamp"
					m_pOwner->m_posBuildTimestamp = lineNo - 1;
					SetFlag( m_viFlags, Found_BuildTimestamp );

					pCore += s_buildTimestampName.m_count;
					pCore = str::SkipWhitespace( pCore );
					pCore = str::SkipWhitespace( pCore, "," );
					pCore = str::SkipWhitespace( pCore );

					std::vector< std::string > values;
					str::QueryEnclosedItems( values, pCore, s_doubleQuote, s_doubleQuote, false );

					if ( !values.empty() )
						m_pOwner->m_origBuildTimestamp = time_utl::ParseStdTimestamp( str::FromAnsi( values.front().c_str() ) );		// store the original value of the timestamp
				}
			}
		}
	}

	return true;
}

size_t CResourceFile::CVersionInfoParser::FindToken( const char* pText, const char* pPart, size_t offset /*= 0*/ )
{
	size_t pos = str::Find< str::Case >( pText, pPart, offset );

	if ( pos != std::string::npos )
	{
		const char* pWhiteSpace = str::StdWhitespace<char>();

		if ( pos != 0 && !str::ContainsAnyOf( pWhiteSpace, pText[ pos - 1 ] ) )
			return std::string::npos;		// not preceeded by a whitespace

		size_t endPos = pos + str::GetLength( pPart );

		if ( pText[ endPos ] != _T('\0') && !str::ContainsAnyOf( pWhiteSpace, pText[ endPos ] ) )
			return std::string::npos;		// not followed by a whitespace
	}

	return pos;
}

bool CResourceFile::CVersionInfoParser::ContainsToken( const char* pText, const char* pPart, size_t offset /*= 0*/ )
{
	size_t pos = str::Find< str::Case >( pText, pPart, offset );
	if ( std::string::npos == pos )
		return false;

	const char* pWhiteSpace = str::StdWhitespace<char>();

	if ( pos != 0 && !str::ContainsAnyOf( pWhiteSpace, pText[ pos - 1 ] ) )
		return false;		// not preceeded by a whitespace

	size_t endPos = pos + str::GetLength( pPart );

	if ( pText[ endPos ] != _T('\0') && !str::ContainsAnyOf( pWhiteSpace, pText[ endPos ] ) )
		return false;		// not followed by a whitespace

	return true;
}

size_t CResourceFile::CVersionInfoParser::LookupDelim( const char* pText, char delim, size_t offset, bool goAfter /*= true*/ ) throws_( CRuntimeException )
{
	size_t foundPos = str::Find<str::Case>( pText, delim, offset );
	if ( utl::npos == foundPos )
		throw CRuntimeException( str::Format( "Cannot find delimiter '%c' in '%s' from offset %d.", delim, pText, offset ) );

	if ( goAfter )
		++foundPos;

	return foundPos;
}

void CResourceFile::CVersionInfoParser::ReplaceBuildTimestamp( std::string& rTsLine, const CTime& buildTimestamp )
{
	size_t startPos = 0;

	startPos = LookupDelim( rTsLine.c_str(), s_doubleQuote[ 0 ], startPos );			// after opening quote for Name
	startPos = LookupDelim( rTsLine.c_str(), s_doubleQuote[ 0 ], startPos );			// after closing quote for Name
	startPos = LookupDelim( rTsLine.c_str(), s_doubleQuote[ 0 ], startPos );			// after opening quote for Value

	size_t endPos = LookupDelim( rTsLine.c_str(), s_doubleQuote[ 0 ], startPos, false );	// on closing quote for Value

	rTsLine = rTsLine.substr( 0, startPos ) + str::ToAnsi( time_utl::FormatTimestamp( buildTimestamp ).c_str() ) + rTsLine.substr( endPos );
}
