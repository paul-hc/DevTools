#ifndef ResourceFile_h
#define ResourceFile_h
#pragma once

#include <iosfwd>
#include "utl/Path.h"
#include "utl/FileState.h"
#include "utl/TextFileUtils.h"


// parses an .rc resource file, and allows modification of VS_VERSION_INFO values in StringFileInfo block
//
class CResourceFile : private utl::noncopyable
{
public:
	CResourceFile( const fs::CPath& rcFilePath ) throws_( CRuntimeException );
	~CResourceFile();

	bool HasBuildTimestamp( void ) const { return m_posBuildTimestamp != utl::npos; }
	CTime StampBuildTime( const CTime& buildTimestamp ) throws_( CRuntimeException );
	void Save( void ) const throws_( std::exception, CException* );
	void Report( std::ostream& os ) const;

	unsigned int GetBuildTimestampLineNum( void ) const { ASSERT( HasBuildTimestamp() ); return static_cast<unsigned int>( m_posBuildTimestamp ) + 1; }
private:
	std::string& RefBuildTimestampLine( void ) { ASSERT( HasBuildTimestamp() ); return m_lines[ m_posBuildTimestamp ]; }

	static size_t FindSeparator( const std::string& line, char chSep, size_t offset = 0 );
	static bool ParseVersionInfo( std::tstring& rSectionKey, const std::string& line );

	
	class CVersionInfoParser : public ILineParserCallback<std::string>
	{
	public:
		CVersionInfoParser( CResourceFile* pOwner ) : m_pOwner( pOwner ), m_viFlags( 0 ), m_viDepthLevel( 0 ) {}

		static void ReplaceBuildTimestamp( std::string& rTsLine, const CTime& buildTimestamp );
	private:
		virtual bool OnParseLine( const std::string& line, unsigned int lineNo );

		static size_t FindToken( const char* pText, const char* pPart, size_t offset = 0 );
		static bool ContainsToken( const char* pText, const char* pPart, size_t offset = 0 );
		static size_t LookupDelim( const char* pText, char delim, size_t offset, bool goAfter = true ) throws_( CRuntimeException );

		enum VersionInfoFlags
		{
			Found_VersionInfo = BIT_FLAG( 0 ),
			Found_BuildTimestamp = BIT_FLAG( 1 ),
			Done_VersionInfo = BIT_FLAG( 2 )
		};
		typedef int TVersionInfoFlags;
	private:
		CResourceFile* m_pOwner;
		TVersionInfoFlags m_viFlags;
		int m_viDepthLevel;
	};

	friend class CVersionInfoParser;
private:
	fs::CPath m_rcFilePath;
	fs::CFileState m_origFileState;
	std::vector< std::string > m_lines;
	size_t m_posBuildTimestamp;						// index in m_lines where the BuildTimestamp value is located
	CTime m_origBuildTimestamp;						// IN
	CTime m_newBuildTimestamp;						// OUT
	unsigned int m_parseLineNo;

	static const char s_doubleQuote[];
public:
	static const str::CPart<char> s_buildTimestampName;	// value name of the "BuildTimestamp" entry
};


#endif // ResourceFile_h
