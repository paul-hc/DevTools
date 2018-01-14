
#include "StdAfx.h"
#include "ImageArchiveStg.h"
#include "InputPasswordDialog.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/Serialization.h"
#include "utl/Thumbnailer.h"
#include "utl/WicImageCache.h"
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pwd
{
	std::tstring ToString( const char* pAnsiPwd )
	{
		// straight conversion through unsigned char - not using MultiByteToWideChar()
		const unsigned char* pAnsi = (const unsigned char*)pAnsiPwd;			// (!) cast to unsigned char for proper negative sign chars
		return std::tstring( pAnsi, pAnsi + str::GetLength( pAnsiPwd ) );
	}

	std::tstring ToString( const wchar_t* pWidePwd )
	{
		return std::tstring( pWidePwd );
	}

	template< typename CharType >
	inline void Crypt( CharType& rChr )
	{
		static const TCHAR charMask = _T('\xAD');
		rChr ^= charMask;
	}
}


// CImageArchiveStg implementation

const TCHAR* CImageArchiveStg::s_compoundStgExts[ _Ext_Count ] = { _T(".ias"), _T(".cid"), _T(".icf") };
const TCHAR* CImageArchiveStg::s_pwdStreamNames[ _PwdTypeCount ] = { _T("pwd"), _T("pwdW") };
const TCHAR* CImageArchiveStg::s_thumbsSubStorageNames[ _ThumbsTypeCount ] = { _T("Thumbnails"), _T("Thumbs_jpeg") };
const TCHAR CImageArchiveStg::s_metadataNameExt[] = _T("_Meta.data");
const TCHAR CImageArchiveStg::s_albumNameExt[] = _T("_Album.sld");
const TCHAR CImageArchiveStg::s_subPathSep = _T('*');


CImageArchiveStg::CImageArchiveStg( IStorage* pRootStorage /*= NULL*/ )
	: fs::CStructuredStorage( pRootStorage )
	, m_pThumbsDecoderId( NULL )
{
}

CImageArchiveStg::CFactory& CImageArchiveStg::Factory( void )
{
	static CFactory factory;
	return factory;
}

void CImageArchiveStg::Close( void )
{
	const fs::CPath& stgFilePath = GetDocFilePath();
	if ( !stgFilePath.IsEmpty() )
		CWicImageCache::Instance().DiscardWithPrefix( stgFilePath.GetPtr() );		// discard cached images based on this storage

	if ( m_pThumbsStorage != NULL )
		m_pThumbsStorage = NULL;

	fs::CStructuredStorage::Close();
}

CStringW CImageArchiveStg::EncodeStreamName( const TCHAR* pStreamName ) const
{
	CStringW streamName( pStreamName );

	wchar_t* pBuffer = streamName.GetBuffer();
	EncodeDeepStreamPath( pBuffer, pBuffer + streamName.GetLength() );
	streamName.ReleaseBuffer();

	streamName = fs::CStructuredStorage::EncodeStreamName( streamName );
	return streamName;
}

void CImageArchiveStg::CreateImageArchive( const TCHAR* pStgFilePath, const std::tstring& password, const std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& filePairs,
										   CObject* pAlbumDoc ) throws_( CException* )
{
	CPushThrowMode pushThrow( this, true );

	Create( pStgFilePath );
	ASSERT( IsOpen() );

	CWaitCursor wait;
	std::vector< CFileAttr > fileAttributes;			// image storage metadata

	SavePassword( password );
	CreateImageFiles( fileAttributes, filePairs );
	CreateMetadataFile( fileAttributes );
	CreateThumbnailsStorage( filePairs );

	if ( pAlbumDoc != NULL )
		SaveAlbumDoc( pAlbumDoc );
}

