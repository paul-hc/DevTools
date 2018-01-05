#ifndef SearchPathEngine_h
#define SearchPathEngine_h
#pragma once

#include "utl/Path.h"
#include "IncludePaths.h"
#include "IncludeNode.h"


namespace sp
{
	enum SearchInPath
	{
		StandardPath	= 0x0001,		// standard INCLUDE path <...>
		LocalPath		= 0x0002,		// local path "..."
		AdditionalPath	= 0x0004,		// additional include path (from DSP file)

		SourcePath		= 0x0010,		// standard SOURCE path
		LibraryPath		= 0x0020,		// standard LIBRARY path
		BinaryPath		= 0x0040,		// standard BINARY path

		AllIncludePaths	= StandardPath | LocalPath | AdditionalPath,
		AllPaths		= AllIncludePaths | SourcePath | LibraryPath | BinaryPath
	};
}


class CSearchPathEngine
{
public:
	CSearchPathEngine( void ) {}

	void SetDspAdditionalIncludePath( const std::tstring& dspDirPath, const std::tstring& dspAdditionalIncludePath, const TCHAR* pSep );

	bool FindFirstIncludeFile( std::pair< std::tstring, loc::IncludeLocation >& rFoundFile, const CIncludeTag& includeTag, const std::tstring& localDirPath,
							   int searchInPath = sp::AllIncludePaths ) const;

	size_t QueryIncludeFiles( std::vector< PathLocationPair >& rFoundFiles, const CIncludeTag& includeTag, const std::tstring& localDirPath,
							  int searchInPath = sp::AllIncludePaths, size_t maxCount = std::tstring::npos ) const;
private:
	static bool CheckValidFile( PathLocationPair& rFoundFile, const std::tstring& fullPath, loc::IncludeLocation location );
private:
	std::vector< std::tstring > m_dspAdditionalIncludePath;
};


#endif // SearchPathEngine_h
