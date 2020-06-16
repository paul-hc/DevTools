
#include "stdafx.h"
#include "ImageArchiveStg.h"
#include "ImageStorageService.h"
#include "FileAttrAlgorithms.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/Serialization.h"
#include "utl/StringUtilities.h"
#include "utl/UI/IProgressService.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/PasswordDialog.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CImageArchiveStg implementation

const TCHAR* CImageArchiveStg::s_compoundStgExts[] = { _T(".ias"), _T(".cid"), _T(".icf") };

const TCHAR* CImageArchiveStg::s_passwordStreamNames[] =
{
	_T("_pwd.w"),				// new WIDE password stream
	_T("pwdW"),					// old WIDE password stream
	_T("pwd")					// legacy ANSI password stream (for backwards compatibility with old saved .icf files)
};

const TCHAR* CImageArchiveStg::s_thumbsStorageNames[] =
{
	_T("Thumbnails"),			// current storage name Slider_v5_3+ (used to be the legacy one, but was promoted in light of the new implicit thumbnail encoder rules)
	_T("Thumbs_jpeg")			// middle aged storage name (Slider_v3_1..Slider_v5_2)
};

const TCHAR CImageArchiveStg::s_albumStreamName[] = _T("_Album.sld");
const TCHAR CImageArchiveStg::s_albumMapStreamName[] = _T("_AlbumMap.txt");


CImageArchiveStg::CImageArchiveStg( void )
	: fs::CStructuredStorage()
	, m_docModelSchema( app::Slider_LatestModelSchema )
{
	SetUseFlatStreamNames( true );		// store deep image stream paths as flattened encoded root streams
}

CImageArchiveStg::~CImageArchiveStg()
{
	if ( IsOpen() )
		CloseDocFile();		// close early so that virtual methods are called properly
}

CImageArchiveStg::CFactory* CImageArchiveStg::Factory( void )
{
	static CFactory s_factory;
	return &s_factory;
}

void CImageArchiveStg::CloseDocFile( void )
{
	if ( !IsOpen() )
		return;

	DiscardCachedImages( GetDocFilePath() );

	__super::CloseDocFile();
}

void CImageArchiveStg::DiscardCachedImages( const fs::CPath& stgFilePath )
{
	if ( !stgFilePath.IsEmpty() )
		CWicImageCache::Instance().DiscardWithPrefix( stgFilePath.GetPtr() );		// discard cached images for the storage
}

bool CImageArchiveStg::IsSpecialStreamName( const TCHAR* pStreamName )
{
	static std::vector< const TCHAR* > s_reservedStreamNames;
	if ( s_reservedStreamNames.empty() )
	{
		s_reservedStreamNames.insert( s_reservedStreamNames.end(), s_passwordStreamNames, END_OF( s_passwordStreamNames ) );
		s_reservedStreamNames.insert( s_reservedStreamNames.end(), s_passwordStreamNames, END_OF( s_thumbsStorageNames ) );
		s_reservedStreamNames.push_back( s_albumStreamName );
		s_reservedStreamNames.push_back( s_albumMapStreamName );
	}

	for ( size_t i = 0; i != s_reservedStreamNames.size(); ++i )
		if ( path::EqualsPtr( s_reservedStreamNames[ i ], pStreamName ) )
			return true;

	return false;
}

TCHAR CImageArchiveStg::GetFlattenPathSep( void ) const
{
	if ( m_docModelSchema < app::Slider_v5_2 )
		return _T('*');			// use old separator for backwards compatibility (the short 31 characters hash suffix uses the old separator)

	return __super::GetFlattenPathSep();
}

bool CImageArchiveStg::RetainOpenedStream( const fs::TEmbeddedPath& streamPath ) const
{
	if ( !streamPath.HasParentPath() )							// a root stream?
		if ( !IsSpecialStreamName( streamPath.GetPtr() ) )		// not a pre-defined one?
			return true;										// keep opened image streams alive for shared access

	return __super::RetainOpenedStream( streamPath );
}

