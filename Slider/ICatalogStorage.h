#ifndef ICatalogStorage_h
#define ICatalogStorage_h
#pragma once

#include "utl/ErrorHandler.h"
#include "utl/FlexPath.h"
#include "utl/StructuredStorage.h"
#include "ModelSchema.h"
#include <unordered_set>


class CCatalogStorageService;
class CImagesModel;
class CCachedThumbBitmap;


EXTERN_C const IID IID_ICatalogStorage;


MIDL_INTERFACE("C5078A69-FF1C-4B13-9202-1EEF0532258D")
ICatalogStorage : public IUnknown
{
public:
	virtual fs::CStructuredStorage* GetDocStorage( void ) = 0;

	virtual app::ModelSchema GetDocModelSchema( void ) const = 0;
	virtual void StoreDocModelSchema( app::ModelSchema docModelSchema ) = 0;

	virtual const std::tstring& GetPassword( void ) const = 0;
	virtual void StorePassword( const std::tstring& password ) = 0;

	virtual void CreateImageArchiveFile( const fs::TStgDocPath& docStgPath, CCatalogStorageService* pCatalogSvc ) throws_( CException* ) = 0;

	virtual bool SavePasswordStream( void ) = 0;
	virtual bool LoadPasswordStream( void ) = 0;

	virtual bool LoadAlbumMap( std::tstring* pAlbumMapText ) = 0;

	// "_Album.sld" stream (with .sld file format)
	virtual bool SaveAlbumStream( CObject* pAlbumDoc ) = 0;
	virtual bool LoadAlbumStream( CObject* pAlbumDoc ) = 0;
	virtual bool EnumerateImages( CImagesModel& rImagesModel ) = 0;

	virtual CCachedThumbBitmap* LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_() = 0;		// caller must delete the image
};


#include "utl/UI/Thumbnailer_fwd.h"


class CCatalogStorageFactory : public CErrorHandler
							 , public fs::IThumbProducer
							 , private utl::noncopyable
{
	CCatalogStorageFactory( void ) : CErrorHandler( utl::CheckMode ) {}
	~CCatalogStorageFactory();
public:
	static CCatalogStorageFactory* Instance( void );

	static CComPtr<ICatalogStorage> CreateStorageObject( void );

	static bool HasCatalogExt( const TCHAR* pFilePath );
	static bool IsVintageCatalog( const TCHAR* pFilePath );
	static const TCHAR* GetDefaultExtension( void ) { return s_imageStorageExts[ CatStg_ias ]; }

	ICatalogStorage* FindStorage( const fs::TStgDocPath& docStgPath ) const;
	CComPtr<ICatalogStorage> AcquireStorage( const fs::TStgDocPath& docStgPath, DWORD mode = STGM_READ );
		// for password-protected storage reading: also prompts user to verify password, returning NULL if not verified

	std::auto_ptr<CFile> OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode = CFile::modeRead );		// either physical or storage-based image file

	// fs::IThumbProducer interface
	virtual bool ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const;
	virtual CCachedThumbBitmap* ExtractThumb( const fs::CFlexPath& srcImagePath );
	virtual CCachedThumbBitmap* GenerateThumb( const fs::CFlexPath& srcImagePath );
private:
	static bool IsPasswordVerified( const fs::TStgDocPath& docStgPath );
	static bool HasSameOpenMode( ICatalogStorage* pCatalogStorage, DWORD mode );
private:
	enum ExtensionType { CatStg_ias, CatStg_cid, CatStg_icf };

	static const TCHAR* s_imageStorageExts[];			// file extension for compound-files (".icf")
};


class CCatalogPasswordStore : private utl::noncopyable
{
	CCatalogPasswordStore( void ) {}
public:
	static CCatalogPasswordStore* Instance( void );

	bool SavePassword( ICatalogStorage* pCatalogStorage );
	bool LoadPasswordVerify( ICatalogStorage* pCatalogStorage, std::tstring* pOutPassword = NULL );
	bool CacheVerifiedPassword( const std::tstring& password );

	bool IsPasswordVerified( const fs::TStgDocPath& docStgPath ) const;
private:
	std::unordered_set<std::tstring> m_verifiedPasswords;
};


class CCatalogStorageHost;


class CMirrorCatalogSave : public fs::stg::CMirrorStorageSave
{
public:
	CMirrorCatalogSave( const fs::TStgDocPath& docStgPath, const fs::TStgDocPath& oldDocStgPath, CCatalogStorageHost* pStorageHost )
		: fs::stg::CMirrorStorageSave( docStgPath, oldDocStgPath )
		, m_pStorageHost( pStorageHost )
	{
		ASSERT_PTR( m_pStorageHost );
	}
protected:
	virtual bool CloseStorage( void );
private:
	CCatalogStorageHost* m_pStorageHost;
};


#endif // ICatalogStorage_h
