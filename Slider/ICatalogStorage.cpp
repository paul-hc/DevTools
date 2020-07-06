
#include "stdafx.h"
#include "ICatalogStorage.h"
#include "CatalogStorageHost.h"
#include "ImageCatalogStg.h"
#include "Application.h"
#include "utl/UI/PasswordDialog.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCatalogStorageFactory implementation

const TCHAR* CCatalogStorageFactory::s_imageStorageExts[] = { _T(".ias"), _T(".cid"), _T(".icf") };

CCatalogStorageFactory::~CCatalogStorageFactory()
{
}

CCatalogStorageFactory* CCatalogStorageFactory::Instance( void )
{
	static CCatalogStorageFactory s_factory;
	return &s_factory;
}

CComPtr< ICatalogStorage > CCatalogStorageFactory::CreateStorageObject( void )
{
	CComPtr< ICatalogStorage > pNewCatalogStorage;

	CImageCatalogStg::CreateObject( &pNewCatalogStorage );
	return pNewCatalogStorage;
}

bool CCatalogStorageFactory::HasCatalogExt( const TCHAR* pFilePath )
{
	return
		path::MatchExt( pFilePath, s_imageStorageExts[ Ext_ias ] ) ||
		path::MatchExt( pFilePath, s_imageStorageExts[ Ext_cid ] ) ||
		path::MatchExt( pFilePath, s_imageStorageExts[ Ext_icf ] );
}

ICatalogStorage* CCatalogStorageFactory::FindStorage( const fs::CPath& docStgPath ) const
{
	if ( fs::CStructuredStorage* pOpenedStorage = fs::CStructuredStorage::FindOpenedStorage( docStgPath ) )		// opened in testing?
		return checked_static_cast< CImageCatalogStg* >( pOpenedStorage );

	return NULL;
}

CComPtr< ICatalogStorage > CCatalogStorageFactory::AcquireStorage( const fs::CPath& docStgPath, DWORD mode /*= STGM_READ*/ )
{
	if ( ICatalogStorage* pFoundCatalogStorage = FindStorage( docStgPath ) )
	{
		ASSERT( pFoundCatalogStorage->GetDocStorage()->IsOpenForReading() == fs::CStructuredStorage::IsReadingMode( mode ) );
		return pFoundCatalogStorage;			// cache hit
	}

	CComPtr< ICatalogStorage > pNewCatalogStorage = CreateStorageObject();
	fs::CStructuredStorage* pDocStorage = pNewCatalogStorage->GetDocStorage();
	CScopedErrorHandling scopedHandling( pDocStorage, this );		// pass current factory throw mode to the storage

	if ( !pDocStorage->OpenDocFile( docStgPath, mode ) )
		return NULL;

	if ( fs::CStructuredStorage::IsReadingMode( mode ) )
		if ( !CCatalogPasswordStore::Instance()->LoadPasswordVerify( pNewCatalogStorage ) )
			return NULL;			// password not verified correctly by user

	return pNewCatalogStorage;
}

std::auto_ptr< CFile > CCatalogStorageFactory::OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode /*= CFile::modeRead*/ )
{
	std::auto_ptr< CFile > pFile;

	if ( !flexImagePath.IsComplexPath() )
		pFile = fs::OpenFile( flexImagePath, IsThrowMode(), mode );			// open physical image file
	else
	{	// storage-based image file:
		fs::CPath docStgPath = flexImagePath.GetPhysicalPath();

		if ( IsPasswordVerified( docStgPath ) )
			if ( ICatalogStorage* pCatalogStorage = FindStorage( docStgPath ) )
			{
				fs::CStructuredStorage* pDocStorage = pCatalogStorage->GetDocStorage();
				CScopedErrorHandling scopedHandling( pDocStorage, this );					// pass current factory throw mode to the storage

				ASSERT( pDocStorage->IsOpenForWriting() == fs::CStructuredStorage::IsWritingMode( mode ) );		// IMP: the storage must be in compatible open mode for the type of file access

				std::auto_ptr< fs::CStreamLocation > pStreamLocation;

				if ( fs::CStructuredStorage::IsReadingMode( mode ) )
					pStreamLocation = pDocStorage->LocateReadStream( flexImagePath.GetEmbeddedPath(), mode );
				else
					pStreamLocation = pDocStorage->LocateWriteStream( flexImagePath.GetEmbeddedPath(), mode );

				if ( pStreamLocation.get() != NULL && pStreamLocation->IsValid() )
					pFile.reset( new fs::CManagedOleStreamFile( pStreamLocation, flexImagePath ) );
			}
			else
				ASSERT( false );		// likely a programming error: storage is not alive in the calling context
	}

	return pFile;
}

