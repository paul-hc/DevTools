#ifndef IncludePaths_h
#define IncludePaths_h
#pragma once


namespace loc
{
	enum IncludeLocation
	{
		StandardPath,		// file is located in standard INCLUDE path directories
		LocalPath,			// file is located in local specified path
		AdditionalPath,		// file is located in additional include path directories
		AbsolutePath,		// file path is absolute
		SourcePath,			// file is located in SOURCE path directories
		LibraryPath,		// file is located in LIBRARY path directories
		BinaryPath			// file is located in BINARY path directories
	};
}


typedef std::pair< std::tstring, loc::IncludeLocation > PathLocationPair;


class CIncludePaths
{
public:
	CIncludePaths( void );
	~CIncludePaths();

	const std::vector< std::tstring >& GetStandard( void ) const { return m_standard; }
	const std::vector< std::tstring >& GetSource( void ) const { return m_source; }
	const std::vector< std::tstring >& GetLibrary( void ) const { return m_library; }
	const std::vector< std::tstring >& GetBinary( void ) const { return m_binary; }
	const std::vector< std::tstring >& GetMoreAdditional( void ) const { return m_moreAdditional; }

	void SetMoreAdditionalIncludePath( const std::tstring& moreAdditional );

	static const TCHAR* GetLocationTag( loc::IncludeLocation location );
private:
	void UpdateMoreAdditional( void );

	static bool AddIncludePath( std::vector< std::tstring >& rPaths, const TCHAR* pIncPaths );

	static bool AddVC6RegistryPath( std::vector< std::tstring >& rPaths, LPCTSTR pEntry );

	static bool AddVC71RegistryPath( std::vector< std::tstring >& rPaths, LPCTSTR pEntry );
	static const std::tstring& GetVC71InstallDir( void );
private:
	std::vector< std::tstring > m_standard;			// standard INCLUDE path directories
	std::vector< std::tstring > m_source;			// SOURCE path directories
	std::vector< std::tstring > m_library;			// LIBRARY path directories
	std::vector< std::tstring > m_binary;			// BINARY path directories
	std::vector< std::tstring > m_moreAdditional;	// more additional include path directories (IDETools stored extra includes)

	static const TCHAR* m_pathSep;
};


#endif // IncludePaths_h
