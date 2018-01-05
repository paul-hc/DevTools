#ifndef SearchSpec_h
#define SearchSpec_h
#pragma once

#include "utl/FileSystem.h"


class CSearchSpec
{
public:
	enum Type
	{
		Invalid,			// invalid search specifier
		DirPath,			// directory path
		ArchiveStgFile,		// file-path of an OLE compound file of type .scf
		ExplicitFile		// explicit file specifier
	};

	enum Options
	{
		Normal,				// don't recurse sub-dirs
		RecurseSubDirs,		// recurse sub-dirs
		AutoDropNumFormat	// filter only numeric format fnames (e.g. "0001.jpg")
	};

	enum PathType { PT_AsIs, PT_DirPath, PT_FilePath };
public:
	CSearchSpec( void ) : m_type( DirPath ), m_options( RecurseSubDirs ) {}
	CSearchSpec( const fs::CPath& searchPath, const std::tstring& searchFilters = std::tstring(), Options options = RecurseSubDirs ) { Setup( searchPath, searchFilters, options ); }

	bool operator==( const CSearchSpec& right ) const;
	bool operator!=( const CSearchSpec& right ) const { return !operator==( right ); }

	void Stream( CArchive& archive );

	bool Setup( const fs::CPath& searchPath, const std::tstring& searchFilters = std::tstring(), Options options = RecurseSubDirs );
	bool BrowseFilePath( PathType pathType = PT_AsIs, CWnd* pParentWnd = NULL, DWORD extraFlags = OFN_FILEMUSTEXIST );

	bool IsValidPath( void ) const { return m_searchPath.FileExist(); }

	bool IsDirPath( void ) const { return DirPath == m_type; }
	bool IsImageArchiveDoc( void ) const { return ArchiveStgFile == m_type; }
	bool IsExplicitFile( void ) const { return ExplicitFile == m_type; }

	bool IsAutoDropDirPath( bool checkValidPath = true ) const { return IsDirPath() && ( !checkValidPath || IsValidPath() ) && AutoDropNumFormat == m_options; }
	const std::tstring& GetImageSearchFilters( void ) const;

	void EnumImageFiles( fs::IEnumerator* pEnumerator ) const;
public:
	// numeric filename support
	static bool IsNumFileName( const TCHAR* pFullPath );
	static int ParseNumFileNameNumber( const TCHAR* pFullPath );
	static std::tstring FormatNumericFilePath( const TCHAR* pFullPath, int numSeq );
public:
	persist fs::CPath m_searchPath;			// directory path (if search attribute) or explicit file path
	persist std::tstring m_searchFilters;	// multiple filters for searching
	persist Type m_type;					// type of search attribute
	persist Options m_options;				// search options
};


#endif // SearchSpec_h
