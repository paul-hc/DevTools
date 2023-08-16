
#include "pch.h"
#include "ImageCatalogStg.h"
#include "CatalogStorageService.h"
#include "FileAttrAlgorithms.h"
#include "AlbumModel.h"
#include "ImagesModel.h"
#include "Application.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/FileEnumerator.h"
#include "utl/RuntimeException.h"
#include "utl/Serialization.h"
#include "utl/StringUtilities.h"
#include "utl/IProgressService.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/IUnknownImpl.hxx"
#include "utl/Serialization.hxx"


// CImageCatalogStg implementation

const TCHAR* CImageCatalogStg::s_pAlbumFolderName = _T("Album");

const TCHAR* CImageCatalogStg::s_thumbsFolderNames[] =
{
	_T("Thumbnails"),			// current storage name Slider_v5_3+ (used to be the legacy one, but was promoted in light of the new implicit thumbnail encoder rules)
	_T("Thumbs_jpeg")			// middle aged storage name (Slider_v3_1..Slider_v5_2)
};

const TCHAR* CImageCatalogStg::s_pAlbumStreamName = _T("_Album.sld");
const TCHAR* CImageCatalogStg::s_pAlbumMapStreamName = _T("_AlbumMap.txt");

const TCHAR* CImageCatalogStg::s_passwordStreamNames[] =
{
	_T("_pwd.w"),				// new WIDE password stream
	_T("pwdW"),					// old WIDE password stream
	_T("pwd")					// legacy ANSI password stream (for backwards compatibility with old saved .icf files)
};


CImageCatalogStg::CImageCatalogStg( void )
	: fs::CStructuredStorage()
	, m_docModelSchema( app::Slider_LatestModelSchema )
	, m_hasAlbumMap( utl::Default )
{
	SetUseFlatStreamNames( true );		// store deep image stream paths as flattened encoded root streams
}

CImageCatalogStg::~CImageCatalogStg()
{
	if ( IsOpen() )
		CloseDocFile();		// close early so that virtual methods are called properly
}

void CImageCatalogStg::CreateObject( ICatalogStorage** ppCatalogStorage )
{
	ASSERT_PTR( ppCatalogStorage );

	*ppCatalogStorage = new CImageCatalogStg();			// initially with a refCount of 1

	ENSURE( 1 == dbg::GetRefCount( *ppCatalogStorage ) );
	ENSURE( !( *ppCatalogStorage )->GetDocStorage()->IsOpen() );
}

void CImageCatalogStg::CloseDocFile( void )
{
	if ( !IsOpen() )
		return;

	DiscardCachedImages( GetDocFilePath() );

	__super::CloseDocFile();
}

size_t CImageCatalogStg::DiscardCachedImages( const fs::TStgDocPath& docStgPath )
{
	REQUIRE( !docStgPath.IsEmpty() );

	// Release indirect references on any open streams for this image archive file - the streams open with CFile::shareExclusive are kept open indirectly by WIC COM interfaces.

	size_t count = CWicImageCache::Instance().DiscardWithPrefix( docStgPath.GetPtr() );		// discard cached images for the storage
	count += app::GetThumbnailer()->DiscardWithPrefix( docStgPath.GetPtr() );				// also discard the thumbs
	return count;
}

bool CImageCatalogStg::IsSpecialStreamName( const TCHAR* pStreamName )
{
	static std::vector<const TCHAR*> s_reservedStreamNames;
	if ( s_reservedStreamNames.empty() )
	{
		s_reservedStreamNames.insert( s_reservedStreamNames.end(), s_passwordStreamNames, END_OF( s_passwordStreamNames ) );
		s_reservedStreamNames.insert( s_reservedStreamNames.end(), s_thumbsFolderNames, END_OF( s_thumbsFolderNames ) );
		s_reservedStreamNames.push_back( s_pAlbumStreamName );
		s_reservedStreamNames.push_back( s_pAlbumMapStreamName );
	}

	for ( size_t i = 0; i != s_reservedStreamNames.size(); ++i )
		if ( path::Equals( s_reservedStreamNames[ i ], pStreamName ) )
			return true;

	return false;
}

