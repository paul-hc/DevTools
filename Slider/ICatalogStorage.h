#ifndef ICatalogStorage_h
#define ICatalogStorage_h
#pragma once

#include "utl/ErrorHandler.h"
#include "utl/FlexPath.h"
#include "utl/StructuredStorage.h"
#include "ModelSchema.h"
#include <hash_set>


class CCatalogStorageService;
class CImagesModel;
class CCachedThumbBitmap;


EXTERN_C const IID IID_ICatalogStorage;


MIDL_INTERFACE("C5078A69-FF1C-4B13-9202-1EEF0532258D")
ICatalogStorage : public IUnknown
{
public:
	virtual fs::CStructuredStorage* GetDocStorage( void ) = 0;

	virtual void StoreDocModelSchema( app::ModelSchema docModelSchema ) = 0;

	virtual const std::tstring& GetPassword( void ) const = 0;
	virtual void StorePassword( const std::tstring& password ) = 0;

	virtual void CreateImageArchiveFile( const fs::CPath& docStgPath, CCatalogStorageService* pCatalogSvc ) throws_( CException* ) = 0;

	virtual bool SavePasswordStream( void ) = 0;
	virtual bool LoadPasswordStream( void ) = 0;

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

	static CComPtr< ICatalogStorage > CreateStorageObject( void );

	static bool HasCatalogExt( const TCHAR* pFilePath );
	static const TCHAR* GetDefaultExtension( void ) { return s_imageStorageExts[ Ext_ias ]; }

	ICatalogStorage* FindStorage( const fs::CPath& docStgPath ) const;
	CComPtr< ICatalogStorage > AcquireStorage( const fs::CPath& docStgPath, DWORD mode = STGM_READ );

	std::auto_ptr< CFile > OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode = CFile::modeRead );		// either physical or storage-based image file

	// fs::IThumbProducer interface
	virtual bool ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const;
	virtual CCachedThumbBitmap* ExtractThumb( const fs::CFlexPath& srcImagePath );
	virtual CCachedThumbBitmap* GenerateThumb( const fs::CFlexPath& srcImagePath );
private:
	static bool IsPasswordVerified( const fs::CPath& docStgPath );
private:
	enum ExtensionType { Ext_ias, Ext_cid, Ext_icf };

	static const TCHAR* s_imageStorageExts[];			// file extension for compound-files (".icf")
};


class CCatalogPasswordStore : private utl::noncopyable
{
	CCatalogPasswordStore( void ) {}
public:
	static CCatalogPasswordStore* Instance( void );

	bool SavePassword( ICatalogStorage* pCatalogStorage );
	bool LoadPasswordVerify( std::tstring* pOutPassword, ICatalogStorage* pCatalogStorage );
	bool CacheVerifiedPassword( const std::tstring& password );

	bool IsPasswordVerified( const fs::CPath& docStgPath ) const;
private:
	stdext::hash_set< std::tstring > m_verifiedPasswords;
};


#endif // ICatalogStorage_h
