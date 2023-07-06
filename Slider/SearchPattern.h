#ifndef SearchPattern_h
#define SearchPattern_h
#pragma once

#include "utl/FileSystem.h"
#include "utl/PathItemBase.h"
#include "Application_fwd.h"


class CEnumTags;


class CSearchPattern : public CPathItemBase
{
public:
	enum Type
	{
		DirPath,			// directory path
		AlbumFile,			// file-path of an album file: slide (.sld) or image catalog, i.e. OLE compound file storage (.ias, etc)
		SingleImage			// image file path (not a pattern)
	};

	enum SearchMode
	{
		ShallowDir,			// don't recurse sub-dirs
		RecurseSubDirs,		// recurse sub-dirs
		AutoDropNumFormat	// filter only numeric format fnames (e.g. "0001.jpg")
	};

	static const CEnumTags& GetTags_Type( void );
	static const CEnumTags& GetTags_SearchMode( void );

	enum BrowseMode { BrowseAsIs, BrowseAsDirPath, BrowseAsFilePath };
public:
	CSearchPattern( void );
	CSearchPattern( const fs::CPath& searchPath );

	bool operator==( const CSearchPattern& right ) const;
	bool operator!=( const CSearchPattern& right ) const { return !operator==( right ); }

	// base overrides
	virtual void SetFilePath( const fs::CPath& filePath );

	void Stream( CArchive& archive );
public:
	const std::tstring& GetWildFilters( void ) const { return m_wildFilters; }
	std::tstring& RefWildFilters( void ) { return m_wildFilters; }
	const std::tstring& GetSafeWildFilters( void ) const;

	Type GetType( void ) const { return m_type; }
	void SetAutoType( void ) { m_type = CheckType(); }

	SearchMode GetSearchMode( void ) const { return m_searchMode; }
	SearchMode& RefSearchMode( void ) { return m_searchMode; }
	void SetSearchMode( SearchMode searchMode ) { m_searchMode = searchMode; }

	bool BrowseFilePath( BrowseMode pathType = BrowseAsIs, CWnd* pParentWnd = nullptr, DWORD extraFlags = OFN_FILEMUSTEXIST );

	bool IsEmpty( void ) const { return GetFilePath().IsEmpty(); }
	bool IsValidPath( void ) const;

	bool IsDirPath( void ) const { return DirPath == m_type; }
	bool IsAlbumFile( void ) const { return AlbumFile == m_type; }
	bool IsSingleImage( void ) const { return SingleImage == m_type; }

	bool IsStorageAlbumFile( void ) const { return IsAlbumFile() && app::IsCatalogFile( GetFilePath().GetPtr() ); }
	bool IsAutoDropDirPath( bool checkValidPath = true ) const { return IsDirPath() && ( !checkValidPath || IsValidPath() ) && AutoDropNumFormat == m_searchMode; }

	void EnumImageFiles( fs::IEnumerator* pEnumerator ) const;
public:
	// numeric filename support
	static bool IsNumFileName( const TCHAR* pFullPath );
	static int ParseNumFileNameNumber( const TCHAR* pFullPath );
	static std::tstring FormatNumericFilePath( const TCHAR* pFullPath, int numSeq );
private:
	using CPathItemBase::ResetFilePath;

	Type CheckType( void ) const;
	static Type BestGuessType( const fs::CPath& searchPath );		// doesn't check for file existence
private:
	persist std::tstring m_wildFilters;		// multiple filters for searching
	persist Type m_type;					// type of search attribute
	persist SearchMode m_searchMode;		// search searchMode
};


#endif // SearchPattern_h
