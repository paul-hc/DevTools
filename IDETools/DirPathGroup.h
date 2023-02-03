#ifndef DirPathGroup_h
#define DirPathGroup_h
#pragma once

#include "utl/PathGroup.h"


class CEnumTags;


namespace inc
{
	enum Location
	{
		StandardPath,		// file is located in standard INCLUDE path directories
		LocalPath,			// file is located in local specified path
		AdditionalPath,		// file is located in additional include path directories
		AbsolutePath,		// file path is absolute
		SourcePath,			// file is located in SOURCE path directories
		LibraryPath,		// file is located in LIBRARY path directories
		BinaryPath			// file is located in BINARY path directories
	};

	const CEnumTags& GetTags_Location( void );


	typedef std::pair<fs::CPath, Location> TPathLocPair;


	class CDirPathGroup : public fs::CPathGroup
	{
	public:
		CDirPathGroup( inc::Location location );
		CDirPathGroup( const TCHAR envVarName[], inc::Location location );

		inc::Location GetLocation( void ) const { return m_location; }

		// text formatting "tag=value"
		std::tstring Format( void ) const;
		bool Parse( const std::tstring& spec );
	private:
		const inc::Location m_location;
	};


	enum SearchFlag
	{
		Flag_StandardPath	= 1 << StandardPath,		// standard INCLUDE path <...>
		Flag_LocalPath		= 1 << LocalPath,			// local path "..."
		Flag_AdditionalPath	= 1 << AdditionalPath,		// additional include path

		Flag_SourcePath		= 1 << SourcePath,			// standard SOURCE path
		Flag_LibraryPath	= 1 << LibraryPath,			// standard LIBRARY path
		Flag_BinaryPath		= 1 << BinaryPath,			// standard BINARY path

			Mask_AllIncludePaths	= Flag_StandardPath | Flag_LocalPath | Flag_AdditionalPath,
			Mask_AllPaths			= Mask_AllIncludePaths | Flag_SourcePath | Flag_LibraryPath | Flag_BinaryPath
	};

	typedef int TSearchFlags;

	typedef std::pair<const CDirPathGroup*, SearchFlag> TDirSearchPair;
}


#endif // DirPathGroup_h
