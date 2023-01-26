#ifndef PathUniqueMaker_h
#define PathUniqueMaker_h
#pragma once

#include "Path.h"
#include <unordered_set>


// Generates unique paths by suffixing with a sequence count on filename collisions, while maintaining an index of existing paths.
// It doesn't check for files existing in the file-system.

class CPathUniqueMaker
{
public:
	CPathUniqueMaker( const TCHAR fmtNumSuffix[] = path::StdFormatNumSuffix() );

	bool IsUniquePath( const fs::CPath& filePath ) const { ASSERT( !filePath.IsEmpty() ); return m_uniquePathsIndex.find( filePath ) == m_uniquePathsIndex.end(); }

	template< typename PathT >
	PathT MakeUnique( const PathT& filePath )
	{
		PathT uniquePath;
		fs::traits::SetPath( uniquePath, MakeUniqueFilename( filePath ) );

		Register( fs::traits::GetPath( uniquePath ) );		// cache the path
		return uniquePath;
	}

	template< typename ContainerT >
	size_t UniquifyPaths( ContainerT& rPaths )
	{
		size_t dupCount = 0;

		for ( typename ContainerT::iterator itPath = rPaths.begin(); itPath != rPaths.end(); ++itPath )
			if ( IsUniquePath( fs::traits::GetPath( *itPath ) ) )
				Register( *itPath );		// cache the path
			else
			{
				*itPath = MakeUnique( *itPath );
				++dupCount;					// account for collision
			}

		return dupCount;
	}
private:
	void Register( const fs::CPath& filePath ) { VERIFY( m_uniquePathsIndex.insert( filePath ).second ); }

	void SetupSuffixPattern( void );
	fs::CPath MakeUniqueFilename( const fs::CPath& filePath ) const;
	UINT QueryExistingSequenceCount( const fs::CPath& filePath ) const;

	static bool ParseNumericSuffix( UINT& rNumber, const TCHAR* pText );
private:
	const TCHAR* m_pFmtNumSuffix;
	std::tstring m_fnSuffixPattern;		// e.g. "_[*]" for "_[%d]"

	std::unordered_set< fs::CPath > m_uniquePathsIndex;
};


#endif // PathUniqueMaker_h
