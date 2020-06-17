#ifndef IImageArchiveStg_h
#define IImageArchiveStg_h
#pragma once

#include "ModelSchema.h"


namespace fs { class CStructuredStorage; }

class CImageStorageService;
class CCachedThumbBitmap;


EXTERN_C const IID IID_IImageArchiveStg;


MIDL_INTERFACE("C5078A69-FF1C-4b13-9202-1EEF0532258D")
IImageArchiveStg : public IUnknown
{
public:
	virtual fs::CStructuredStorage* GetDocStorage( void ) = 0;

	virtual void StoreDocModelSchema( app::ModelSchema docModelSchema ) = 0;

	virtual void CreateImageArchive( const fs::CPath& stgFilePath, CImageStorageService* pImagesSvc ) throws_( CException* ) = 0;

	virtual bool SavePassword( const std::tstring& password ) = 0;
	virtual std::tstring LoadPassword( void ) = 0;

	// "_Album.sld" stream (with .sld file format)
	virtual void SaveAlbumDoc( CObject* pAlbumDoc ) = 0;
	virtual bool LoadAlbumDoc( CObject* pAlbumDoc ) = 0;

	virtual CCachedThumbBitmap* LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_() = 0;		// caller must delete the image
};


#endif // IImageArchiveStg_h
