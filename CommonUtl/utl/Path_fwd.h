#ifndef Path_fwd_h
#define Path_fwd_h
#pragma once


namespace fs
{
	class CPath;
	class CFlexPath;	


	typedef CPath TFilePath;			// alias for file paths
	typedef CPath TDirPath;				// alias for directory paths
	typedef CPath TPatternPath;			// alias for file path or directory paths with wildcards

	typedef CPath TEmbeddedPath;
	typedef CPath TStgDocPath;			// path to a compound document file
	typedef TDirPath TStgSubDirPath;	// sub-path of a sub-storage within a compound document


	template< typename PathT > class CPathIndex;
}


#endif // Path_fwd_h