TCHAR CImageCatalogStg::GetFlattenPathSep( void ) const
{
	if ( m_docModelSchema <= app::Slider_v5_1 )
		return _T('*');			// use old separator for backwards compatibility (the short 31 characters hash suffix uses the old separator)

	return __super::GetFlattenPathSep();
}

bool CImageCatalogStg::RetainOpenedStream( const fs::TEmbeddedPath& streamPath ) const
{
	if ( !streamPath.HasParentPath() )							// a root stream?
		if ( !IsSpecialStreamName( streamPath.GetPtr() ) )		// not a pre-defined one?
			return true;										// keep opened image streams alive for shared access

	return __super::RetainOpenedStream( streamPath );
}


// ICatalogStorage interface implementation

DEFINE_GUID( IID_ICatalogStorage, 0xC5078A69, 0xFF1C, 0x4B13, 0x92, 0x2, 0x1E, 0xEF, 0x5, 0x32, 0x25, 0x8D );		// {C5078A69-FF1C-4B13-9202-1EEF0532258D}

fs::CStructuredStorage* CImageCatalogStg::GetDocStorage( void )
{
	return this;
}

app::ModelSchema CImageCatalogStg::GetDocModelSchema( void ) const
{
	return m_docModelSchema;
}

void CImageCatalogStg::StoreDocModelSchema( app::ModelSchema docModelSchema )
{
	m_docModelSchema = docModelSchema;
}

void CImageCatalogStg::StorePassword( const std::tstring& password )
{
	m_password = password;

	if ( !m_password.empty() )
		CCatalogPasswordStore::Instance()->CacheVerifiedPassword( m_password );
}

void CImageCatalogStg::CreateImageArchiveFile( const fs::TStgDocPath& docStgPath, CCatalogStorageService* pCatalogSvc ) throws_( CException* )
{
	ASSERT_PTR( pCatalogSvc );

	CWaitCursor wait;
	fs::stg::CScopedCreateDocMode scopedThrow( this, nullptr );		// closes the archive when exiting the scope

	CreateDocFile( docStgPath );
	ASSERT( IsOpen() );

	CreateDir( s_pAlbumFolderName );					// create "Album" sub-storage
	CreateDir( s_thumbsFolderNames[ CurrentVer ] );		// create "Thumbnails" sub-storage

	StorePassword( pCatalogSvc->GetPassword() );
	SavePasswordStream();

	CreateImageFiles( pCatalogSvc );
	CreateThumbnailsSubStorage( pCatalogSvc );

	if ( CObject* pAlbumDoc = pCatalogSvc->GetAlbumDoc() )
		SaveAlbumStream( pAlbumDoc );			// also save the "_Album.sld" stream
}

