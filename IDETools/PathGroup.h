#ifndef PathGroup_h
#define PathGroup_h
#pragma once

#include "utl/Path.h"


class CEnumTags;


class CPathGroup
{
public:
	CPathGroup( void ) {}
	CPathGroup( const TCHAR environVar[] );

	void Clear( void );

	const std::vector< std::tstring >& GetSpecs( void ) const { return m_specs; }
	const std::vector< fs::CPath >& GetPaths( void ) const { return m_paths; }

	bool Contains( const fs::CPath& path ) const;
	bool AnyIsPartOf( const fs::CPath& path ) const;		// any of m_paths is sub-path of path?

	// specs
	std::tstring Join( const TCHAR sep[] = s_sep ) const;
	void Store( const TCHAR flat[], const TCHAR delims[] = s_sep );
	void Add( const TCHAR flat[], const TCHAR delims[] = s_sep );

	void Load( const TCHAR section[], const TCHAR entry[] );
	void Save( const TCHAR section[], const TCHAR entry[] ) const;
private:
	void ExpandPaths( void );
private:
	std::vector< std::tstring > m_specs;			// path specifiers could contain environment variables
	std::vector< fs::CPath > m_paths;				// expanded environment variables
public:
	static const TCHAR s_sep[];
};


typedef CPathGroup TFilePathGroup;


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


	typedef std::pair< fs::CPath, Location > TPathLocPair;


	class CDirPathGroup : public CPathGroup
	{
	public:
		CDirPathGroup( inc::Location location ) : m_location( location ) {}
		CDirPathGroup( const TCHAR environVar[], inc::Location location ) : CPathGroup( environVar ), m_location( location ) {}

		inc::Location GetLocation( void ) const { return m_location; }
	private:
		const inc::Location m_location;
	};
}


#endif // PathGroup_h
