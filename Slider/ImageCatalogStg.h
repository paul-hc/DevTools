#ifndef ImageCatalogStg_h
#define ImageCatalogStg_h
#pragma once

#include "utl/FlexPath.h"
#include "utl/IUnknownImpl.h"
#include "utl/StructuredStorage.h"
#include "ICatalogStorage.h"
#include <set>
#include <hash_map>


namespace wic { enum ImageFormat; }


// Compound file storage with password protection for .ias, .cid and .icf structured document files.
// Contains the image files, sub-storage with thumbnail files, encrypted password file.
// Maintains deep image stream paths from the source images (relative to SRC common root path).
// Deep image stream paths are flattened using '|' as separator (formerly '*') - this avoids the use of sub-storages for images and thumbnails.
class CImageCatalogStg : public fs::CStructuredStorage
					   , public utl::IUnknownImpl< ICatalogStorage >			// the public gateway for manipulating image archives
{
	CImageCatalogStg( void );
	virtual ~CImageCatalogStg();
public:
	// base overrides
	virtual void CloseDocFile( void );

	static void CreateObject( ICatalogStorage** ppCatalogStorage );
	static size_t DiscardCachedImages( const fs::CPath& docStgPath );		// to avoid sharing violations on stream access
private:
	// ICatalogStorage interface
	virtual fs::CStructuredStorage* GetDocStorage( void );

	virtual app::ModelSchema GetDocModelSchema( void ) const;
	virtual void StoreDocModelSchema( app::ModelSchema docModelSchema );

	virtual const std::tstring& GetPassword( void ) const { return m_password; }
	virtual void StorePassword( const std::tstring& password );

	virtual void CreateImageArchiveFile( const fs::CPath& docStgPath, CCatalogStorageService* pCatalogSvc ) throws_( CException* );

	virtual bool SavePasswordStream( void );
	virtual bool LoadPasswordStream( void );

	virtual bool LoadAlbumMap( std::tstring* pAlbumMapText );				// pass NULL to find out if the catalog has an "_AlbumMap.txt" stream

	virtual bool SaveAlbumStream( CObject* pAlbumDoc );		// "_Album.sld" stream (with .sld file format)
	virtual bool LoadAlbumStream( CObject* pAlbumDoc );
	virtual bool EnumerateImages( CImagesModel& rImagesModel );

	virtual CCachedThumbBitmap* LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_();		// caller must delete the image
protected:
	// base overrides
	virtual TCHAR GetFlattenPathSep( void ) const;
	virtual bool RetainOpenedStream( const fs::TEmbeddedPath& streamPath ) const;
private:
	void CreateImageFiles( CCatalogStorageService* pCatalogSvc ) throws_( CException* );
	void CreateThumbnailsSubStorage( const CCatalogStorageService* pCatalogSvc ) throws_( CException* );

	CComPtr< IStream > OpenThumbnailImageStream( const TCHAR* pImageEmbeddedPath );
	static wic::ImageFormat MakeThumbStreamName( fs::TEmbeddedPath& rThumbStreamName, const TCHAR* pSrcImagePath );
	static bool IsSpecialStreamName( const TCHAR* pStreamName );

	bool DeleteOldVersionStream( const TCHAR* pStreamName );
	bool DeleteAnyOldVersionStream( const TCHAR* altStreamNames[], size_t altCount );

	// backwards compatibility
	bool bkw_LoadAlbumMetadataStream( CObject* pAlbumDoc );
	void bkw_DecodeStreamPath( std::tstring& rStreamPath );
	bool bkw_AlterOlderDocModelSchema( app::ModelSchema docModelSchema );
private:
	app::ModelSchema m_docModelSchema;			// transient: loaded model schema from file, stored by the album doc
	std::tstring m_password;					// allow password edititng of any document (including .sld), in preparation for SaveAs .ias
	utl::Ternary m_hasAlbumMap;
private:
	enum Alternates { CurrentVer };				// first entry in an array of alternates

	static const TCHAR* s_pAlbumFolderName;				// "Album" - album sub-storage name
	static const TCHAR* s_thumbsFolderNames[];			// "Thumbnails" - thumbs sub-storage name (with old variants)

	static const TCHAR* s_pAlbumStreamName;				// "_Album.sld" - album info stream (superset of meta-data)
	static const TCHAR* s_pAlbumMapStreamName;			// "_AlbumMap.txt" - meta-data stream in a compound-file (obsolete)
	static const TCHAR* s_passwordStreamNames[];		// "_pwd.w" - enchrypted password stream (with old variants)
private:
	class CAlbumMapWriter : public fs::CTextFileWriter
	{
	public:
		CAlbumMapWriter( std::auto_ptr< COleStreamFile > pAlbumMapFile );

		void WriteHeader( void );
		void WriteEntry( const std::tstring& streamName, const TCHAR* pImageEmbeddedPath );
		void WriteImageTotals( UINT64 totalImagesSize );
		void WriteThumbnailTotals( size_t thumbCount, UINT64 totalThumbsSize );
	private:
		enum { StreamNoPadding = 5, StreamNamePadding = 34 };

		std::auto_ptr< COleStreamFile > m_pAlbumMapFile;
		size_t m_imageCount;
		static const TCHAR s_lineFmt[];
	};
};


#endif // ImageCatalogStg_h
