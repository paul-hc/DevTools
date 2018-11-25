#ifndef IRegistrySection_h
#define IRegistrySection_h
#pragma once

#include "ISubject.h"


interface IRegistrySection : public IMemoryManaged
{
	virtual int GetIntParameter( const TCHAR entryName[], int defaultValue ) const = 0;
	virtual std::tstring GetStringParameter( const TCHAR entryName[], const TCHAR* pDefaultValue = NULL ) const = 0;

	virtual bool SaveParameter( const TCHAR entryName[], int value ) const = 0;
	virtual bool SaveParameter( const TCHAR entryName[], const std::tstring& value ) const = 0;
};



#endif // IRegistrySection_h