bool CCatalogStorageFactory::IsPasswordVerified( const fs::CPath& docStgPath )
{
	return CCatalogPasswordStore::Instance()->IsPasswordVerified( docStgPath );
}

bool CCatalogStorageFactory::ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const
{
	return srcImagePath.IsComplexPath();
}

CCachedThumbBitmap* CCatalogStorageFactory::ExtractThumb( const fs::CFlexPath& srcImagePath )
{
	if ( srcImagePath.IsComplexPath() )
	{
		fs::CPath docStgPath = srcImagePath.GetPhysicalPath();

		if ( ICatalogStorage* pCatalogStorage = FindStorage( docStgPath ) )
		{
			CScopedErrorHandling scopedCheck( pCatalogStorage->GetDocStorage(), utl::CheckMode );		// no exceptions for thumb extraction

			return pCatalogStorage->LoadThumbnail( srcImagePath );
		}
	}
	return NULL;
}

CCachedThumbBitmap* CCatalogStorageFactory::GenerateThumb( const fs::CFlexPath& srcImagePath )
{
	fs::CPath docStgPath( srcImagePath.GetPhysicalPath() );
	REQUIRE( srcImagePath.IsComplexPath() );

	const CThumbnailer* pThumbnailer = safe_ptr( app::GetThumbnailer() );

	if ( CComPtr< IWICBitmapSource > pBitmapSource = CWicImageCache::Instance().LookupBitmapSource( fs::ImagePathKey( srcImagePath, 0 ) ) )		// use frame 0 for the thumbnail
		if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->NewScaledThumb( pBitmapSource, srcImagePath ) )
		{
			// IMP: the resulting bitmap, even the scaled bitmap will keep the stream alive for the lifetime of the bitmap; same if we scale the bitmap;
			// Since the thumb keeps alive the WIC bitmap, we need to copy to a memory bitmap.
			// This way we unlock the doc stg stream for future access.

			//TRACE_COM_PTR( pBitmapSource, "BEFORE DetachSourceToBitmap() in CCatalogStorageFactory::GenerateThumb()" );
			pThumbBitmap->GetOrigin().DetachSourceToBitmap();		// release any bitmap source dependencies (IStream, HFILE, etc)
			//TRACE_COM_PTR( pBitmapSource, "AFTER DetachSourceToBitmap()" );
			return pThumbBitmap;
		}

	return NULL;
}


// CCatalogPasswordStore implementation

CCatalogPasswordStore* CCatalogPasswordStore::Instance( void )
{
	static CCatalogPasswordStore s_store;
	return &s_store;
}

bool CCatalogPasswordStore::SavePassword( ICatalogStorage* pCatalogStorage )
{
	ASSERT_PTR( pCatalogStorage );

	try
	{
		fs::stg::CScopedWriteDocMode scopedDocWrite( pCatalogStorage->GetDocStorage(), NULL );		// switch storage to write/throw mode

		return pCatalogStorage->SavePasswordStream();
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}
}

bool CCatalogPasswordStore::LoadPasswordVerify( ICatalogStorage* pCatalogStorage, std::tstring* pOutPassword /*= NULL*/ )
{
	ASSERT_PTR( pCatalogStorage );

	if ( !pCatalogStorage->LoadPasswordStream() )
		return false;

	const std::tstring& password = pCatalogStorage->GetPassword();
	if ( !password.empty() )
		if ( m_verifiedPasswords.find( password ) == m_verifiedPasswords.end() )
		{
			CPasswordDialog dlg( AfxGetMainWnd(), &pCatalogStorage->GetDocStorage()->GetDocFilePath() );
			dlg.SetVerifyPassword( password );
			if ( dlg.DoModal() != IDOK )
				return false;

			m_verifiedPasswords.insert( password );
		}

	utl::AssignPtr( pOutPassword, password );
	return true;
}