void CImageArchiveStg::CreateImageArchive( const fs::CPath& stgFilePath, CImageStorageService* pImagesSvc ) throws_( CException* )
{
	ASSERT_PTR( pImagesSvc );

	CScopedErrorHandling scopedThrow( this, utl::ThrowMode );

	CreateDocFile( stgFilePath.GetPtr() );
	ASSERT( IsOpen() );

	CWaitCursor wait;

	SavePassword( pImagesSvc->GetPassword() );
	Factory()->CacheVerifiedPassword( stgFilePath, pImagesSvc->GetPassword() );

	CreateImageFiles( pImagesSvc );
	CreateThumbnailsSubStorage( pImagesSvc );
}

void CImageArchiveStg::CreateImageFiles( CImageStorageService* pImagesSvc ) throws_( CException* )
{
	// Prevent sharing violations on SRC stream open.
	//	2020-04-11: Still doesn't work, I get exception on open. I suspect the source stream (image file) must be kept open with CFile::shareExclusive by some WIC indirect COM interface.
	if ( !pImagesSvc->IsEmpty() )
		app::GetThumbnailer()->Clear();		// also discard the thumbs - note: cached SRC images were discarded by CImageStorageService::Build()

	std::vector< CTransferFileAttr* >& rTransferAttrs = pImagesSvc->RefTransferAttrs();

	CFactory* pFactory = Factory();
	CScopedErrorHandling scopedThrow( pFactory, utl::ThrowMode );

	pImagesSvc->GetProgress()->AdvanceStage( _T("Saving embedded image files") );
	pImagesSvc->GetProgress()->SetBoundedProgressCount( rTransferAttrs.size() );

	CAlbumMapWriter albumMap( CreateStreamFile( s_albumMapStreamName ) );		// binary mode
	albumMap.WriteHeader();

	for ( size_t pos = 0; pos != rTransferAttrs.size(); )
	{
		CTransferFileAttr* pXferAttr = rTransferAttrs[ pos ];

		pImagesSvc->GetProgress()->AdvanceItem( pXferAttr->GetPath().Get() );

		try
		{
			const fs::TEmbeddedPath& streamPath = pXferAttr->GetPath();

			std::auto_ptr< CFile > pSrcImageFile = pFactory->OpenFlexImageFile( pXferAttr->GetSrcImagePath() );
			std::auto_ptr< COleStreamFile > pDestStreamFile( CreateStreamFile( streamPath.GetPtr() ) );

			fs::BufferedCopy( *pDestStreamFile, *pSrcImageFile );

			albumMap.WriteEntry( EncodeStreamName( streamPath.GetPtr() ), streamPath.GetPtr() );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
			{
				case IDABORT:	throw new mfc::CUserAbortedException;
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

void CImageArchiveStg::CreateThumbnailsSubStorage( const CImageStorageService* pImagesSvc ) throws_( CException* )
{
	const std::vector< CTransferFileAttr* >& transferAttrs = pImagesSvc->GetTransferAttrs();

	pImagesSvc->GetProgress()->AdvanceStage( _T("Saving thumbnails") );
	pImagesSvc->GetProgress()->SetBoundedProgressCount( transferAttrs.size() );

	CScopedCurrentDir scopedThumbsDir( this, CreateDir( s_thumbsStorageNames[ CurrentVer ] ) );

	CThumbnailer* pThumbnailer = app::GetThumbnailer();
	thumb::CPushBoundsSize largerBounds( pThumbnailer, 128 );			// generate larger thumbs to minimize regeneration later

	size_t thumbCount = 0;
	UINT64 totalThumbsSize = 0;

	for ( std::vector< CTransferFileAttr* >::const_iterator itXferAttr = transferAttrs.begin(); itXferAttr != transferAttrs.end(); )
	{
		try
		{
			if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->AcquireThumbnail( ( *itXferAttr )->GetSrcImagePath() ) )
			{
				fs::TEmbeddedPath thumbStreamName;
				if ( const GUID* pContainerFormatId = wic::GetContainerFormatId( MakeThumbStreamName( thumbStreamName, ( *itXferAttr )->GetPath().GetPtr() ) ) )	// good to save thumbnail?
				{
					CComPtr< IStream > pThumbStream = CreateStream( thumbStreamName.GetPtr() );
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
				case IDCANCEL:		throw new mfc::CUserAbortedException;
				case IDTRYAGAIN:	continue;
				case IDCONTINUE:	break;
			}
		}

		pImagesSvc->GetProgress()->AdvanceItem( ( *itXferAttr )->GetPath().Get() );
		++itXferAttr;
	}

	ResetToRootCurrentDir();

	CAlbumMapWriter albumMap( OpenStreamFile( s_albumMapStreamName, CFile::modeWrite ) );

	albumMap.SeekToEnd();		// will append text
	albumMap.WriteThumbnailTotals( thumbCount, totalThumbsSize );
}

CCachedThumbBitmap* CImageArchiveStg::LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_()
{
	ASSERT( IsOpen() );

	try
	{
		CComPtr< IStorage > pThumbsStorage;
		if ( const TCHAR* pThumbsDirName = FindAlternate_DirName( ARRAY_PAIR( s_thumbsStorageNames ) ).first )
			pThumbsStorage = OpenDir( pThumbsDirName );

		if ( pThumbsStorage != NULL )
		{
			CScopedCurrentDir scopedThumbsDir( this, pThumbsStorage );

			if ( CComPtr< IStream > pThumbStream = OpenThumbnailImageStream( imageComplexPath.GetEmbeddedPathPtr() ) )
				if ( CComPtr< IWICBitmapSource > pSavedBitmap = wic::LoadBitmapFromStream( pThumbStream ) )
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
	return NULL;
}

CComPtr< IStream > CImageArchiveStg::OpenThumbnailImageStream( const TCHAR* pImageEmbeddedPath )
{
	fs::TEmbeddedPath thumbStreamName;
	MakeThumbStreamName( thumbStreamName, pImageEmbeddedPath );

	const TCHAR* pThumbStreamName = thumbStreamName.GetPtr();

	if ( !path::EquivalentPtr( pThumbStreamName, pImageEmbeddedPath ) )		// possibly an archive build with Slider_v5_2-
		if ( StreamExist( pImageEmbeddedPath ) )			// backwards compatibility: also try with straight SRC image path as stream name
			pThumbStreamName = pImageEmbeddedPath;

	if ( NULL == pThumbStreamName )
		return NULL;

	return OpenStream( pThumbStreamName );
}

wic::ImageFormat CImageArchiveStg::MakeThumbStreamName( fs::TEmbeddedPath& rThumbStreamName, const TCHAR* pSrcImagePath )
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

void CImageArchiveStg::SaveAlbumDoc( CObject* pAlbumDoc )
{
	ASSERT_PTR( pAlbumDoc );

	std::auto_ptr< COleStreamFile > pAlbumFile( CreateStreamFile( s_albumStreamName ) );

	if ( NULL == pAlbumFile.get() )
		return;

	CArchive archive( pAlbumFile.get(), CArchive::store );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
	pAlbumDoc->Serialize( archive );
}

bool CImageArchiveStg::LoadAlbumDoc( CObject* pAlbumDoc )
{
	ASSERT_PTR( pAlbumDoc );
	std::auto_ptr< COleStreamFile > pAlbumFile;
	{
		CScopedErrorHandling scopedCheck( this, utl::CheckMode );			// album not found is not an error
		pAlbumFile = OpenStreamFile( s_albumStreamName );					// load stream "_Album.sld"
	}

	if ( NULL == pAlbumFile.get() )
		return false;

	CArchive archive( pAlbumFile.get(), CArchive::load );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
	pAlbumDoc->Serialize( archive );
	return true;
}

bool CImageArchiveStg::HasImageArchiveExt( const TCHAR* pFilePath )
{
	return
		path::MatchExt( pFilePath, s_compoundStgExts[ Ext_ias ] ) ||
		path::MatchExt( pFilePath, s_compoundStgExts[ Ext_cid ] ) ||
		path::MatchExt( pFilePath, s_compoundStgExts[ Ext_icf ] );
}

bool CImageArchiveStg::SavePassword( const std::tstring& password )
{
	ASSERT( IsOpen() );

	try
	{
		if ( const TCHAR* pFoundStreamName = FindAlternate_StreamName( ARRAY_PAIR( s_passwordStreamNames ) ).first )
			DeleteStream( pFoundStreamName );		// remove existing password stream

		if ( password.empty() )
			return true;			// no password, done

		std::auto_ptr< COleStreamFile > pPwdFile( CreateStreamFile( s_passwordStreamNames[ CurrentVer ] ) );
		if ( NULL == pPwdFile.get() )
			return false;

		std::tstring encryptedPassword = pwd::ToEncrypted( password );
		CArchive archive( pPwdFile.get(), CArchive::store );
		archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

		archive << encryptedPassword;

		archive.Close();
		//pwdFile.Close();
		return true;
	}
	catch ( CException* pExc )
	{
		app::GetUserReport().ReportError( pExc, MB_OK | MB_ICONHAND );
		return false;
	}
}

std::tstring CImageArchiveStg::LoadPassword( void )
{
	ASSERT( IsOpen() );

	std::pair< const TCHAR*, size_t > streamName = FindAlternate_StreamName( ARRAY_PAIR( s_passwordStreamNames ) );
	bool hasWidePwd = streamName.second != ( COUNT_OF( s_passwordStreamNames ) - 1 );		// found a WIDE stream?

	if ( NULL == streamName.first )
		return std::tstring();				// no password stored

	std::auto_ptr< COleStreamFile > pPwdFile = OpenStreamFile( streamName.first );
	ASSERT_PTR( pPwdFile.get() );

	std::tstring password;
	CArchive archive( pPwdFile.get(), CArchive::load );
	serial::CScopedLoadingArchive scopedLoadingArchive( archive, m_docModelSchema );

	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
	try
	{
		if ( hasWidePwd )
		{
			std::wstring widePassword;
			archive >> widePassword;
			password = pwd::ToString( widePassword.c_str() );
		}
		else
		{
			std::string ansiPassword;
			archive >> ansiPassword;
			password = pwd::ToString( ansiPassword.c_str() );
		}
	}
	catch ( CException* pExc )
	{
		app::GetUserReport().ReportError( pExc );
	}
	archive.Close();
	//pwdFile.Close();
	return pwd::ToDecrypted( password );
}


// CImageArchiveStg::CFactory implementation

void CImageArchiveStg::CFactory::Clear( void )
{
	utl::ClearOwningAssocContainerValues( m_storageMap );
}

CImageArchiveStg* CImageArchiveStg::CFactory::FindStorage( const fs::CPath& stgFilePath ) const
{
	stdext::hash_map< fs::CPath, CImageArchiveStg* >::const_iterator itFound = m_storageMap.find( stgFilePath );

	if ( itFound != m_storageMap.end() )
		return itFound->second;

	if ( CStructuredStorage* pOpenedStorage = fs::CStructuredStorage::FindOpenedStorage( stgFilePath ) )		// opened in testing?
		return checked_static_cast< CImageArchiveStg* >( pOpenedStorage );

	return NULL;
}

CImageArchiveStg* CImageArchiveStg::CFactory::AcquireStorage( const fs::CPath& stgFilePath, DWORD mode /*= STGM_READ*/ )
{
	if ( CImageArchiveStg* pImageStg = FindStorage( stgFilePath ) )
		return pImageStg;					// cache hit

	std::auto_ptr< CImageArchiveStg > imageArchiveStgPtr( new CImageArchiveStg );
	CScopedErrorHandling scopedHandling( imageArchiveStgPtr.get(), this );		// pass current factory throw mode to the storage

	if ( !imageArchiveStgPtr->OpenDocFile( stgFilePath.GetPtr(), mode ) )
		return NULL;

	m_storageMap[ stgFilePath ] = imageArchiveStgPtr.get();
	return imageArchiveStgPtr.release();
}

bool CImageArchiveStg::CFactory::ReleaseStorage( const fs::CPath& stgFilePath )
{
	stdext::hash_map< fs::CPath, CImageArchiveStg* >::iterator itFound = m_storageMap.find( stgFilePath );
	if ( itFound == m_storageMap.end() )
		return false;

	delete itFound->second;
	m_storageMap.erase( itFound );
	return true;
}

void CImageArchiveStg::CFactory::ReleaseStorages( const std::vector< fs::CPath >& stgFilePaths )
{
	for ( std::vector< fs::CPath >::const_iterator itStgPath = stgFilePaths.begin(); itStgPath != stgFilePaths.end(); ++itStgPath )
		CImageArchiveStg::Factory()->ReleaseStorage( *itStgPath );
}

std::auto_ptr< CFile > CImageArchiveStg::CFactory::OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode /*= CFile::modeRead*/ )
{
	std::auto_ptr< CFile > pFile;

	// could be either physical image or archive-based image file
	if ( !flexImagePath.IsComplexPath() )
		pFile = fs::OpenFile( flexImagePath, IsThrowMode(), mode );
	else
	{
		fs::CPath stgFilePath( flexImagePath.GetPhysicalPath() );
		ASSERT( IsPasswordVerified( stgFilePath ) );

		CScopedAcquireStorage stg( stgFilePath, mode );
		if ( CImageArchiveStg* pImageArchiveStg = stg.Get() )
		{
			CScopedErrorHandling scopedHandling( pImageArchiveStg, this );		// pass current factory throw mode to the storage

			ASSERT( pImageArchiveStg->IsRootCurrentDir() );
			if ( CComPtr< IStream > pImageStream = pImageArchiveStg->LocateStream( flexImagePath.GetEmbeddedPath(), mode ) )		// (?) fails when copying from deep.ias to shallow.ias
				pFile.reset( pImageArchiveStg->MakeOleStreamFile( flexImagePath.GetEmbeddedPathPtr(), pImageStream ).release() );
		}
	}

	return pFile;
}

bool CImageArchiveStg::CFactory::SavePassword( const std::tstring& password, const fs::CPath& stgFilePath )
{
	CScopedAcquireStorage stg( stgFilePath, STGM_READWRITE );					// stream creation requires STGM_WRITE access for storage
	if ( stg.Get() != NULL )
	{
		CScopedErrorHandling scopedHandling( stg.Get(), this );	// pass current factory throw mode to the storage

		if ( stg.Get()->SavePassword( password ) )
		{
			CacheVerifiedPassword( stgFilePath, password );		// for doc SaveAs
			return true;
		}
	}

	return false;
}

std::tstring CImageArchiveStg::CFactory::LoadPassword( const fs::CPath& stgFilePath )
{
	std::tstring password;
	if ( CImageArchiveStg* pArchiveStg = AcquireStorage( stgFilePath ) )
	{
		CScopedErrorHandling scopedHandling( pArchiveStg, this );		// pass current factory throw mode to the storage

		password = pArchiveStg->LoadPassword();
		if ( !password.empty() )
			m_passwordProtected[ stgFilePath ] = password;
	}
	return password;
}

bool CImageArchiveStg::CFactory::CacheVerifiedPassword( const fs::CPath& stgFilePath, const std::tstring& password )
{
	if ( password.empty() )
	{
		stdext::hash_map< fs::CPath, std::tstring >::iterator itFound = m_passwordProtected.find( stgFilePath );
		if ( itFound != m_passwordProtected.end() )
		{
			m_verifiedPasswords.erase( itFound->second );
			m_passwordProtected.erase( itFound );
		}
		return false;
	}

	m_passwordProtected[ stgFilePath ] = password;
	m_verifiedPasswords.insert( password );				// assume password was new/edited, so implicitly verified
	return true;
}

bool CImageArchiveStg::CFactory::VerifyPassword( std::tstring* pOutPassword, const fs::CPath& stgFilePath )
{
	std::tstring password = LoadPassword( stgFilePath );
	if ( !password.empty() )
		if ( m_verifiedPasswords.find( password ) == m_verifiedPasswords.end() )
		{
			CPasswordDialog dlg( AfxGetMainWnd(), &stgFilePath );
			dlg.SetVerifyPassword( password );
			if ( dlg.DoModal() != IDOK )
				return false;

			m_verifiedPasswords.insert( password );
		}

	utl::AssignPtr( pOutPassword, password );
	return true;
}

bool CImageArchiveStg::CFactory::IsPasswordVerified( const fs::CPath& stgFilePath ) const
{
	stdext::hash_map< fs::CPath, std::tstring >::const_iterator itFound = m_passwordProtected.find( stgFilePath );
	if ( itFound == m_passwordProtected.end() )
		return true;				// not password protected

	return m_verifiedPasswords.find( itFound->second ) != m_verifiedPasswords.end();
}

bool CImageArchiveStg::CFactory::SaveAlbumDoc( CObject* pAlbumDoc, const fs::CPath& stgFilePath )
{
	CScopedAcquireStorage stg( stgFilePath, STGM_READWRITE );		// stream creation requires STGM_WRITE access for storage
	if ( stg.Get() != NULL )
	{
		CScopedErrorHandling scopedHandling( stg.Get(), this );		// pass current factory throw mode to the storage

		stg.Get()->SaveAlbumDoc( pAlbumDoc );
		return true;
	}
	return false;
}

bool CImageArchiveStg::CFactory::LoadAlbumDoc( CObject* pAlbumDoc, const fs::CPath& stgFilePath )
{
	if ( IsPasswordVerified( stgFilePath ) )
		if ( CImageArchiveStg* pArchiveStg = AcquireStorage( stgFilePath ) )
		{
			CScopedErrorHandling scopedHandling( pArchiveStg, this );	// pass current factory throw mode to the storage

			return pArchiveStg->LoadAlbumDoc( pAlbumDoc );
		}

	return false;
}

bool CImageArchiveStg::CFactory::ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const
{
	return srcImagePath.IsComplexPath();
}

CCachedThumbBitmap* CImageArchiveStg::CFactory::ExtractThumb( const fs::CFlexPath& srcImagePath )
{
	if ( srcImagePath.IsComplexPath() )
	{
		fs::CPath stgFilePath = srcImagePath.GetPhysicalPath();
		REQUIRE( IsPasswordVerified( stgFilePath ) );

		if ( CImageArchiveStg* pArchiveStg = AcquireStorage( stgFilePath ) )
		{
			CScopedErrorHandling scopedCheck( pArchiveStg, utl::CheckMode );		// no exceptions for thumb extraction

			return pArchiveStg->LoadThumbnail( srcImagePath );
		}
	}
	return NULL;
}

CCachedThumbBitmap* CImageArchiveStg::CFactory::GenerateThumb( const fs::CFlexPath& srcImagePath )
{
	fs::CPath stgFilePath( srcImagePath.GetPhysicalPath() );
	ASSERT( srcImagePath.IsComplexPath() );
	ASSERT( IsPasswordVerified( stgFilePath ) );

	const CThumbnailer* pThumbnailer = safe_ptr( app::GetThumbnailer() );

	if ( CComPtr< IWICBitmapSource > pBitmapSource = CWicImageCache::Instance().LookupBitmapSource( fs::ImagePathKey( srcImagePath, 0 ) ) )		// use frame 0 for the thumbnail
		if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->NewScaledThumb( pBitmapSource, srcImagePath ) )
		{
			// IMP: the resulting bitmap, even the scaled bitmap will keep the stream alive for the lifetime of the bitmap; same if we scale the bitmap;
			// Since the thumb keeps alive the WIC bitmap, we need to copy to a memory bitmap.
			// This way we unlock the doc stg stream for future access.
			//
			TRACE_COM_PTR( pBitmapSource, "BEFORE DetachSourceToBitmap() in CImageArchiveStg::CFactory::GenerateThumb()" );
			pThumbBitmap->GetOrigin().DetachSourceToBitmap();		// release any bitmap source dependencies (IStream, HFILE, etc)
			TRACE_COM_PTR( pBitmapSource, "AFTER DetachSourceToBitmap()" );
			return pThumbBitmap;
		}

	return NULL;
}


// CAlbumMapWriter implementation

const TCHAR CImageArchiveStg::CAlbumMapWriter::s_lineFmt[] = _T("%-*s  %-*s%s");

CImageArchiveStg::CAlbumMapWriter::CAlbumMapWriter( std::auto_ptr< COleStreamFile > pAlbumMapFile )
	: fs::CTextFileWriter( pAlbumMapFile.get() )
	, m_pAlbumMapFile( pAlbumMapFile )
	, m_imageCount( 0 )
{
}

void CImageArchiveStg::CAlbumMapWriter::WriteHeader( void )
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

void CImageArchiveStg::CAlbumMapWriter::WriteEntry( const std::tstring& streamName, const TCHAR* pImageEmbeddedPath )
{
	++m_imageCount;

	WriteLine( str::Format( s_lineFmt,
		StreamNoPadding, num::FormatNumber( m_imageCount ).c_str(),
		StreamNamePadding, streamName.c_str(),
		pImageEmbeddedPath ) );
}

void CImageArchiveStg::CAlbumMapWriter::WriteImageTotals( UINT64 totalImagesSize )
{
	WriteEmptyLine();

	std::tstring fileSizePair = num::FormatFileSizeAsPair( totalImagesSize );

	WriteLine( str::Format( _T("Total images: %d"), m_imageCount ) );
	WriteLine( str::Format( _T("Total size of images: %s"), fileSizePair.c_str() ) );

	TRACE( _T("(*) Total image archive file size: %s\n"), fileSizePair.c_str() );
}

void CImageArchiveStg::CAlbumMapWriter::WriteThumbnailTotals( size_t thumbCount, UINT64 totalThumbsSize )
{
	WriteEmptyLine();

	WriteLine( str::Format( _T("Total thumbnails: %d"), thumbCount ) );
	WriteLine( str::Format( _T("Total size of thumbnails: %s"), num::FormatFileSizeAsPair( totalThumbsSize ).c_str() ) );
}


// CImageArchiveStg::CScopedAcquireStorage implementation

CImageArchiveStg::CScopedAcquireStorage::CScopedAcquireStorage( const fs::CPath& stgFilePath, DWORD mode )
	: m_stgFilePath( stgFilePath )
	, m_pArchiveStg( Factory()->FindStorage( m_stgFilePath ) )
	, m_mustRelease( false )
	, m_oldOpenMode( (DWORD)NotOpenMask )
{
	if ( NULL == m_pArchiveStg )
	{
		m_pArchiveStg = Factory()->AcquireStorage( m_stgFilePath, mode );
		if ( m_pArchiveStg != NULL )
			m_mustRelease = true;
	}
	else	// cache hit, check if it need reopening for writeable access
		if ( HasFlag( mode, WriteableModeMask ) )										// needs write or create access?
			if ( !HasFlag( m_pArchiveStg->GetOpenMode(), WriteableModeMask ) )			// was read access?
			{
				m_oldOpenMode = m_pArchiveStg->GetOpenMode();
				// close and reopen the storage in the required writeable mode
				if ( false == ( HasFlag( mode, STGM_CREATE )
						? m_pArchiveStg->CreateDocFile( m_stgFilePath.GetPtr(), mode )
						: m_pArchiveStg->OpenDocFile( m_stgFilePath.GetPtr(), mode ) ) )
				{
					m_pArchiveStg = NULL;
					Factory()->ReleaseStorage( m_stgFilePath );
				}
			}
}

CImageArchiveStg::CScopedAcquireStorage::~CScopedAcquireStorage()
{
	// restore original storage state
	if ( m_pArchiveStg != NULL )
		if ( m_mustRelease )
			Factory()->ReleaseStorage( m_stgFilePath );
		else if ( m_oldOpenMode != NotOpenMask )
			m_pArchiveStg->OpenDocFile( m_stgFilePath.GetPtr(), m_oldOpenMode );
}