void CImageCatalogStg::CreateImageFiles( CCatalogStorageService* pCatalogSvc ) throws_( CException* )
{
	std::vector<CTransferFileAttr*>& rTransferAttrs = pCatalogSvc->RefTransferAttrs();

	CCatalogStorageFactory* pFactory = CCatalogStorageFactory::Instance();
	CScopedErrorHandling scopedThrow( pFactory, utl::ThrowMode );

	pCatalogSvc->GetProgress()->AdvanceStage( _T("Saving embedded image files") );
	pCatalogSvc->GetProgress()->SetBoundedProgressCount( rTransferAttrs.size() );

	CScopedCurrentDir scopedAlbumFolder( this, s_pAlbumFolderName, STGM_READWRITE );		// location of "_AlbumMap.txt"
	CAlbumMapWriter albumMap( CreateStreamFile( s_pAlbumMapStreamName ) );
	albumMap.WriteHeader();

	CScopedCurrentDir scopedRootFolder( this, s_rootFolderName, STGM_READWRITE );			// location of archived images

	for ( size_t pos = 0; pos != rTransferAttrs.size(); )
	{
		CTransferFileAttr* pXferAttr = rTransferAttrs[ pos ];

		pCatalogSvc->GetProgress()->AdvanceItem( pXferAttr->GetPath().Get() );

		try
		{
			const fs::TEmbeddedPath& streamPath = pXferAttr->GetPath();

			std::auto_ptr<CFile> pSrcImageFile = pFactory->OpenFlexImageFile( pXferAttr->GetSrcImagePath() );
			std::auto_ptr<COleStreamFile> pDestStreamFile( CreateStreamFile( streamPath.GetPtr() ) );

			fs::BufferedCopy( *pDestStreamFile, *pSrcImageFile );

			albumMap.WriteEntry( EncodeStreamName( streamPath.GetPtr() ), streamPath.GetPtr() );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
			{
				case IDABORT:	throw new mfc::CUserAbortedException();
				case IDRETRY:	continue;
				case IDIGNORE:
					// remove the offender
					delete pXferAttr;
					rTransferAttrs.erase( rTransferAttrs.begin() + pos );
					break;
			}
		}

		++pos;
	}

	UINT64 totalImagesSize = std::accumulate( rTransferAttrs.begin(), rTransferAttrs.end(), UINT64( 0 ), func::AddFileSize() );

	albumMap.WriteImageTotals( totalImagesSize );
}

void CImageCatalogStg::CreateThumbnailsSubStorage( const CCatalogStorageService* pCatalogSvc ) throws_( CException* )
{
	const std::vector<CTransferFileAttr*>& transferAttrs = pCatalogSvc->GetTransferAttrs();

	pCatalogSvc->GetProgress()->AdvanceStage( _T("Saving thumbnails") );
	pCatalogSvc->GetProgress()->SetBoundedProgressCount( transferAttrs.size() );

	CScopedCurrentDir scopedThumbsFolder( this, s_thumbsFolderNames[ CurrentVer ], STGM_READWRITE );

	CThumbnailer* pThumbnailer = app::GetThumbnailer();
	thumb::CPushBoundsSize largerBounds( pThumbnailer, 128 );			// generate larger thumbs to minimize regeneration later

	size_t thumbCount = 0;
	UINT64 totalThumbsSize = 0;

	for ( std::vector<CTransferFileAttr*>::const_iterator itXferAttr = transferAttrs.begin(); itXferAttr != transferAttrs.end(); )
	{
		try
		{
			if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->AcquireThumbnail( ( *itXferAttr )->GetSrcImagePath() ) )
			{
				fs::TEmbeddedPath thumbStreamName;
				if ( const GUID* pContainerFormatId = wic::GetContainerFormatId( MakeThumbStreamName( thumbStreamName, ( *itXferAttr )->GetPath().GetPtr() ) ) )	// good to save thumbnail?
				{
					CComPtr<IStream> pThumbStream = CreateStream( thumbStreamName.GetPtr() );
					CScopedErrorHandling scopedThrow( &pThumbBitmap->GetOrigin(), utl::ThrowMode );

					pThumbBitmap->GetOrigin().SaveBitmapToStream( pThumbStream, *pContainerFormatId );

					++thumbCount;
					totalThumbsSize += fs::stg::GetStreamSize( &*pThumbStream );
				}
			}
			else
				throw new mfc::CRuntimeException( str::Format( _T("Cannot generate a thumbnail for source image:\n%s"), ( *itXferAttr )->GetSrcImagePath().GetPtr() ) );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_CANCELTRYCONTINUE | MB_ICONEXCLAMATION | MB_DEFBUTTON3 ) )
			{
				case IDCANCEL:		throw new mfc::CUserAbortedException();
				case IDTRYAGAIN:	continue;
				case IDCONTINUE:	break;
			}
		}

		pCatalogSvc->GetProgress()->AdvanceItem( ( *itXferAttr )->GetPath().Get() );
		++itXferAttr;
	}

	ResetToRootCurrentDir();

	CScopedCurrentDir scopedAlbumFolder( this, s_pAlbumFolderName, STGM_READWRITE );		// location of "_AlbumMap.txt"
	CAlbumMapWriter albumMap( OpenStreamFile( s_pAlbumMapStreamName, CFile::modeWrite ) );

	albumMap.SeekToEnd();		// will append text
	albumMap.WriteThumbnailTotals( thumbCount, totalThumbsSize );
}

