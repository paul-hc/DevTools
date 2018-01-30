#ifndef SearchPathEngine_fwd_h
#define SearchPathEngine_fwd_h
#pragma once


namespace sp
{
	enum SearchInPath
	{
		StandardPath	= 0x0001,		// standard INCLUDE path <...>
		LocalPath		= 0x0002,		// local path "..."
		AdditionalPath	= 0x0004,		// additional include path

		SourcePath		= 0x0010,		// standard SOURCE path
		LibraryPath		= 0x0020,		// standard LIBRARY path
		BinaryPath		= 0x0040,		// standard BINARY path

		AllIncludePaths	= StandardPath | LocalPath | AdditionalPath,
		AllPaths		= AllIncludePaths | SourcePath | LibraryPath | BinaryPath
	};
}


#endif // SearchPathEngine_fwd_h
