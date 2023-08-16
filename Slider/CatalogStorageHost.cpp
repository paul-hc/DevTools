
#include "pch.h"
#include "CatalogStorageHost.h"
#include "ICatalogStorage.h"
#include "utl/Algorithms.h"
#include "utl/UI/ImagingWic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCatalogStorageHost::CCatalogStorageHost( void )
{
}

CCatalogStorageHost::~CCatalogStorageHost()
{
	Clear();
}

void CCatalogStorageHost::Clear( void )
{
	m_imageStorages.clear();
}

ICatalogStorage* CCatalogStorageHost::Push( const fs::TStgDocPath& docStgPath, StorageType stgType )
{
	ASSERT_NULL( Find( docStgPath ) );			// add only once

	CComPtr<ICatalogStorage> pCatalogStorage = CCatalogStorageFactory::Instance()->AcquireStorage( docStgPath, STGM_READ );
	if ( pCatalogStorage != nullptr )
	{
		ENSURE( pCatalogStorage->GetDocStorage()->IsOpenForReading() );
		m_imageStorages.push_back( pCatalogStorage );

		if ( EmbeddedStorage == stgType )
			RetrofitStreamEncodingSchema( pCatalogStorage );
	}

	return pCatalogStorage;
}

bool CCatalogStorageHost::Remove( const fs::TStgDocPath& docStgPath )
{
	size_t foundPos = FindPos( docStgPath );
	if ( utl::npos == foundPos )
		return false;

	m_imageStorages.erase( m_imageStorages.begin() + foundPos );
	return true;
}

void CCatalogStorageHost::ModifyMultiple( const std::vector<fs::TStgDocPath>& newStgPaths, const std::vector<fs::TStgDocPath>& oldStgPaths,
										  StorageType stgType /*= EmbeddedStorage*/ )
{
	// remove & close old storages that are no longer part of the new ones:
	for ( std::vector<fs::TStgDocPath>::const_iterator itOldStgPath = oldStgPaths.begin(); itOldStgPath != oldStgPaths.end(); ++itOldStgPath )
		if ( utl::FindPos( newStgPaths.begin(), newStgPaths.end(), *itOldStgPath ) != utl::npos )
			Remove( *itOldStgPath );

	// push & open new storages:
	for ( std::vector<fs::TStgDocPath>::const_iterator itNewStgPath = newStgPaths.begin(); itNewStgPath != newStgPaths.end(); ++itNewStgPath )
		if ( ICatalogStorage* pFoundCatalogStorage = Find( *itNewStgPath ) )		// found already open?
			ASSERT( pFoundCatalogStorage->GetDocStorage()->IsOpenForReading() );
		else
			Push( *itNewStgPath, stgType );
}

ICatalogStorage* CCatalogStorageHost::GetAt( size_t pos ) const
{
	ASSERT( pos < GetCount() );
	return m_imageStorages[ pos ];
}

const fs::TStgDocPath& CCatalogStorageHost::GetDocFilePathAt( size_t pos ) const
{
	return GetAt( pos )->GetDocStorage()->GetDocFilePath();
}

ICatalogStorage* CCatalogStorageHost::Find( const fs::TStgDocPath& docStgPath ) const
{
	size_t foundPos = FindPos( docStgPath );
	return foundPos != utl::npos ? GetAt( foundPos ) : nullptr;
}

size_t CCatalogStorageHost::FindPos( const fs::TStgDocPath& docStgPath ) const
{
	REQUIRE( CCatalogStorageFactory::HasCatalogExt( docStgPath.GetPtr() ) );

	for ( size_t pos = 0; pos != m_imageStorages.size(); ++pos )
		if ( docStgPath == GetDocFilePathAt( pos ) )
			return pos;

	return utl::npos;
}

namespace pred
{
	static bool IsDeepStream( const fs::TEmbeddedPath& elementPath, const STATSTG& stgStat )
	{
		return
			STGTY_STREAM == stgStat.type &&
			wic::IsValidFileImageFormat( elementPath.GetPtr() ) &&
			elementPath.Get().find_first_of( _T("*|") ) != std::tstring::npos;
	}
}

bool CCatalogStorageHost::RetrofitStreamEncodingSchema( ICatalogStorage* pEmbeddedCatalog )
{
	ASSERT_PTR( pEmbeddedCatalog );
	// backwards compatibility check: when referring an embedded storage in .sld albums, the original m_docModelSchema gets lost.
	// Try to retrofit the ModelSchema if deep streams use the old '*' separator.

	if ( fs::CStructuredStorage* pStorage = pEmbeddedCatalog->GetDocStorage() )
		if ( app::Slider_LatestModelSchema == pEmbeddedCatalog->GetDocModelSchema() && pStorage->IsOpenForReading() )
		{
			fs::CStructuredStorage::CScopedCurrentDir scopedRoot( pStorage );
			fs::TEmbeddedPath deepStreamPath;

			if ( pStorage->FindFirstElementThat( deepStreamPath, &pred::IsDeepStream, Shallow ) )		// find the first stream that satisfies the predicate
			{
				if ( deepStreamPath.Get().find( _T('*') ) != std::tstring::npos )
					pEmbeddedCatalog->StoreDocModelSchema( app::Slider_v5_1 );

				return true;		// found a deep stream path
			}
		}

	return false;
}