CCachedThumbBitmap* CImageCatalogStg::LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_()
{
	ASSERT( IsOpen() );

	try
	{
		CComPtr<IStorage> pThumbsStorage;
		if ( const TCHAR* pThumbsFolderName = FindAlternate_DirName( ARRAY_SPAN( s_thumbsFolderNames ) ).first )
			pThumbsStorage = OpenDir( pThumbsFolderName );

		if ( pThumbsStorage != nullptr )
		{
			CScopedCurrentDir scopedThumbsFolder( this, pThumbsStorage );

			if ( CComPtr<IStream> pThumbStream = OpenThumbnailImageStream( imageComplexPath.GetEmbeddedPathPtr() ) )
				if ( CComPtr<IWICBitmapSource> pSavedBitmap = wic::LoadBitmapFromStream( pThumbStream ) )
					if ( CCachedThumbBitmap* pThumbnail = app::GetThumbnailer()->NewScaledThumb( pSavedBitmap, imageComplexPath ) )
					{
						// IMP: the resulting bitmap, even the scaled bitmap will keep the stream alive for the lifetime of the bitmap; same if we scale the bitmap;
						// Since the thumb keeps alive the WIC bitmap, we need to copy to a memory bitmap.
						// This way we unlock the doc stg stream for future access.
						//
						pThumbnail->GetOrigin().DetachSourceToBitmap();
						return pThumbnail;
					}
		}
	}
	catch ( CException* pExc )
	{
		app::HandleException( pExc );
	}
	return nullptr;
}

CComPtr<IStream> CImageCatalogStg::OpenThumbnailImageStream( const TCHAR* pImageEmbeddedPath )
{
	fs::TEmbeddedPath thumbStreamName;
	MakeThumbStreamName( thumbStreamName, pImageEmbeddedPath );

	const TCHAR* pThumbStreamName = thumbStreamName.GetPtr();

	if ( !path::Equivalent( pThumbStreamName, pImageEmbeddedPath ) )		// possibly an archive build with Slider_v5_2-
		if ( StreamExist( pImageEmbeddedPath ) )			// backwards compatibility: also try with straight SRC image path as stream name
			pThumbStreamName = pImageEmbeddedPath;

	if ( nullptr == pThumbStreamName )
		return nullptr;

	return OpenStream( pThumbStreamName );
}

wic::ImageFormat CImageCatalogStg::MakeThumbStreamName( fs::TEmbeddedPath& rThumbStreamName, const TCHAR* pSrcImagePath )
{
	// Note: for transparency we can't rely on CWicBitmap::GetBmpFmt().m_hasAlphaChannel, as is true even for a true color .jpg,
	// so we pick a thumbnail format that matches the qualities of the source image.
	// As a result, the thumbnail stream extension may be different than the original embedded image file.

	rThumbStreamName.Set( pSrcImagePath );

	wic::ImageFormat thumbImageFormat = wic::JpegFormat;

	switch ( wic::FindFileImageFormat( pSrcImagePath ) )
	{
		case wic::BmpFormat:
		case wic::JpegFormat:
		case wic::TiffFormat:
		case wic::WmpFormat:
			thumbImageFormat = wic::JpegFormat;
			break;
		case wic::GifFormat:
			return wic::GifFormat;			// keep it as it is
		case wic::PngFormat:
		case wic::IcoFormat:
			thumbImageFormat = wic::PngFormat;
			break;
		case wic::UnknownImageFormat:
			return wic::UnknownImageFormat;
	}

	wic::ReplaceImagePathExt( rThumbStreamName, thumbImageFormat );
	return thumbImageFormat;
}

