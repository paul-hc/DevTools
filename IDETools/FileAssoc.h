#ifndef FileAssoc_h
#define FileAssoc_h
#pragma once

#include "utl/Path.h"
#include "FileType.h"


enum CircularIndex;


class CFileAssoc
{
public:
	CFileAssoc( void ) { Clear(); }
	CFileAssoc( const std::tstring& path ) { SetPath( path ); }

	fs::CPath GetPath( void ) const { return m_parts.MakePath(); }
	bool SetPath( const fs::CPath& path );
	void Clear( void );

	bool IsValid( void ) const { return GetPath().FileExist(); }
	bool IsValidKnownAssoc( void ) const;
	bool HasSpecialAssociations( void ) const { return m_hasSpecialAssociations; }

	fs::CPath GetNextAssoc( bool forward = true );
	fs::CPath GetNextFileNameVariation( bool forward = true );
	fs::CPath GetNextFileNameVariationEx( bool forward = true );

	fs::CPath GetComplementaryAssoc( void );

	// File associations helpers
	static bool IsResourceHeaderFile( const fs::CPathParts& parts );
protected:
	int GetNextIndex( int& index, bool forward = true ) const;
	void GetComplIndex( std::vector< CircularIndex >& rIndexes ) const;

	fs::CPath FindAssociation( const TCHAR* pExt ) const;

	static fs::CPath LookupSpecialFullPath( const fs::CPath& wildPattern );
	static void QueryComplementaryParentDirs( std::vector< fs::CPath >& rComplementaryDirs, const fs::CPath& dir );

	fs::CPath GetSpecialComplementaryAssoc( void ) const;

	void FindVariationsOf( std::vector< fs::CPath >& rVariations, int& rThisIdx, const fs::CPath& pattern );
protected:
	fs::CPathParts m_parts;
	ft::FileType m_fileType;
	CircularIndex m_circularIndex;
	bool m_hasSpecialAssociations;			// has special associations

	static const TCHAR*	s_circularExt[];
	static const TCHAR*	s_relDirs[];
	static const fs::CPath s_emptyPath;
};


#endif // FileAssoc_h
