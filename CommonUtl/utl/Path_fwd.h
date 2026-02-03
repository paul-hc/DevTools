#ifndef Path_fwd_h
#define Path_fwd_h
#pragma once


namespace fs
{
	class CPath;
	class CFlexPath;


	typedef CPath TFilePath;			// alias for file paths
	typedef CPath TDirPath;				// alias for directory paths
	typedef CPath TRelativePath;		// alias for relative paths
	typedef CPath TPatternPath;			// alias for file path or directory paths with wildcards

	typedef CPath TEmbeddedPath;
	typedef CPath TStgDocPath;			// path to a compound document file
	typedef TDirPath TStgSubDirPath;	// sub-path of a sub-storage within a compound document


	template< typename PathT > class CPathIndex;
}


namespace shell
{
	typedef fs::CPath TPath;			// alias for a SHELL PATH, which is either a file-system path (fs::CPath) or a GUID path (e.g. Control Panel folder, applet, etc)
	typedef TPath TRelativePath;		// alias for a relative SHELL PATH
	typedef TPath TDirPath;				// alias for directory paths
}


#endif // Path_fwd_h
