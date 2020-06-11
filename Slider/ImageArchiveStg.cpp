
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

const TCHAR* CImageArchiveStg::s_compoundStgExts[ _Ext_Count ] = { _T(".ias"), _T(".cid"), _T(".icf") };
const TCHAR* CImageArchiveStg::s_pwdStreamNames[ _PwdTypeCount ] = { _T("pwd"), _T("pwdW") };
const TCHAR* CImageArchiveStg::s_thumbsSubStorageNames[ _ThumbsTypeCount ] = { _T("Thumbnails"), _T("Thumbs_jpeg") };
const TCHAR CImageArchiveStg::s_metadataFilename[] = _T("_Meta.data");
const TCHAR CImageArchiveStg::s_albumFilename[] = _T("_Album.sld");
const TCHAR CImageArchiveStg::s_subPathSep = _T('|');		// was '*'


CImageArchiveStg::CImageArchiveStg( IStorage* pRootStorage /*= NULL*/ )
	: fs::CStructuredStorage( pRootStorage )
	, m_docModelSchema( app::Slider_LatestModelSchema )
{
}

CImageArchiveStg::~CImageArchiveStg()
{
	Close();		// close early so that virtual methods are called properly
}

CImageArchiveStg::CFactory* CImageArchiveStg::Factory( void )
{
	static CFactory s_factory;
	return &s_factory;
}

void CImageArchiveStg::Close( void )
{
	DiscardCachedImages( GetDocFilePath() );
	m_pThumbsStorage = NULL;

	__super::Close();
}

void CImageArchiveStg::DiscardCachedImages( const fs::CPath& stgFilePath )
{
	if ( !stgFilePath.IsEmpty() )
		CWicImageCache::Instance().DiscardWithPrefix( stgFilePath.GetPtr() );		// discard cached images for the storage
}

std::tstring CImageArchiveStg::EncodeStreamName( const TCHAR* pStreamName ) const
{
	std::tstring streamName = pStreamName;

	FlattenDeepStreamPath( streamName );
	return __super::EncodeStreamName( streamName.c_str() );
}

TCHAR CImageArchiveStg::GetSubPathSep( void ) const
{
	if ( m_docModelSchema < app::Slider_v5_2 )
		return _T('*');			// use old separator for backwards compatibility (the short 31 characters hash suffix uses the old separator)

	return s_subPathSep;
}

void CImageArchiveStg::CreateImageArchive( const fs::CPath& stgFilePath, CImageStorageService* pImagesSvc ) throws_( CException* )
{
	ASSERT_PTR( pImagesSvc );

	CPushThrowMode pushThrow( this, true );

	CreateDocFile( stgFilePath.GetPtr() );
	ASSERT( IsOpen() );

	CWaitCursor wait;

	SavePassword( pImagesSvc->GetPassword() );
	Factory()->CacheVerifiedPassword( stgFilePath, pImagesSvc->GetPassword() );

	CreateImageFiles( pImagesSvc );
	CreateMetadataFile( pImagesSvc->GetTransferAttrs() );
	CreateThumbnailsSubStorage( pImagesSvc );
}