bool CImageCatalogStg::DeleteOldVersionStream( const TCHAR* pStreamName )
{
	CScopedCurrentDir scopedRootDir( this, s_rootFolderName, STGM_READWRITE );		// previously metadata files were located in the root storage

	if ( StreamExist( pStreamName ) )
		if ( DeleteStream( pStreamName ) )
			return true;

	return false;
}

bool CImageCatalogStg::DeleteAnyOldVersionStream( const TCHAR* altStreamNames[], size_t altCount )
{
	REQUIRE( altCount != 0 );

	for ( size_t i = 0; i != altCount; ++i )
		if ( DeleteOldVersionStream( altStreamNames[ i ] ) )
			return true;

	return false;
}


bool CImageCatalogStg::SaveAlbumStream( CObject* pAlbumDoc )
{
	ASSERT_PTR( pAlbumDoc );
	REQUIRE( IsOpenForWriting() );

	DeleteOldVersionStream( s_pAlbumStreamName );		// File Save: delete old root stream - album files have been moved to "Album" folder

	CScopedCurrentDir scopedAlbumFolder( this, s_pAlbumFolderName, STGM_READWRITE );
	std::auto_ptr<COleStreamFile> pAlbumFile( CreateStreamFile( s_pAlbumStreamName ) );

	if ( nullptr == pAlbumFile.get() )
		return false;

	CArchive archive( pAlbumFile.get(), CArchive::store );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
	pAlbumDoc->Serialize( archive );
	return true;
}

bool CImageCatalogStg::LoadAlbumStream( CObject* pAlbumDoc )
{
	ASSERT_PTR( pAlbumDoc );

	CScopedErrorHandling scopedCheck( this, utl::CheckMode );			// album not found is not an error
	CScopedCurrentDir scopedAlbumFolder( this, StorageExist( s_pAlbumFolderName ) ? s_pAlbumFolderName : s_rootFolderName );

	if ( StreamExist( s_pAlbumStreamName ) )							// older catalogs may not contain the album stream (not an error)
	{
		std::auto_ptr<COleStreamFile> pAlbumFile = OpenStreamFile( s_pAlbumStreamName );		// load stream "_Album.sld"

		if ( nullptr == pAlbumFile.get() )
			return false;

		CArchive loadArchive( pAlbumFile.get(), CArchive::load );
		loadArchive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
		loadArchive.m_pDocument = reinterpret_cast<CDocument*>( pAlbumDoc );

		pAlbumDoc->Serialize( loadArchive );

		StoreDocModelSchema( svc::ToAlbumModel( pAlbumDoc )->GetModelSchema() );		// copy over the model schema saved in the album stream
		return true;
	}

	// backwards compatibility:
	CImagesModel& rImagesModel = svc::ToImagesModel( pAlbumDoc );
	rImagesModel.Clear();

	if ( bkw_LoadAlbumMetadataStream( pAlbumDoc ) )			// load "_Meta.data" stream - older catalogs may not contain the album stream (not an error)
		return true;

	if ( EnumerateImages( rImagesModel ) )					// fallback to enumerating the image content in the root storage
		return true;

	return false;
}

