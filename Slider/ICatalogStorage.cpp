
#include "pch.h"
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

CComPtr<ICatalogStorage> CCatalogStorageFactory::CreateStorageObject( void )
{
	CComPtr<ICatalogStorage> pNewCatalogStorage;

	CImageCatalogStg::CreateObject( &pNewCatalogStorage );
	return pNewCatalogStorage;
}

bool CCatalogStorageFactory::HasCatalogExt( const TCHAR* pFilePath )
{
	return
		path::MatchExt( pFilePath, s_imageStorageExts[ CatStg_ias ] ) ||
		path::MatchExt( pFilePath, s_imageStorageExts[ CatStg_cid ] ) ||
		path::MatchExt( pFilePath, s_imageStorageExts[ CatStg_icf ] );
}

bool CCatalogStorageFactory::IsVintageCatalog( const TCHAR* pFilePath )
{
	return
		path::MatchExt( pFilePath, s_imageStorageExts[ CatStg_cid ] ) ||
		path::MatchExt( pFilePath, s_imageStorageExts[ CatStg_icf ] );
}

bool CCatalogStorageFactory::HasSameOpenMode( ICatalogStorage* pCatalogStorage, DWORD mode )
{
	return pCatalogStorage->GetDocStorage()->IsOpenForReading() == fs::CStructuredStorage::IsReadingMode( mode );
}

ICatalogStorage* CCatalogStorageFactory::FindStorage( const fs::TStgDocPath& docStgPath ) const
{
	if ( fs::CStructuredStorage* pOpenedStorage = fs::CStructuredStorage::FindOpenedStorage( docStgPath ) )		// opened in testing?
		return checked_static_cast<CImageCatalogStg*>( pOpenedStorage );

	return nullptr;
}

CComPtr<ICatalogStorage> CCatalogStorageFactory::AcquireStorage( const fs::TStgDocPath& docStgPath, DWORD mode /*= STGM_READ*/ )
{
	if ( ICatalogStorage* pFoundCatalogStorage = FindStorage( docStgPath ) )
	{
		ENSURE( HasSameOpenMode( pFoundCatalogStorage, mode ) );
		return pFoundCatalogStorage;			// cache hit
	}

	CComPtr<ICatalogStorage> pNewCatalogStorage = CreateStorageObject();
	fs::CStructuredStorage* pDocStorage = pNewCatalogStorage->GetDocStorage();
	CScopedErrorHandling scopedHandling( pDocStorage, this );		// pass current factory throw mode to the storage

	if ( !pDocStorage->OpenDocFile( docStgPath, mode ) )
		return nullptr;

	if ( fs::CStructuredStorage::IsReadingMode( mode ) )
		if ( !CCatalogPasswordStore::Instance()->LoadPasswordVerify( pNewCatalogStorage ) )
			return nullptr;			// password not verified correctly by user

	return pNewCatalogStorage;
}

std::auto_ptr<CFile> CCatalogStorageFactory::OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode /*= CFile::modeRead*/ )
{
	std::auto_ptr<CFile> pFile;

	if ( !flexImagePath.IsComplexPath() )
		pFile = fs::OpenFile( flexImagePath, IsThrowMode(), mode );			// open physical image file
	else
	{	// storage-based image file:
		fs::TStgDocPath docStgPath = flexImagePath.GetPhysicalPath();

		if ( IsPasswordVerified( docStgPath ) )
			if ( ICatalogStorage* pCatalogStorage = FindStorage( docStgPath ) )
			{
				fs::CStructuredStorage* pDocStorage = pCatalogStorage->GetDocStorage();
				CScopedErrorHandling scopedHandling( pDocStorage, this );					// pass current factory throw mode to the storage

				ASSERT( pDocStorage->IsOpenForWriting() == fs::CStructuredStorage::IsWritingMode( mode ) );		// IMP: the storage must be in compatible open mode for the type of file access

				std::auto_ptr<fs::CStreamLocation> pStreamLocation;

				if ( fs::CStructuredStorage::IsReadingMode( mode ) )
					pStreamLocation = pDocStorage->LocateReadStream( flexImagePath.GetEmbeddedPath(), mode );
				else
					pStreamLocation = pDocStorage->LocateWriteStream( flexImagePath.GetEmbeddedPath(), mode );

				if ( pStreamLocation.get() != nullptr && pStreamLocation->IsValid() )
					pFile.reset( new fs::CManagedOleStreamFile( pStreamLocation, flexImagePath ) );
			}
			else
				ASSERT( false );		// likely a programming error: storage is not alive in the calling context
	}

	return pFile;
}

bool CCatalogStorageFactory::IsPasswordVerified( const fs::TStgDocPath& docStgPath )
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
		fs::TStgDocPath docStgPath = srcImagePath.GetPhysicalPath();

		if ( ICatalogStorage* pCatalogStorage = FindStorage( docStgPath ) )
		{
			CScopedErrorHandling scopedCheck( pCatalogStorage->GetDocStorage(), utl::CheckMode );		// no exceptions for thumb extraction

			return pCatalogStorage->LoadThumbnail( srcImagePath );
		}
	}
	return nullptr;
}

CCachedThumbBitmap* CCatalogStorageFactory::GenerateThumb( const fs::CFlexPath& srcImagePath )
{
	fs::TStgDocPath docStgPath( srcImagePath.GetPhysicalPath() );
	REQUIRE( srcImagePath.IsComplexPath() );

	const CThumbnailer* pThumbnailer = safe_ptr( app::GetThumbnailer() );

	if ( CComPtr<IWICBitmapSource> pBitmapSource = CWicImageCache::Instance().LookupBitmapSource( fs::TImagePathKey( srcImagePath, 0 ) ) )		// use frame 0 for the thumbnail
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

	return nullptr;
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
		fs::stg::CScopedWriteDocMode scopedDocWrite( pCatalogStorage->GetDocStorage(), nullptr );		// switch storage to write/throw mode

		return pCatalogStorage->SavePasswordStream();
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}
}

bool CCatalogPasswordStore::LoadPasswordVerify( ICatalogStorage* pCatalogStorage, std::tstring* pOutPassword /*= nullptr*/ )
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

bool CCatalogPasswordStore::IsPasswordVerified( const fs::TStgDocPath& docStgPath ) const
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


// CMirrorCatalogSave implementation

bool CMirrorCatalogSave::CloseStorage( void )
{
	return m_pStorageHost->Remove( m_docStgPath );
}
