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
class CTransferFileAttr;
class CCachedThumbBitmap;
namespace ui { interface IProgressService; }


// compound file storage with password protection for .ias, .cid and .icf structured document files
// contains image files, files metadata, substorage with thumbnail files, encrypted password file

class CImageArchiveStg : public fs::CStructuredStorage
{
public:
	CImageArchiveStg( IStorage* pRootStorage = NULL );
	virtual ~CImageArchiveStg();

	// operations
	void Close( void );
	static void DiscardCachedImages( const fs::CPath& stgFilePath );		// to avoid sharing violations on stream access

	void CreateImageArchive( const fs::CPath& stgFilePath, const std::tstring& password, const std::vector< TTransferPathPair >& xferPairs,
							 ui::IProgressService* pProgressService ) throws_( CException* );

	bool SavePassword( const std::tstring& password );
	std::tstring LoadPassword( void );

	void StoreDocModelSchema( app::ModelSchema docModelSchema ) { m_docModelSchema = docModelSchema; }

	// "_Album.sld" stream (with .sld file format)
	void SaveAlbumDoc( CObject* pAlbumDoc );
	bool LoadAlbumDoc( CObject* pAlbumDoc );

	void LoadImagesMetadata( std::vector< CFileAttr* >& rOutFileAttribs );
	CCachedThumbBitmap* LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_();		// caller must delete the image

	static bool HasImageArchiveExt( const TCHAR* pFilePath );
	static const TCHAR* GetDefaultExtension( void ) { return s_compoundStgExts[ Ext_ias ]; }
protected:
	// base overrides
	virtual CStringW EncodeStreamName( const TCHAR* pStreamName ) const;

	template< typename Iterator >
	static void EncodeDeepStreamPath( Iterator itStart, Iterator itEnd ) { std::replace( itStart, itEnd, _T('\\'), s_subPathSep ); }		// '\' -> '*'

	template< typename Iterator >
	static void DecodeDeepStreamPath( Iterator itStart, Iterator itEnd ) { std::replace( itStart, itEnd, s_subPathSep, _T('\\') ); }		// '*' -> '\'
private:
	void CreateImageFiles( std::vector< CTransferFileAttr* >& rTransferAttribs, ui::IProgressService* pProgressService ) throws_( CException* );
	void CreateMetadataFile( const std::vector< CTransferFileAttr* >& transferAttribs );
	void CreateThumbnailsSubStorage( const std::vector< CTransferFileAttr* >& transferAttribs, ui::IProgressService* pProgressService );
private:
	CComPtr< IStorage > m_pThumbsStorage;
	const GUID* m_pThumbsDecoderId;
	app::ModelSchema m_docModelSchema;			// transient: loaded model schema from file, stored by the album doc

private:
	enum ExtensionType { Ext_ias, Ext_cid, Ext_icf, _Ext_Count };
	enum PwdFmt
	{
		PwdAnsi,							// old ANSI password stream
		PwdWide,							// new WIDE password stream
			_PwdTypeCount
	};
	enum ThumbsFmt
	{
		Thumbs_bmp,							// "Thumbnails" old sub-storage in BMP format (but with original extension .jpg, .png, etc)
		Thumbs_jpeg,						// "Thumbs_jpeg" new thumbnails sub-storage in JPG format (but with original extension .jpg, .png, etc)
			_ThumbsTypeCount
	};

	static const TCHAR* s_compoundStgExts[ _Ext_Count ];				// file extension for compound-files (".icf")
	static const TCHAR* s_pwdStreamNames[ _PwdTypeCount ];				// PwdAnsi for backwards compatibility with old saved .icf files
	static const TCHAR* s_thumbsSubStorageNames[ _ThumbsTypeCount ];	// name of the thumbs sub-storage (old "Thumbnails" in bmp format with .jpg extension)
	static const TCHAR s_metadataFilename[];							// meta-data file in a compound-file
	static const TCHAR s_albumFilename[];								// album info (superset of meta-data)
	static const TCHAR s_subPathSep;									// sub-path separator for deep stream names
public:
	class CFactory : public CThrowMode, public fs::IThumbProducer, private utl::noncopyable
	{
	public:
		CFactory( void ) : CThrowMode( false ) {}
		~CFactory() { Clear(); }							// factory singleton must be cleared on ExitInstance (not later)

		void Clear( void );

		CImageArchiveStg* FindStorage( const fs::CPath& stgFilePath ) const;
		CImageArchiveStg* AcquireStorage( const fs::CPath& stgFilePath, DWORD mode = STGM_READ );
		bool ReleaseStorage( const fs::CPath& stgFilePath );
		void ReleaseStorages( const std::vector< fs::CPath >& stgFilePaths );

		// FLEX: image path could refer to either physical image or archive-based image file
		CFile* OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode = CFile::modeRead );

		void LoadImagesMetadata( std::vector< CFileAttr* >& rOutFileAttribs, const fs::CPath& stgFilePath );

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
		stdext::hash_map< fs::CPath, CImageArchiveStg* > m_storageMap;		// owns values
		stdext::hash_map< fs::CPath, std::tstring > m_passwordProtected;	// known password-protected storages to encrypted passwords
		std::set< std::tstring > m_verifiedPasswords;
	};

	static CFactory& Factory( void );
private:
	enum { WriteableModeMask = STGM_CREATE | STGM_WRITE | STGM_READWRITE, NotOpenMask = -1 };

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