bool CImageCatalogStg::bkw_LoadAlbumMetadataStream( CObject* pAlbumDoc )
{
	static const TCHAR s_metadataStreamName[] = _T("_Meta.data");
	CScopedCurrentDir scopedAlbumFolder( this, s_rootFolderName );

	ASSERT_PTR( pAlbumDoc );
	CImagesModel& rImagesModel = svc::ToImagesModel( pAlbumDoc );

	if ( StreamExist( s_metadataStreamName ) )
	{
		std::auto_ptr<COleStreamFile> pMetadataFile = OpenStreamFile( s_metadataStreamName );

		if ( pMetadataFile.get() != nullptr )
		{
			CArchive loadArchive( pMetadataFile.get(), CArchive::load );
			loadArchive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

			// needed for backwards compatibility
			loadArchive.m_pDocument = reinterpret_cast<CDocument*>( pAlbumDoc );

			// bug fix: speculate less, and let the CFileAttr::EvalLoadingSchema() do the finer model schema evaluation (from the binary stream)
			//bkw_AlterOlderDocModelSchema( app::Slider_v4_0 );		// arbitrarily set to an older version

			serial::CStreamingGuard schemaGuard( loadArchive );
			serial::CScopedLoadingArchive scopedLoadingArchive( loadArchive, m_docModelSchema );

			try
			{	// load metadata
				persist UINT totalImagesSize;
				loadArchive >> totalImagesSize;

				serial::StreamOwningPtrs( loadArchive, rImagesModel.RefFileAttrs() );
				rImagesModel.StoreBaselineSequence();

				bkw_AlterOlderDocModelSchema( svc::ToAlbumModel( pAlbumDoc )->GetModelSchema() );					// alter with the older model version

				ASSERT( rImagesModel.IsEmpty() || rImagesModel.GetFileAttrAt( 0 )->GetPath().IsComplexPath() );		// ensure deserialized to full complex path
				return true;
			}
			catch ( CException* pExc )
			{
				app::HandleException( pExc );
			}
		}
	}

	return false;
}

bool CImageCatalogStg::EnumerateImages( CImagesModel& rImagesModel )
{
	ASSERT( IsOpenForReading() );

	rImagesModel.Clear();

	CScopedCurrentDir scopedAlbumFolder( this, s_rootFolderName );
	fs::CPathEnumerator imagesEnum;

	if ( !EnumElements( &imagesEnum, Shallow ) )			// enumerate root files
		return false;

	const fs::TStgDocPath& docStgPath = GetDocFilePath();

	for ( std::vector<fs::TEmbeddedPath>::const_iterator itStreamPath = imagesEnum.m_filePaths.begin(); itStreamPath != imagesEnum.m_filePaths.end(); ++itStreamPath )
		if ( wic::FindFileImageFormat( itStreamPath->GetPtr() ) != wic::UnknownImageFormat )		// an image file?
			if ( CComPtr<IStream> pImageStream = OpenStream( itStreamPath->GetPtr() ) )
				if ( const fs::CStreamState* pSrcStreamState = FindOpenedStream( *itStreamPath ) )
				{
					fs::CFileState streamState = *pSrcStreamState;
					std::tstring streamPath = itStreamPath->Get();

					bkw_DecodeStreamPath( streamPath );		// un-flatten deep stream names, adjusting m_docModelSchema based on separator of deep stream names

					streamState.m_fullPath = fs::CFlexPath::MakeComplexPath( docStgPath.Get(), streamPath );	// store the fully qualified flex-path of image

					rImagesModel.AddFileAttr( new CFileAttr( streamState ) );

					// keep the cached stream open: otherwise it causes sharing violations later on LocateReadStream:
					//CloseStream( itStreamPath->GetPtr() );
					//TRACE_COM_PTR( pImageStream, "CImageCatalogStg::EnumerateImages() - after CloseStream" );
				}

	rImagesModel.StoreBaselineSequence();
	return true;
}

void CImageCatalogStg::bkw_DecodeStreamPath( std::tstring& rStreamPath )
{
	// un-flatten deep stream names, heuristically adjusting m_docModelSchema based on separator of deep stream names
	if ( rStreamPath.find( GetFlattenPathSep() ) != std::tstring::npos )
	{
		std::replace( rStreamPath.begin(), rStreamPath.end(), GetFlattenPathSep(), _T('\\') );		// deepify '|' -> '\'
		bkw_AlterOlderDocModelSchema( app::Slider_v5_2 );		// arbitrarily assume the newest older version using '|' stream sepator
	}
	else if ( rStreamPath.find( _T('*') ) != std::tstring::npos )
	{
		std::replace( rStreamPath.begin(), rStreamPath.end(), _T('*'), _T('\\') );					// deepify '*' -> '\'
		bkw_AlterOlderDocModelSchema( app::Slider_v4_2 );		// arbitrarily assume an older version using '*' stream sepator
	}
	else
		bkw_AlterOlderDocModelSchema( app::Slider_v5_5 );		// arbitrarily assume the previous to current version (newest older version)
}