void CImageArchiveStg::CreateImageFiles( std::vector< CFileAttr >& rFileAttributes, const std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& filePairs ) throws_( CException* )
{
	rFileAttributes.reserve( filePairs.size() );

	CPushThrowMode pushThrow( &Factory(), true );
	app::CScopedProgress progress( 0, (int)filePairs.size(), 1, _T("Save image files:") );

	for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::const_iterator it = filePairs.begin(); it != filePairs.end(); )
	{
		if ( !it->first.FileExist() )
		{
			CFileException error( CFileException::fileNotFound, -1, it->first.GetPtr() );		// source image doesn't exist
			app::GetUserReport().ReportError( &error, MB_OK | MB_ICONWARNING );					// just a warning, keep going
		}
		else
			try
			{
				CFileAttr fileAttr;
				fileAttr.SetFromTransferPair( it->first, it->second );

				std::tstring& rStreamName = const_cast< std::tstring& >( fileAttr.GetPath().Get() );
				EncodeDeepStreamPath( rStreamName.begin(), rStreamName.end() );

				std::auto_ptr< CFile > pSrcImageFile( Factory().OpenFlexImageFile( it->first ) );
				std::auto_ptr< COleStreamFile > pDestStreamFile( CreateFile( it->second.GetPtr() ) );
				fs::BufferedCopy( *pDestStreamFile, *pSrcImageFile );

				rFileAttributes.push_back( fileAttr );

				progress.StepIt();
			}
			catch ( CException* pExc )
			{
				switch ( app::GetUserReport().ReportError( pExc, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
				{
					case IDABORT:	throw new mfc::CUserAbortedException;
					case IDRETRY:	continue;
					case IDIGNORE:	break;
				}
			}

		++it;
	}

#ifdef _DEBUG
	size_t totalImagesSize = std::accumulate( rFileAttributes.begin(), rFileAttributes.end(), size_t( 0 ), func::AddFileSize() );
	TRACE( _T("(*) Total image archive file size: %d = %d KB\n"), totalImagesSize, totalImagesSize / KiloByte );
#endif
}

void CImageArchiveStg::CreateMetadataFile( const std::vector< CFileAttr >& fileAttributes )
{
	std::auto_ptr< COleStreamFile > pMetaDataFile( CreateFile( CImageArchiveStg::s_metadataNameExt ) );
	CArchive archive( pMetaDataFile.get(), CArchive::store );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

	// save metadata
	size_t totalImagesSize = std::accumulate( fileAttributes.begin(), fileAttributes.end(), size_t( 0 ), func::AddFileSize() );
	archive << static_cast< UINT >( totalImagesSize );		// for backwards compatibility
	serial::StreamItems( archive, const_cast< std::vector< CFileAttr >& >( fileAttributes ) );

	archive.Close();
	pMetaDataFile->Close();
}

void CImageArchiveStg::CreateThumbnailsStorage( const std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& filePairs )
{
	CComPtr< IStorage > pThumbsStorage = CreateDir( CImageArchiveStg::s_thumbsSubStorageNames[ Thumbs_jpeg ] );
	ASSERT_PTR( pThumbsStorage );

	app::CScopedProgress progress( 0, (int)filePairs.size(), 1, _T("Save thumbnails:") );

	CThumbnailer* pThumbnailer = app::GetThumbnailer();
	thumb::CPushBoundsSize largerBounds( pThumbnailer, 128 );			// generate larger thumbs to minimize regeneration later

	for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::const_iterator it = filePairs.begin(); it != filePairs.end(); )
	{
		if ( it->first.FileExist() )
			try
			{
				if ( CCachedThumbBitmap* pThumbBitmap = pThumbnailer->AcquireThumbnail( it->first ) )
				{
					CComPtr< IStream > pThumbStream = CreateStream( it->second.GetPtr(), pThumbsStorage );
					CPushThrowMode pushThrow( &pThumbBitmap->GetOrigin(), true );
					pThumbBitmap->GetOrigin().SaveBitmapToStream( pThumbStream, CThumbnailer::s_containerFormatId );
				}
				else
					throw new mfc::CRuntimeException( str::Format( _T("Cannot generate a thumbnail for file:\n%s"), it->first.GetPtr() ) );
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

		++it;
		progress.StepIt();
	}
}

void CImageArchiveStg::LoadImagesMetadata( std::vector< CFileAttr >& rFileAttributes )
{
	ASSERT( IsOpen() );

	std::auto_ptr< COleStreamFile > pMetaDataFile( OpenFile( s_metadataNameExt ) );
	if ( pMetaDataFile.get() != NULL )
	{
		CArchive archive( pMetaDataFile.get(), CArchive::load );
		archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

		try
		{	// load metadata
			UINT totalImagesSize;
			archive >> totalImagesSize;

			serial::StreamItems( archive, rFileAttributes );

			// convert the persisted image sub-path to a fully qualified logical path
			const fs::CPath& docFilePath = GetDocFilePath();
			for ( std::vector< CFileAttr >::iterator itFileAttr = rFileAttributes.begin(); itFileAttr != rFileAttributes.end(); ++itFileAttr )
			{
				std::tstring streamPath = itFileAttr->GetPath().Get();
				DecodeDeepStreamPath( streamPath.begin(), streamPath.end() );		// decode deep stream paths: '*' -> '\'

				itFileAttr->m_pathKey.first = fs::CFlexPath::MakeComplexPath( docFilePath.Get(), streamPath.c_str() );
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
			{
				m_pThumbsStorage = OpenDir( s_thumbsSubStorageNames[ Thumbs_jpeg ] );
				m_pThumbsDecoderId = &CLSID_WICJpegDecoder;
			}
			else if ( StorageExist( s_thumbsSubStorageNames[ Thumbs_bmp ] ) )		// backwards compatibility
			{
				m_pThumbsStorage = OpenDir( s_thumbsSubStorageNames[ Thumbs_bmp ] );
				m_pThumbsDecoderId = &CLSID_WICBmpDecoder;
			}

		if ( m_pThumbsStorage != NULL )
		{
			ASSERT_PTR( m_pThumbsDecoderId );

			if ( CComPtr< IStream > pThumbStream = OpenStream( imageComplexPath.GetEmbeddedPath(), m_pThumbsStorage ) )
				if ( CComPtr< IWICBitmapSource > pSavedBitmap = wic::LoadBitmapFromStream( pThumbStream, *m_pThumbsDecoderId ) )
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
	std::auto_ptr< COleStreamFile > pAlbumFile( CreateFile( CImageArchiveStg::s_albumNameExt ) );
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
		pAlbumFile.reset( OpenFile( CImageArchiveStg::s_albumNameExt ) );
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

		CArchive archive( pPwdFile.get(), CArchive::store );
		archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()

		archive << password;

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
	PwdFmt pwdFmt = PwdWide;

	std::auto_ptr< COleStreamFile > pPwdFile;
	{
		CPushThrowMode pushNoThrow( this, false );
		pPwdFile.reset( OpenFile( s_pwdStreamNames[ PwdWide ] ) );
		if ( pPwdFile.get() != NULL )
			pwdFmt = PwdWide;				// found WIDE password
		else
		{
			pPwdFile.reset( OpenFile( s_pwdStreamNames[ PwdAnsi ] ) );
			if ( pPwdFile.get() != NULL )
				pwdFmt = PwdAnsi;			// found ANSI password (backwards compatibility)
			else
				return password;			// no password stored
		}
	}

	CArchive archive( pPwdFile.get(), CArchive::load );
	archive.m_bForceFlat = FALSE;			// same as CDocument::OnOpenDocument()
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
	return password;			// encrypted
}

void CImageArchiveStg::EncryptPassword( std::tstring& rPassword )
{
	std::reverse( rPassword.begin(), rPassword.end() );

	for ( size_t i = 0; i != rPassword.size(); ++i )
		pwd::Crypt( rPassword[ i ] );
}

void CImageArchiveStg::DecryptPassword( std::tstring& rPassword )
{
	for ( size_t i = 0; i != rPassword.size(); ++i )
		pwd::Crypt( rPassword[ i ] );

	std::reverse( rPassword.begin(), rPassword.end() );
}


// CImageArchiveStg::CFactory implementation

void CImageArchiveStg::CFactory::Clear( void )
{
	utl::ClearOwningAssocContainerValues( m_storageMap );
}

CImageArchiveStg* CImageArchiveStg::CFactory::FindStorage( const fs::CPath& stgFilePath ) const
{
	stdext::hash_map< fs::CPath, CImageArchiveStg* >::const_iterator itFound = m_storageMap.find( stgFilePath );
	return itFound != m_storageMap.end() ? itFound->second : NULL;
}

CImageArchiveStg* CImageArchiveStg::CFactory::AcquireStorage( const fs::CPath& stgFilePath, DWORD mode /*= STGM_READ*/ )
{
	if ( CImageArchiveStg* pImageStg = FindStorage( stgFilePath ) )
		return pImageStg;					// cache hit

	std::auto_ptr< CImageArchiveStg > imageArchiveStgPtr( new CImageArchiveStg );
	CPushThrowMode pushMode( imageArchiveStgPtr.get(), IsThrowMode() );		// pass current factory throw mode to the storage
	if ( !imageArchiveStgPtr->Open( stgFilePath.GetPtr(), mode ) )
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
		CImageArchiveStg::Factory().ReleaseStorage( *itStgPath );
}

CFile* CImageArchiveStg::CFactory::OpenFlexImageFile( const fs::CFlexPath& flexImagePath, DWORD mode /*= CFile::modeRead*/ )
{
	// could be either physical image or archive-based image file
	if ( !flexImagePath.IsComplexPath() )
	{
		static mfc::CAutoException< CFileException > fileError;
		fileError.m_strFileName = flexImagePath.GetPtr();

		std::auto_ptr< CFile > pImageFile( new CFile );
		if ( !pImageFile->Open( flexImagePath.GetPtr(), mode | CFile::shareDenyWrite, &fileError ) )		// note: CFile::shareExclusive causes sharing violation
			if ( IsThrowMode() )
				throw &fileError;
			else
				return NULL;

		return pImageFile.release();
	}

	fs::CPath stgFilePath( flexImagePath.GetPhysicalPath() );
	ASSERT( IsPasswordVerified( stgFilePath ) );

	CScopedAcquireStorage stg( stgFilePath, mode );
	if ( stg.Get() != NULL )
	{
		CPushThrowMode pushMode( stg.Get(), IsThrowMode() );							// pass current factory throw mode to the storage
		return stg.Get()->OpenFile( flexImagePath.GetEmbeddedPath(), NULL, mode );		// OpenDeepFile fails when copying from deep.ias to shallow.ias
	}
	return NULL;
}

void CImageArchiveStg::CFactory::LoadImagesMetadata( std::vector< CFileAttr >& rFileAttributes, const fs::CPath& stgFilePath )
{
	ASSERT( IsPasswordVerified( stgFilePath ) );

	if ( CImageArchiveStg* pArchiveStg = AcquireStorage( stgFilePath ) )
	{
		CPushThrowMode pushMode( pArchiveStg, IsThrowMode() );					// pass current factory throw mode to the storage
		pArchiveStg->LoadImagesMetadata( rFileAttributes );
	}
}

bool CImageArchiveStg::CFactory::SavePassword( const std::tstring& password, const fs::CPath& stgFilePath )
{
	CScopedAcquireStorage stg( stgFilePath, STGM_READWRITE );					// stream creation requires STGM_WRITE access for storage
	if ( stg.Get() != NULL )
	{
		CPushThrowMode pushMode( stg.Get(), IsThrowMode() );					// pass current factory throw mode to the storage

		if ( !password.empty() )
			m_passwordProtected[ stgFilePath ] = password;

		if ( stg.Get()->SavePassword( password ) )
		{
			m_verifiedPasswords.insert( password );
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

bool CImageArchiveStg::CFactory::VerifyPassword( const fs::CPath& stgFilePath )
{
	std::tstring password = LoadPassword( stgFilePath );
	if ( !password.empty() )
		if ( m_verifiedPasswords.find( password ) == m_verifiedPasswords.end() )
		{
			CInputPasswordDialog dlg( stgFilePath.GetNameExt(), AfxGetMainWnd() );

			for ( ;; )
			{
				if ( dlg.DoModal() != IDOK )
					return false;

				CImageArchiveStg::EncryptPassword( dlg.m_password );
				if ( dlg.m_password == password )
					break;
				else
				{
					app::GetApp()->SetStatusBarMessage( _T("Password is incorrect!") );
					MessageBeep( MB_ICONERROR );
				}
			}
			m_verifiedPasswords.insert( password );
		}

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
		fs::CPath stgFilePath( srcImagePath.GetPhysicalPath() );
		ASSERT( IsPasswordVerified( stgFilePath ) );
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
	, m_pArchiveStg( Factory().FindStorage( m_stgFilePath ) )
	, m_mustRelease( false )
	, m_oldOpenMode( (DWORD)NotOpenMask )
{
	if ( NULL == m_pArchiveStg )
	{
		m_pArchiveStg = Factory().AcquireStorage( m_stgFilePath, mode );
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
						? m_pArchiveStg->Create( m_stgFilePath.GetPtr(), mode )
						: m_pArchiveStg->Open( m_stgFilePath.GetPtr(), mode ) ) )
				{
					m_pArchiveStg = NULL;
					Factory().ReleaseStorage( m_stgFilePath );
				}
			}
}

CImageArchiveStg::CScopedAcquireStorage::~CScopedAcquireStorage()
{
	// restore original storage state
	if ( m_pArchiveStg != NULL )
		if ( m_mustRelease )
			Factory().ReleaseStorage( m_stgFilePath );
		else if ( m_oldOpenMode != NotOpenMask )
			m_pArchiveStg->Open( m_stgFilePath.GetPtr(), m_oldOpenMode );
}