bool CCatalogPasswordStore::CacheVerifiedPassword( const std::tstring& password )
{
	ASSERT( !password.empty() );
	return m_verifiedPasswords.insert( password ).second;
}

bool CCatalogPasswordStore::IsPasswordVerified( const fs::CPath& docStgPath ) const
{
	if ( ICatalogStorage* pCatalogStorage = CCatalogStorageFactory::Instance()->FindStorage( docStgPath ) )
		if ( pCatalogStorage->GetPassword().empty() )
			return true;		// not password protected?
		else if ( m_verifiedPasswords.find( pCatalogStorage->GetPassword() ) != m_verifiedPasswords.end() )
			return true;		// password already validated by user
		else
			return false;

	return false;				// assume not verified on closed storage
}


// CCatalogStorageHost implementation

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

ICatalogStorage* CCatalogStorageHost::Push( const fs::CPath& docStgPath, DWORD mode /*= STGM_READ*/ )
{
	ASSERT_NULL( Find( docStgPath ) );			// add only once

	CComPtr< ICatalogStorage > pStorage = CCatalogStorageFactory::Instance()->AcquireStorage( docStgPath, mode );
	if ( pStorage != NULL )
	{
		REQUIRE( pStorage->GetDocStorage()->IsOpenForReading() == fs::CStructuredStorage::IsReadingMode( mode ) );

		m_imageStorages.push_back( pStorage );
	}

	return pStorage;
}

bool CCatalogStorageHost::Remove( const fs::CPath& docStgPath )
{
	size_t foundPos = FindPos( docStgPath );
	if ( utl::npos == foundPos )
		return false;

	m_imageStorages.erase( m_imageStorages.begin() + foundPos );
	return true;
}

void CCatalogStorageHost::ModifyMultiple( const std::vector< fs::CPath >& newStgPaths, const std::vector< fs::CPath >& oldStgPaths,
										  DWORD mode /*= STGM_READ*/ )
{
	// remove & close old storages that are no longer part of the new ones:
	for ( std::vector< fs::CPath >::const_iterator itOldStgPath = oldStgPaths.begin(); itOldStgPath != oldStgPaths.end(); ++itOldStgPath )
		if ( utl::FindPos( newStgPaths.begin(), newStgPaths.end(), *itOldStgPath ) != utl::npos )
			Remove( *itOldStgPath );

	// push & open new storages:
	for ( std::vector< fs::CPath >::const_iterator itNewStgPath = newStgPaths.begin(); itNewStgPath != newStgPaths.end(); ++itNewStgPath )
		if ( ICatalogStorage* pFoundCatalogStorage = Find( *itNewStgPath ) )		// found already open?
			ASSERT( pFoundCatalogStorage->GetDocStorage()->IsOpenForReading() );
		else
			Push( *itNewStgPath, mode );
}

ICatalogStorage* CCatalogStorageHost::GetAt( size_t pos ) const
{
	ASSERT( pos < GetCount() );
	return m_imageStorages[ pos ];
}

const fs::CPath& CCatalogStorageHost::GetDocFilePathAt( size_t pos ) const
{
	return GetAt( pos )->GetDocStorage()->GetDocFilePath();
}

ICatalogStorage* CCatalogStorageHost::Find( const fs::CPath& docStgPath ) const
{
	size_t foundPos = FindPos( docStgPath );
	return foundPos != utl::npos ? GetAt( foundPos ) : NULL;
}

size_t CCatalogStorageHost::FindPos( const fs::CPath& docStgPath ) const
{
	REQUIRE( CCatalogStorageFactory::HasCatalogExt( docStgPath.GetPtr() ) );

	for ( size_t pos = 0; pos != m_imageStorages.size(); ++pos )
		if ( docStgPath == GetDocFilePathAt( pos ) )
			return pos;

	return utl::npos;
}


// CMirrorCatalogSave implementation

bool CMirrorCatalogSave::CloseStorage( void )
{
	return m_pStorageHost->Remove( m_docStgPath );
}
