#ifndef RegistrySection_h
#define RegistrySection_h
#pragma once

#include "IRegistrySection.h"


class CAppRegistrySection : public IRegistrySection
{
public:
	CAppRegistrySection( const std::tstring& section ) : m_pApp( AfxGetApp() ), m_section( section ) { ASSERT_PTR( m_pApp ); }

	// IRegistrySection implementation
	virtual const std::tstring& GetSectionName( void ) const;
	virtual int GetIntParameter( const TCHAR entryName[], int defaultValue ) const;
	virtual std::tstring GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue = nullptr ) const;

    virtual bool SaveParameter( const TCHAR entryName[], int value ) const;
    virtual bool SaveParameter( const TCHAR entryName[], const std::tstring& rValue ) const;
private:
	CWinApp* m_pApp;
	std::tstring m_section;
};


struct CRegistryEntry;


class CRegistrySection : public IRegistrySection
{
public:
	enum Hive { ClassesRoot, CurrentUser, LocalMachine, Users, CurrentConfig };

	CRegistrySection( Hive hive, const std::tstring& section );
	CRegistrySection( HKEY hKey, const std::tstring& section );

	// IRegistrySection implementation
	virtual const std::tstring& GetSectionName( void ) const;
	virtual int GetIntParameter( const TCHAR entryName[], int defaultValue ) const;
	virtual std::tstring GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue = nullptr ) const;

    virtual bool SaveParameter( const TCHAR entryName[], int value ) const;
    virtual bool SaveParameter( const TCHAR entryName[], const std::tstring& value ) const;
public:
	bool DeleteParameter( const TCHAR entryName[] ) const;
	bool DeleteSection( void ) const;
private:
	bool GetParameter( const TCHAR entryName[], CRegistryEntry& rEntry ) const;
	bool SaveParameter( const TCHAR entryName[], const CRegistryEntry& entry ) const;
private:
	HKEY m_hKey;
	std::tstring m_section;

	static const HKEY s_hives[];
};


#endif // RegistrySection_h
