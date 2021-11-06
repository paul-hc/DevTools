#ifndef CatalogStorageHost_h
#define CatalogStorageHost_h
#pragma once

#include "utl/Path.h"
#include "ModelSchema.h"
#include <atlcomcli.h>


interface ICatalogStorage;
enum StorageType { MainStorage, EmbeddedStorage };

// owns multiple image archives, opened for reading initially
class CCatalogStorageHost
{
public:
	CCatalogStorageHost( void );
	~CCatalogStorageHost();

	void Clear( void );
	ICatalogStorage* Push( const fs::TStgDocPath& docStgPath, StorageType stgType );		// open catalog storage for reading
	bool Remove( const fs::TStgDocPath& docStgPath );

	bool IsEmpty( void ) const { return 0 == m_imageStorages.size(); }
	size_t GetCount( void ) const { return m_imageStorages.size(); }
	ICatalogStorage* GetAt( size_t pos ) const;
	ICatalogStorage* Find( const fs::TStgDocPath& docStgPath ) const;

	const fs::TStgDocPath& GetDocFilePathAt( size_t pos ) const;

	template< typename PathContainerT >
	void PushMultiple( const PathContainerT& docStgPaths, StorageType stgType = EmbeddedStorage )
	{
		for ( PathContainerT::const_iterator itDocFilePath = docStgPaths.begin(); itDocFilePath != docStgPaths.end(); ++itDocFilePath )
			Push( *itDocFilePath, stgType );		// loaded an embedded storage?
	}

	template< typename PathContainerT >
	void RemoveMultiple( const PathContainerT& docStgPaths )
	{
		for ( PathContainerT::const_iterator itDocFilePath = docStgPaths.begin(); itDocFilePath != docStgPaths.end(); ++itDocFilePath )
			Remove( *itDocFilePath );
	}

	void ModifyMultiple( const std::vector< fs::TStgDocPath >& newStgPaths, const std::vector< fs::TStgDocPath >& oldStgPaths, StorageType stgType = EmbeddedStorage );
private:
	size_t FindPos( const fs::TStgDocPath& docStgPath ) const;
	static bool RetrofitStreamEncodingSchema( ICatalogStorage* pEmbeddedCatalog );
private:
	std::vector< CComPtr<ICatalogStorage> > m_imageStorages;			// opened for reading
};


#endif // CatalogStorageHost_h
