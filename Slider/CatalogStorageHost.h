#ifndef CatalogStorageHost_h
#define CatalogStorageHost_h
#pragma once

#include "utl/Path.h"
#include <atlcomcli.h>


interface ICatalogStorage;


// owns multiple image archives, opened for reading initially
class CCatalogStorageHost
{
public:
	CCatalogStorageHost( void );
	~CCatalogStorageHost();

	void Clear( void );
	ICatalogStorage* Push( const fs::CPath& docStgPath, DWORD mode = STGM_READ );		// open image storage for reading
	bool Remove( const fs::CPath& docStgPath );

	bool IsEmpty( void ) const { return 0 == m_imageStorages.size(); }
	size_t GetCount( void ) const { return m_imageStorages.size(); }
	ICatalogStorage* GetAt( size_t pos ) const;
	ICatalogStorage* Find( const fs::CPath& docStgPath ) const;

	const fs::CPath& GetDocFilePathAt( size_t pos ) const;

	template< typename PathContainerT >
	void PushMultiple( const PathContainerT& docStgPaths, DWORD mode = STGM_READ )
	{
		for ( PathContainerT::const_iterator itDocFilePath = docStgPaths.begin(); itDocFilePath != docStgPaths.end(); ++itDocFilePath )
			Push( *itDocFilePath, mode );
	}

	template< typename PathContainerT >
	void RemoveMultiple( const PathContainerT& docStgPaths )
	{
		for ( PathContainerT::const_iterator itDocFilePath = docStgPaths.begin(); itDocFilePath != docStgPaths.end(); ++itDocFilePath )
			Remove( *itDocFilePath );
	}

	void ModifyMultiple( const std::vector< fs::CPath >& newStgPaths, const std::vector< fs::CPath >& oldStgPaths, DWORD mode = STGM_READ );
private:
	size_t FindPos( const fs::CPath& docStgPath ) const;
private:
	std::vector< CComPtr< ICatalogStorage > > m_imageStorages;			// opened for reading
};


#endif // CatalogStorageHost_h
