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

	std::tstring GetPath( void ) const { return m_parts.MakePath(); }
	bool SetPath( const std::tstring& path );
	void Clear( void );

	bool IsValid( void ) const { return fs::FileExist( GetPath().c_str() ); }
	bool IsValidKnownAssoc( void ) const;
	bool HasSpecialAssociations( void ) const { return m_hasSpecialAssociations; }

	std::tstring GetNextAssoc( bool forward = true );
	std::tstring GetNextFileNameVariation( bool forward = true );
	std::tstring GetNextFileNameVariationEx( bool forward = true );

	std::tstring GetComplementaryAssoc( void );

	// File associations helpers
	static bool IsResourceHeaderFile( const fs::CPathParts& parts );
protected:
	int GetNextIndex( int& index, bool forward = true ) const;
	void GetComplIndex( std::vector< CircularIndex >& rIndexes ) const;

	std::tstring FindAssociation( const TCHAR* pExt ) const;

	static std::tstring LookupSpecialFullPath( const std::tstring& wildPattern );
	static void QueryComplementaryParentDirs( std::vector< std::tstring >& rComplementaryDirs, const std::tstring& dir );

	std::tstring GetSpecialComplementaryAssoc( void ) const;

	void FindVariationsOf( std::vector< std::tstring >& rVariations, int& rThisIdx, const std::tstring& pattern );
protected:
	fs::CPathParts m_parts;
	ft::FileType m_fileType;
	CircularIndex m_circularIndex;
	bool m_hasSpecialAssociations;			// has special associations

	static const TCHAR*	m_circularExt[];
	static const TCHAR*	m_relDirs[];
};


#endif // FileAssoc_h
