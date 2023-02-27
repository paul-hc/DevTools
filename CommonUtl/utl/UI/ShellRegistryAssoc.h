#ifndef ShellRegistryAssoc_h
#define ShellRegistryAssoc_h
#pragma once

#include "Registry_fwd.h"


namespace shell
{
	// registry shell association helpers for HKEY_CLASSES_ROOT hive sub-keys

	reg::TKeyPath MakeShellHandlerVerbPath( const TCHAR handlerName[], const TCHAR verb[] );	// "<handlerName>\\shell\\<verb>"
	bool QueryHandlerName( std::tstring& rHandlerName, const TCHAR ext[] );						// for ".txt" returns "Notepad++_file", for "SliderAlbum.Document" returns "Slider Album Document"

	const TCHAR* GetDdeOpenCmd( void );
	std::tstring MakeParam( unsigned int paramNo = 1 );

	bool RegisterShellVerb( const reg::TKeyPath& verbPath, const reg::TKeyPath& modulePath,
							const TCHAR* pVerbTag = nullptr, const TCHAR* pDdeCmd = nullptr, const std::tstring extraParams = str::GetEmpty() );
	bool UnregisterShellVerb( const reg::TKeyPath& verbPath );

	bool RegisterAdditionalDocumentExt( const reg::TKeyPath& docExt, const std::tstring& docTypeId );
}


#endif // ShellRegistryAssoc_h
