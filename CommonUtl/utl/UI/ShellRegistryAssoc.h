#ifndef ShellRegistryAssoc_h
#define ShellRegistryAssoc_h
#pragma once

#include "Path.h"


namespace shell
{
	// registry shell association helpers for HKEY_CLASSES_ROOT hive sub-keys

	fs::CPath MakeShellHandlerVerbPath( const TCHAR handlerName[], const TCHAR verb[] );		// "<handlerName>\\shell\\<verb>"
	bool QueryHandlerName( std::tstring& rHandlerName, const TCHAR ext[] );						// for ".txt" returns "Notepad++_file", for "SliderAlbum.Document" returns "Slider Album Document"

	const TCHAR* GetDdeOpenCmd( void );
	std::tstring MakeParam( unsigned int paramNo = 1 );

	bool RegisterShellVerb( const fs::CPath& verbPath, const fs::CPath& modulePath,
							const TCHAR* pVerbTag = NULL, const TCHAR* pDdeCmd = NULL, const std::tstring extraParams = str::GetEmpty() );
	bool UnregisterShellVerb( const fs::CPath& verbPath );

	bool RegisterAdditionalDocumentExt( const std::tstring& docTypeId, const fs::CPath& docExt );
}


#endif // ShellRegistryAssoc_h
