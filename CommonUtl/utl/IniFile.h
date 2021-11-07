#ifndef IniFile_h
#define IniFile_h
#pragma once

#include <map>


namespace fs { class CPath; }


// a collection of sections made of properties with key=value structure that can be serialized to/from text file/stream

class CIniFile
{
public:
	typedef std::map< std::tstring, std::tstring > TSectionMap;

	CIniFile( void ) {}

	const TSectionMap* FindSection( const std::tstring& sectionKey ) const;
	TSectionMap& RetrieveSection( const std::tstring& sectionKey );
	TSectionMap& operator[]( const std::tstring& sectionKey ) { return RetrieveSection( sectionKey ); }

	bool SwapSectionProperties( const std::tstring& sectionKey, TSectionMap& rProperties );

	void Save( const fs::CPath& filePath ) const throws_( std::exception );
	void Load( const fs::CPath& filePath ) throws_( std::exception );

	void Save( std::ostream& rOutStream ) const throws_( std::exception );
	void Load( std::istream& rInStream ) throws_( std::exception );
private:
	static std::string ConvertToUtf8( const std::tstring& text );
	static std::tstring ParseFromUtf8( const std::string& text );

	static size_t FindSeparator( const std::string& line, char chSep, size_t offset = 0 );

	static bool ParseSection( std::tstring& rSectionKey, const std::string& line );
	static bool ParseProperty( std::tstring& rPropKey, std::tstring& rPropValue, const std::string& line );
private:
	std::map< std::tstring, TSectionMap > m_sectionMap;		// section name to property bag
	std::vector< std::tstring > m_orderedSections;			// order in which sections get saved

	static unsigned int s_parseLineNo;
};


#endif // IniFile_h
