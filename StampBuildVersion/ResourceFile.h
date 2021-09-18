#ifndef ResourceFile_h
#define ResourceFile_h
#pragma once

#include <iosfwd>
#include <stack>
#include "utl/Path.h"
#include "utl/FileState.h"
#include "utl/TextFileUtils.h"
#include "utl/TokenIterator.h"
#include "CmdLineOptions_fwd.h"


namespace rc
{
	typedef str::CTokenIterator< pred::TCompareCase, char > TTokenIterator;

	class CVersionInfoParser;
}


// parses an .rc resource file, and allows modification of VS_VERSION_INFO values in StringFileInfo block
//
class CResourceFile : private utl::noncopyable
{
	friend class rc::CVersionInfoParser;
public:
	CResourceFile( const fs::CPath& rcFilePath, app::TOption optionFlags ) throws_( CRuntimeException );
	~CResourceFile();

	bool HasBuildTimestamp( void ) const { return m_pos.m_buildTimestamp != utl::npos; }
	CTime StampBuildTime( const CTime& buildTimestamp ) throws_( CRuntimeException );
	void Save( void ) const throws_( std::exception, CException* );
	void Report( std::ostream& os ) const;
private:
	bool FoundFirstValue( void ) const { return m_pos.m_firstValue != utl::npos; }

	std::string& RefLine_BuildTimestamp( void ) { ASSERT( HasBuildTimestamp() ); return m_lines[ m_pos.m_buildTimestamp ]; }
	std::string& RefLine_FirstValue( void ) { ASSERT( FoundFirstValue() ); return m_lines[ m_pos.m_firstValue ]; }

	void OnEndParsing( void );
	unsigned int GetBuildTimestampLineNum( void ) const { ASSERT( HasBuildTimestamp() ); return static_cast<unsigned int>( m_pos.m_buildTimestamp ) + 1; }
private:
	struct CParsePos		// stores key positions found in VersionInfo section during parsing
	{
		CParsePos( void ) : m_buildTimestamp( utl::npos ), m_firstValue( utl::npos ) {}
	public:
		size_t m_buildTimestamp;				// index in m_lines where the BuildTimestamp value is located
		size_t m_firstValue;					// index in m_lines where the first value is located (in case we must add the "BuildTimestamp" value)
	};
private:
	fs::CPath m_rcFilePath;
	app::TOption m_optionFlags;
	fs::CFileState m_origFileState;

	std::vector< std::string > m_lines;
	CParsePos m_pos;							// index in m_lines where the BuildTimestamp value is located
	CTime m_origBuildTimestamp;					// IN
	CTime m_newBuildTimestamp;					// OUT
};


#endif // ResourceFile_h