bool CImageCatalogStg::bkw_AlterOlderDocModelSchema( app::ModelSchema docModelSchema )
{
	if ( docModelSchema >= m_docModelSchema )
		return false;			// only allow altering to older model schema

	StoreDocModelSchema( docModelSchema );
	return true;
}

bool CImageCatalogStg::SavePasswordStream( void )
{
	REQUIRE( IsOpenForWriting() );

	try
	{
		DeleteAnyOldVersionStream( ARRAY_SPAN( s_passwordStreamNames ) );		// remove existing password stream

		if ( m_password.empty() )
			return true;			// no password, done

		CScopedCurrentDir scopedAlbumFolder( this, s_pAlbumFolderName, STGM_READWRITE );
		std::auto_ptr<COleStreamFile> pPwdFile( CreateStreamFile( s_passwordStreamNames[ CurrentVer ] ) );

		if ( nullptr == pPwdFile.get() )
			return false;

		std::tstring encryptedPassword = pwd::ToEncrypted( m_password );
		CArchive saveArchive( pPwdFile.get(), CArchive::store );
		saveArchive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

		saveArchive << encryptedPassword;

		saveArchive.Close();
		//pwdFile.Close();
		return true;
	}
	catch ( CException* pExc )
	{
		app::GetUserReport().ReportError( pExc, MB_OK | MB_ICONHAND );
		return false;
	}
}

bool CImageCatalogStg::LoadPasswordStream( void )
{
	ASSERT( IsOpen() );

	const TCHAR* pPasswordStreamName = nullptr;
	bool hasWidePwd = true;				// found a WIDE stream?

	CScopedCurrentDir scopedAlbumFolder( this, s_rootFolderName );			// start lookup in root (for backwards compatibility)

	{
		std::pair<const TCHAR*, size_t> streamName = FindAlternate_StreamName( ARRAY_SPAN( s_passwordStreamNames ) );
		pPasswordStreamName = streamName.first;

		if ( streamName.first != nullptr )			// password stream found in the root?
			hasWidePwd = streamName.second != ( COUNT_OF( s_passwordStreamNames ) - 1 );		// oldest stream name uses ANSI stream?
	}

	if ( nullptr == pPasswordStreamName )
		if ( StorageExist( s_pAlbumFolderName ) )
		{
			VERIFY( ChangeCurrentDir( s_pAlbumFolderName ) );

			if ( StreamExist( s_passwordStreamNames[ CurrentVer ] ) )
				pPasswordStreamName = s_passwordStreamNames[ CurrentVer ];
		}

	if ( nullptr == pPasswordStreamName )
	{
		StorePassword( std::tstring() );
		return true;					// document is not password-protected
	}

	std::tstring password;
	bool succeeded = true;
	std::auto_ptr<COleStreamFile> pPwdFile = OpenStreamFile( pPasswordStreamName );
	ASSERT_PTR( pPwdFile.get() );

	CArchive loadArchive( pPwdFile.get(), CArchive::load );
	serial::CScopedLoadingArchive scopedLoadingArchive( loadArchive, m_docModelSchema );

	loadArchive.m_bForceFlat = FALSE;		// same as CDocument::OnOpenDocument()
	try
	{
		if ( hasWidePwd )
		{
			std::wstring widePassword;
			loadArchive >> widePassword;
			password = pwd::ToString( widePassword.c_str() );
		}
		else
		{
			std::string ansiPassword;
			loadArchive >> ansiPassword;
			password = pwd::ToString( ansiPassword.c_str() );

			StoreDocModelSchema( app::Slider_v3_1 );		// assume the oldest legacy version
		}

		m_password = pwd::ToDecrypted( password );

		succeeded = true;
	}
	catch ( CException* pExc )
	{
		app::GetUserReport().ReportError( pExc );
		succeeded = false;
	}
	loadArchive.Close();
	//pwdFile.Close();

	return succeeded;
}

