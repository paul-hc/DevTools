#ifndef IniFileRegistrySection_h
#define IniFileRegistrySection_h
#pragma once

#include "IRegistrySection.h"
#include <map>


namespace fs { class CPath; }
class CPropertyLineReader;


class CIniFileRegistrySection : public IRegistrySection
{
public:
	CIniFileRegistrySection( std::istream& rParameterStream, const std::tstring& section );
	CIniFileRegistrySection( const fs::CPath& iniFilePath, const std::tstring& section );

	size_t GetSize( void ) const;

	// IRegistrySection interface
	virtual const std::tstring& GetSectionName( void ) const;
	virtual int GetIntParameter( const TCHAR entryName[], int defaultValue ) const;
	virtual std::tstring GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue = NULL ) const;
	virtual bool SaveParameter( const TCHAR entryName[], int value ) const;
	virtual bool SaveParameter( const TCHAR entryName[], const std::tstring& value ) const;
private:
	void LoadSection( std::istream& rParameterStream );
	bool LocateSection( CPropertyLineReader& rLineReader ) const;
	bool ExtractSectionName( std::string& rOutSectionName, const char* pLineBuffer, int lineSize ) const;
private:
	std::tstring m_section;
	std::string m_sectionAnsi;
	std::map< std::string, std::tstring > m_entries;		// <name, value>	(ANSI -> WIDE)
};


#endif // IniFileRegistrySection_h
