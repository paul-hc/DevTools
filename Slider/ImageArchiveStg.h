#ifndef ImageArchiveStg_h
#define ImageArchiveStg_h
#pragma once

#include "utl/FlexPath.h"
#include "utl/StructuredStorage.h"
#include "utl/UI/Thumbnailer_fwd.h"
#include "ArchivingModel_fwd.h"
#include "ModelSchema.h"
#include <set>
#include <hash_map>


class CFileAttr;
class CImageStorageService;
class CTransferFileAttr;
class CCachedThumbBitmap;
namespace wic { enum ImageFormat; }


// Compound file storage with password protection for .ias, .cid and .icf structured document files.
// Contains the image files, sub-storage with thumbnail files, encrypted password file.
// Maintains deep image stream paths from the source images (relative to SRC common root path).
// Deep image stream paths are flattened using '|' as separator (formerly '*') - this avoids the use of sub-storages for images and thumbnails.

class CImageArchiveStg : public fs::CStructuredStorage
{
public:
	CImageArchiveStg( void );
	virtual ~CImageArchiveStg();

	// operations
	void Close( void );
	static void DiscardCachedImages( const fs::CPath& stgFilePath );		// to avoid sharing violations on stream access

	void CreateImageArchive( const fs::CPath& stgFilePath, CImageStorageService* pImagesSvc ) throws_( CException* );

	bool SavePassword( const std::tstring& password );
	std::tstring LoadPassword( void );

	void StoreDocModelSchema( app::ModelSchema docModelSchema ) { m_docModelSchema = docModelSchema; }

	// "_Album.sld" stream (with .sld file format)
	void SaveAlbumDoc( CObject* pAlbumDoc );
	bool LoadAlbumDoc( CObject* pAlbumDoc );

	CCachedThumbBitmap* LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_();		// caller must delete the image

	static bool HasImageArchiveExt( const TCHAR* pFilePath );
	static const TCHAR* GetDefaultExtension( void ) { return s_compoundStgExts[ Ext_ias ]; }
protected:
	// base overrides
	virtual std::tstring EncodeStreamName( const TCHAR* pStreamName ) const;
	virtual bool RetainOpenedStream( const TCHAR* pStreamName, IStorage* pParentDir ) const;

	TCHAR GetSubPathSep( void ) const;

	void FlattenDeepStreamPath( std::tstring& rDeepPath ) const { std::replace( rDeepPath.begin(), rDeepPath.end(), _T('\\'), GetSubPathSep() ); }			// '\' -> '|'
	void UnflattenDeepStreamPath( std::tstring& rDeepPath ) const { std::replace( rDeepPath.begin(), rDeepPath.end(), GetSubPathSep(), _T('\\') ); }		// '|' or '*' -> '\'
private:
	void CreateImageFiles( CImageStorageService* pImagesSvc ) throws_( CException* );
	void CreateThumbnailsSubStorage( const CImageStorageService* pImagesSvc ) throws_( CException* );

	CComPtr< IStream > OpenThumbnailImageStream( const TCHAR* pImageEmbeddedPath );
	static wic::ImageFormat MakeThumbStreamName( fs::TEmbeddedPath& rThumbStreamName, const TCHAR* pSrcImagePath );
	static bool IsSpecialStreamName( const TCHAR* pStreamName );
private:
	CComPtr< IStorage > m_pThumbsStorage;
	app::ModelSchema m_docModelSchema;			// transient: loaded model schema from file, stored by the album doc
private:
	enum Alternates { CurrentVer };
	enum ExtensionType { Ext_ias, Ext_cid, Ext_icf };

	static const TCHAR* s_compoundStgExts[];			// file extension for compound-files (".icf")
	static const TCHAR* s_passwordStreamNames[];		// "_pwd.w" - enchrypted password stream (with old variants)
	static const TCHAR* s_thumbsStorageNames[];			// "Thumbnails" - thumbs sub-storage name (with old variants)
	static const TCHAR s_albumStreamName[];				// "_Album.sld" - album info stream (superset of meta-data)
	static const TCHAR s_albumMapStreamName[];			// "_AlbumMap.txt" - meta-data stream in a compound-file (obsolete)
	static const TCHAR s_subPathSep;					// sub-path separator for deep stream names
public:
	class CFactory : public CErrorHandler
				   , public fs::IThumbProducer
				   , private utl::noncopyable
	{
	public:
		CFactory( void ) : CErrorHandler( utl::CheckMode ) {}
		~CFactory() { Clear(); }							// factory singleton must be cleared on ExitInstance (not later)

		void Clear( void );

		CImageArchiveStg* FindStorage( const fs::CPath& stgFilePath ) const;
		CImageArchiveStg* AcquireStorage( const fs::CPath& stgFilePath, DWORD mode = STGM_READ );
		bool ReleaseStorage( const fs::CPath& stgFilePath );
		void ReleaseStorages( const std::vector< fs::CPath >& stgFilePaths );

		// FLEX: image path could refer to either physical image or archive-based image file
		std::auto_ptr< CFile > OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode = CFile::modeRead );

		bool SavePassword( const std::tstring& password, const fs::CPath& stgFilePath );
		std::tstring LoadPassword( const fs::CPath& stgFilePath );
		bool CacheVerifiedPassword( const fs::CPath& stgFilePath, const std::tstring& password );		// for doc SaveAs
		bool VerifyPassword( std::tstring* pOutPassword, const fs::CPath& stgFilePath );

		bool SaveAlbumDoc( CObject* pAlbumDoc, const fs::CPath& stgFilePath );
		bool LoadAlbumDoc( CObject* pAlbumDoc, const fs::CPath& stgFilePath );

		// fs::IThumbProducer interface
		virtual bool ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const;
		virtual CCachedThumbBitmap* ExtractThumb( const fs::CFlexPath& srcImagePath );
		virtual CCachedThumbBitmap* GenerateThumb( const fs::CFlexPath& srcImagePath );
	private:
		bool IsPasswordVerified( const fs::CPath& stgFilePath ) const;
	private:
		stdext::hash_map< fs::CPath, CImageArchiveStg* > m_storageMap;		// factory-managed (with ownership) archives
		stdext::hash_map< fs::CPath, std::tstring > m_passwordProtected;	// known password-protected storages to encrypted passwords
		std::set< std::tstring > m_verifiedPasswords;
	};

	static CFactory* Factory( void );
private:
	enum { WriteableModeMask = STGM_CREATE | STGM_WRITE | STGM_READWRITE, NotOpenMask = -1 };

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

	// used mostly to get temporary writeable access to an image storage
	//
	class CScopedAcquireStorage
	{
	public:
		CScopedAcquireStorage( const fs::CPath& stgFilePath, DWORD mode );
		~CScopedAcquireStorage();

		CImageArchiveStg* Get( void ) const { return m_pArchiveStg; }
	private:
		fs::CPath m_stgFilePath;
		CImageArchiveStg* m_pArchiveStg;
		bool m_mustRelease;
		DWORD m_oldOpenMode;
	};
};


#endif // ImageArchiveStg_h