bool CImageCatalogStg::LoadAlbumMap( std::tstring* pAlbumMapText )
{
	if ( m_hasAlbumMap != utl::Default && nullptr == pAlbumMapText )
		return utl::True == m_hasAlbumMap;

	CScopedCurrentDir scopedAlbumFolder( this, s_pAlbumFolderName );

	if ( utl::Default == m_hasAlbumMap )
		utl::SetTernary( m_hasAlbumMap, StreamExist( s_pAlbumMapStreamName ) );

	if ( pAlbumMapText != nullptr )
		if ( utl::True == m_hasAlbumMap )
		{
			std::auto_ptr<COleStreamFile> pTextFile = OpenStreamFile( s_pAlbumMapStreamName );
			ASSERT_PTR( pTextFile.get() );

			UINT totalLength = static_cast<UINT>( pTextFile->GetLength() );
			std::vector<char> buffer( totalLength + 1, _T('\0') );
			pTextFile->Read( &buffer.front(), totalLength );

			*pAlbumMapText = str::FromUtf8( &buffer.front() );
		}
		else
			pAlbumMapText->clear();

	return utl::True == m_hasAlbumMap;
}


// CAlbumMapWriter implementation

const TCHAR CImageCatalogStg::CAlbumMapWriter::s_lineFmt[] = _T("%-*s  %-*s%s");

CImageCatalogStg::CAlbumMapWriter::CAlbumMapWriter( std::auto_ptr<COleStreamFile> pAlbumMapFile )
	: fs::CTextFileWriter( pAlbumMapFile.get() )
	, m_pAlbumMapFile( pAlbumMapFile )
	, m_imageCount( 0 )
{
}

void CImageCatalogStg::CAlbumMapWriter::WriteHeader( void )
{
	static const TCHAR* s_header[] = { _T("NO."), _T("STREAM NAME"), _T("IMAGE PATH") };
	enum HeaderField { StreamNo, StreamName, ImagePath };

	WriteLine( str::Format( s_lineFmt, StreamNoPadding, s_header[ StreamNo ], StreamNamePadding, s_header[ StreamName ], s_header[ ImagePath ] ) );

	static const TCHAR s_underlineCh = _T('-');

	WriteLine( str::Format( s_lineFmt,
		StreamNoPadding, std::tstring( str::GetLength( s_header[ StreamNo ] ), s_underlineCh ).c_str(),
		StreamNamePadding, std::tstring( str::GetLength( s_header[ StreamName ] ), s_underlineCh ).c_str(),
		std::tstring( str::GetLength( s_header[ ImagePath ] ), s_underlineCh ).c_str() ) );
}

void CImageCatalogStg::CAlbumMapWriter::WriteEntry( const std::tstring& streamName, const TCHAR* pImageEmbeddedPath )
{
	++m_imageCount;

	WriteLine( str::Format( s_lineFmt,
		StreamNoPadding, num::FormatNumber( m_imageCount ).c_str(),
		StreamNamePadding, streamName.c_str(),
		pImageEmbeddedPath ) );
}

void CImageCatalogStg::CAlbumMapWriter::WriteImageTotals( UINT64 totalImagesSize )
{
	WriteEmptyLine();

	std::tstring fileSizePair = num::FormatFileSizeAsPair( totalImagesSize );

	WriteLine( str::Format( _T("Total images: %d"), m_imageCount ) );
	WriteLine( str::Format( _T("Total size of images: %s"), fileSizePair.c_str() ) );

	TRACE( _T("(*) Total image archive file size: %s\n"), fileSizePair.c_str() );
}

void CImageCatalogStg::CAlbumMapWriter::WriteThumbnailTotals( size_t thumbCount, UINT64 totalThumbsSize )
{
	WriteEmptyLine();

	WriteLine( str::Format( _T("Total thumbnails: %d"), thumbCount ) );
	WriteLine( str::Format( _T("Total size of thumbnails: %s"), num::FormatFileSizeAsPair( totalThumbsSize ).c_str() ) );
}