void CImageArchiveStg::CreateImageFiles( CImageStorageService* pImagesSvc ) throws_( CException* )
{
	// Prevent sharing violations on SRC stream open.
	//	2020-04-11: Still doesn't work, I get exception on open. I suspect the source stream (image file) must be kept open with CFile::shareExclusive by some WIC indirect COM interface.
	if ( !pImagesSvc->IsEmpty() )
		app::GetThumbnailer()->Clear();		// also discard the thumbs - note: cached SRC images were discarded by CImageStorageService::Build()

	std::vector< CTransferFileAttr* >& rTransferAttrs = pImagesSvc->RefTransferAttrs();

	CPushThrowMode pushThrow( Factory(), true );
	CFactory* pFactory = Factory();

	pImagesSvc->GetProgress()->AdvanceStage( _T("Saving embedded image files") );
	pImagesSvc->GetProgress()->SetBoundedProgressCount( rTransferAttrs.size() );

	for ( size_t pos = 0; pos != rTransferAttrs.size(); )
	{
		CTransferFileAttr* pXferAttr = rTransferAttrs[ pos ];

		pImagesSvc->GetProgress()->AdvanceItem( pXferAttr->GetPath().Get() );

		try
		{
			std::tstring& rStreamName = const_cast< std::tstring& >( pXferAttr->GetPath().Get() );
			FlattenDeepStreamPath( rStreamName );

			std::auto_ptr< CFile > pSrcImageFile = pFactory->OpenFlexImageFile( pXferAttr->GetSrcImagePath() );
			std::auto_ptr< COleStreamFile > pDestStreamFile( CreateFile( pXferAttr->GetPath().GetPtr() ) );

			fs::BufferedCopy( *pDestStreamFile, *pSrcImageFile );
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

#ifdef _DEBUG
	size_t totalImagesSize = std::accumulate( rTransferAttrs.begin(), rTransferAttrs.end(), size_t( 0 ), func::AddFileSize() );
	TRACE( _T("(*) Total image archive file size: %s (%s)\n"), num::FormatFileSize( totalImagesSize ).c_str(), num::FormatFileSize( totalImagesSize, num::Bytes ).c_str() );
#endif
}

void CImageArchiveStg::CreateMetadataFile( const std::vector< CTransferFileAttr* >& transferAttrs )
{
	std::auto_ptr< COleStreamFile > pMetaDataFile( CreateFile( CImageArchiveStg::s_metadataFilename ) );
	CArchive archive( pMetaDataFile.get(), CArchive::store );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

	// save metadata
	size_t totalImagesSize = std::accumulate( transferAttrs.begin(), transferAttrs.end(), size_t( 0 ), func::AddFileSize() );
	archive << static_cast< UINT >( totalImagesSize );		// for backwards compatibility
	serial::SaveOwningPtrs( archive, transferAttrs );		// stream as CFileAttr

	archive.Close();
	pMetaDataFile->Close();
}

void CImageArchiveStg::CreateThumbnailsSubStorage( const CImageStorageService* pImagesSvc )
{
	const std::vector< CTransferFileAttr* >& transferAttrs = pImagesSvc->GetTransferAttrs();

	pImagesSvc->GetProgress()->AdvanceStage( _T("Saving thumbnails") );
	pImagesSvc->GetProgress()->SetBoundedProgressCount( transferAttrs.size() );

	CComPtr< IStorage > pThumbsStorage = CreateDir( CImageArchiveStg::s_thumbsSubStorageNames[ Thumbs_jpeg ] );
	ASSERT_PTR( pThumbsStorage );

	CThumbnailer* pThumbnailer = app::GetThumbnailer();
	thumb::CPushBoundsSize largerBounds( pThumbnailer, 128 );			// generate larger thumbs to minimize regeneration later

	for ( std::vector< CTransferFileAttr* >::const_iterator itXferAttr = transferAttrs.begin(); itXferAttr != transferAttrs.end(); )
	{
		try
		{
			if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->AcquireThumbnail( ( *itXferAttr )->GetSrcImagePath() ) )
			{
				fs::CPath thumbStreamName;
				const GUID* pContainerFormatId = wic::GetContainerFormatId( MakeThumbStreamName( thumbStreamName, ( *itXferAttr )->GetPath().GetPtr() ) );
				ASSERT_PTR( pContainerFormatId );

				CComPtr< IStream > pThumbStream = CreateStream( thumbStreamName.GetPtr(), pThumbsStorage );
				CPushThrowMode pushThrow( &pThumbBitmap->GetOrigin(), true );

				pThumbBitmap->GetOrigin().SaveBitmapToStream( pThumbStream, *pContainerFormatId );
			}
			else
				throw new mfc::CRuntimeException( str::Format( _T("Cannot generate a thumbnail for file:\n%s"), ( *itXferAttr )->GetSrcImagePath().GetPtr() ) );
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
}

wic::ImageFormat CImageArchiveStg::MakeThumbStreamName( fs::CPath& rThumbStreamName, const TCHAR* pSrcImagePath )
{
	// GUID_ContainerFormatJpeg or GUID_ContainerFormatPng (for transparent formats)
	// Note: for transparency we can't rely on CWicBitmap::GetBmpFmt().m_hasAlphaChannel, as is true even for a true color .jpg
	// We have to infer the thumb save format from the source file extension.

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

void CImageArchiveStg::LoadImagesMetadata( std::vector< CFileAttr* >& rOutTransferAttrs )
{
	ASSERT( IsOpen() );

	std::auto_ptr< COleStreamFile > pMetaDataFile( OpenFile( s_metadataFilename ) );
	if ( pMetaDataFile.get() != NULL )
	{
		CArchive archive( pMetaDataFile.get(), CArchive::load );
		archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

		serial::CScopedLoadingArchive scopedLoadingArchive( archive, m_docModelSchema );

		try
		{	// load metadata
			UINT totalImagesSize;
			archive >> totalImagesSize;

			serial::StreamOwningPtrs( archive, rOutTransferAttrs );

			// convert the persisted image sub-path to a fully qualified logical path
			const fs::CPath& docFilePath = GetDocFilePath();
			for ( std::vector< CFileAttr* >::iterator itFileAttr = rOutTransferAttrs.begin(); itFileAttr != rOutTransferAttrs.end(); ++itFileAttr )
			{
				std::tstring streamPath = ( *itFileAttr )->GetPath().Get();
				UnflattenDeepStreamPath( streamPath );		// decode deep stream paths: '|' -> '\'

				( *itFileAttr )->SetPath( fs::CFlexPath::MakeComplexPath( docFilePath.Get(), streamPath.c_str() ) );
			}
		}
		catch ( CException* pExc )
		{
			app::HandleException( pExc );
		}
	}
}

CCachedThumbBitmap* CImageArchiveStg::LoadThumbnail( const fs::CFlexPath& imageComplexPath ) throws_()
{
	ASSERT( IsOpen() );

	try
	{
		if ( NULL == m_pThumbsStorage )
			if ( StorageExist( s_thumbsSubStorageNames[ Thumbs_jpeg ] ) )
				m_pThumbsStorage = OpenDir( s_thumbsSubStorageNames[ Thumbs_jpeg ] );
			else if ( StorageExist( s_thumbsSubStorageNames[ Thumbs_bmp ] ) )		// backwards compatibility
				m_pThumbsStorage = OpenDir( s_thumbsSubStorageNames[ Thumbs_bmp ] );

		if ( m_pThumbsStorage != NULL )
		{
			fs::CPath thumbStreamName;
			MakeThumbStreamName( thumbStreamName, imageComplexPath.GetEmbeddedPath() );

			CComPtr< IStream > pThumbStream = OpenStream( thumbStreamName.GetPtr(), m_pThumbsStorage );
			if ( pThumbStream == NULL )
				pThumbStream = OpenStream( imageComplexPath.GetEmbeddedPath(), m_pThumbsStorage );		// backwards compatibility: try with straight SRC image path as stream name

			if ( pThumbStream != NULL )
				if ( CComPtr< IWICBitmapSource > pSavedBitmap = wic::LoadBitmapFromStream( pThumbStream/*, m_pThumbsDecoderId*/ ) )
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

void CImageArchiveStg::SaveAlbumDoc( CObject* pAlbumDoc )
{
	ASSERT_PTR( pAlbumDoc );

	std::auto_ptr< COleStreamFile > pAlbumFile( CreateFile( CImageArchiveStg::s_albumFilename ) );

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
		CPushThrowMode pushNoThrow( this, false );				// album not found is not an error
		pAlbumFile = OpenFile( CImageArchiveStg::s_albumFilename );		// load stream "_Album.sld"
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
		// remove existing password streams
		if ( StreamExist( s_pwdStreamNames[ PwdAnsi ] ) )
			DeleteStream( s_pwdStreamNames[ PwdAnsi ] );
		if ( StreamExist( s_pwdStreamNames[ PwdWide ] ) )
			DeleteStream( s_pwdStreamNames[ PwdWide ] );

		if ( password.empty() )
			return true;			// no password, done

		std::auto_ptr< COleStreamFile > pPwdFile( CreateFile( s_pwdStreamNames[ PwdWide ] ) );
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

	std::tstring password;
	PwdFmt pwdFmt;

	if ( StreamExist( s_pwdStreamNames[ PwdWide ] ) )
		pwdFmt = PwdWide;				// has WIDE password
	else if ( StreamExist( s_pwdStreamNames[ PwdAnsi ] ) )
		pwdFmt = PwdAnsi;				// has ANSI password (backwards compatibility)
	else
		return password;				// no password stored

	std::auto_ptr< COleStreamFile > pPwdFile = OpenFile( s_pwdStreamNames[ pwdFmt ] );
	ASSERT_PTR( pPwdFile.get() );

	CArchive archive( pPwdFile.get(), CArchive::load );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

	serial::CScopedLoadingArchive scopedLoadingArchive( archive, m_docModelSchema );

	try
	{
		if ( PwdWide == pwdFmt )
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
	CPushThrowMode pushMode( imageArchiveStgPtr.get(), IsThrowMode() );		// pass current factory throw mode to the storage
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
		if ( stg.Get() != NULL )
		{
			CPushThrowMode pushMode( stg.Get(), IsThrowMode() );							// pass current factory throw mode to the storage
			pFile.reset( stg.Get()->OpenFile( flexImagePath.GetEmbeddedPath(), NULL, mode ).release() );		// OpenDeepFile fails when copying from deep.ias to shallow.ias
		}
	}

	return pFile;
}

void CImageArchiveStg::CFactory::LoadImagesMetadata( std::vector< CFileAttr* >& rOutTransferAttrs, const fs::CPath& stgFilePath )
{
	ASSERT( IsPasswordVerified( stgFilePath ) );

	if ( CImageArchiveStg* pArchiveStg = AcquireStorage( stgFilePath ) )
	{
		CPushThrowMode pushMode( pArchiveStg, IsThrowMode() );					// pass current factory throw mode to the storage
		pArchiveStg->LoadImagesMetadata( rOutTransferAttrs );
	}
}

bool CImageArchiveStg::CFactory::SavePassword( const std::tstring& password, const fs::CPath& stgFilePath )
{
	CScopedAcquireStorage stg( stgFilePath, STGM_READWRITE );					// stream creation requires STGM_WRITE access for storage
	if ( stg.Get() != NULL )
	{
		CPushThrowMode pushMode( stg.Get(), IsThrowMode() );					// pass current factory throw mode to the storage

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
		CPushThrowMode pushMode( pArchiveStg, IsThrowMode() );		// pass current factory throw mode to the storage
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
		CPushThrowMode pushMode( stg.Get(), IsThrowMode() );		// pass current factory throw mode to the storage
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
			CPushThrowMode pushMode( pArchiveStg, IsThrowMode() );		// pass current factory throw mode to the storage
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
			CPushThrowMode pushMode( pArchiveStg, false );				// no exceptions for thumb extraction
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
			TRACE_COM_PTR( pBitmapSource );
			pThumbBitmap->GetOrigin().DetachSourceToBitmap();		// release any bitmap source dependencies (IStream, HFILE, etc)
			TRACE_COM_PTR( pBitmapSource );
			return pThumbBitmap;
		}

	return NULL;
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
