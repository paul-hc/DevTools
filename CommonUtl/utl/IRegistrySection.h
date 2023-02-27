#ifndef IRegistrySection_h
#define IRegistrySection_h
#pragma once


interface IRegistrySection : public utl::IMemoryManaged
{
	virtual const std::tstring& GetSectionName( void ) const = 0;

	virtual int GetIntParameter( const TCHAR entryName[], int defaultValue ) const = 0;
	virtual std::tstring GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue = nullptr ) const = 0;

	virtual bool SaveParameter( const TCHAR entryName[], int value ) const = 0;
	virtual bool SaveParameter( const TCHAR entryName[], const std::tstring& value ) const = 0;
};



#endif // IRegistrySection_h
