
#include "stdafx.h"
#include "StructuredStorage.h"
#include "StorageTrail.h"
#include "FlexPath.h"
#include "StringUtilities.h"
#include "utl/AppTools.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CStructuredStorage implementation

	mfc::CAutoException< CFileException > CStructuredStorage::s_fileError;

	CStructuredStorage::CStructuredStorage( void )
		: CErrorHandler( utl::CheckMode )
		, m_useStreamSharing( false )			// by default there is no need for shared storage reading access
		, m_openMode( 0 )
	{
	}

	CStructuredStorage::~CStructuredStorage()
	{
		Close();
	}

	void CStructuredStorage::SetUseStreamSharing( bool useStreamSharing /*= true*/ )
	{
		m_useStreamSharing = useStreamSharing;
		m_openedStreamStates.Clear();
	}

	const fs::CPath& CStructuredStorage::GetDocFilePath( void ) const
	{
		if ( m_docFilePath.IsEmpty() && IsOpen() )
			m_docFilePath = fs::stg::GetElementName( &*m_pRootStorage );

		return m_docFilePath;
	}

	DWORD CStructuredStorage::ToMode( DWORD mode )
	{
		enum { AccessMask = 0x0000000F, ShareMask = 0x000000F0 };		// covers for STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE

		if ( HasFlag( mode, ShareMask ) )
			return mode;

		return mode | STGM_SHARE_EXCLUSIVE;		// MSDN: compound-file storages/streams must be opened at least with STGM_SHARE_EXCLUSIVE
	}

	std::tstring CStructuredStorage::MakeShortFilename( const TCHAR* pFilename )
	{
		// pFilename could refer to a dir, file, or a sub-path;
		// encode with a hashed suffix if pFilename longer than the stream limit (compound document limitation).

		return path::MakeShortHashedFilename( pFilename, MaxFilenameLen );
	}


	void CStructuredStorage::Close( void )
	{
		if ( !IsOpen() )
			return;

		UnregisterDocStg( this );

		m_pRootStorage = NULL;			// release the storage
		m_openMode = 0;
		m_docFilePath.Clear();
		m_openedStreamStates.Clear();
	}

	bool CStructuredStorage::CreateDocFile( const TCHAR* pDocFilePath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		if ( IsOpen() )
			Close();

		mode = ToMode( mode );

		HRESULT hResult = HR_AUDIT( ::StgCreateStorageEx( pDocFilePath, mode, STGFMT_STORAGE, 0, NULL, 0, IID_PPV_ARGS( &m_pRootStorage ) ) );
		//HRESULT hResult = HR_AUDIT( ::StgCreateDocfile( pDocFilePath, mode, 0, &m_pRootStorage ) );		// old API NT4-

		if ( FAILED( hResult ) )
			return HandleError( hResult, NULL, pDocFilePath );

		m_openMode = mode;
		GetDocFilePath();		// cache the sotrage path
		RegisterDocStg( this );
		return true;
	}

	bool CStructuredStorage::OpenDocFile( const TCHAR* pDocFilePath, DWORD mode /*= STGM_READ*/ )
	{
		if ( IsOpen() )
			Close();

		mode = ToMode( mode );

		HRESULT hResult = HR_AUDIT( ::StgOpenStorageEx( pDocFilePath, mode, STGFMT_STORAGE, 0, NULL, 0, IID_PPV_ARGS( &m_pRootStorage ) ) );
		//HRESULT hResult = HR_AUDIT( ::StgOpenStorage( pDocFilePath, NULL, mode, NULL, 0, &m_pRootStorage ) );		// old API NT4-

		if ( FAILED( hResult ) )
			return HandleError( hResult, NULL, pDocFilePath );

		m_openMode = mode;
		GetDocFilePath();		// cache the sotrage path
		RegisterDocStg( this );
		return true;
	}

	bool CStructuredStorage::IsValidDocFile( const TCHAR* pDocFilePath )
	{
		HRESULT hResult = ::StgIsStorageFile( pDocFilePath );
		return S_OK == hResult;
	}


	CComPtr< IStorage > CStructuredStorage::CreateDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->CreateStorage( storageName.c_str(), ToMode( mode ), 0, 0, &pDirStorage );

		if ( FAILED( hResult ) )
			HandleError( hResult, pDirName );
		return pDirStorage;
	}

	CComPtr< IStorage > CStructuredStorage::OpenDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStorage( storageName.c_str(), NULL, ToMode( mode ), NULL, 0, &pDirStorage );

		if ( FAILED( hResult ) )
			HandleError( hResult, pDirName );

		return pDirStorage;
	}

	bool CStructuredStorage::DeleteDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/ )
	{
		std::tstring storageName = MakeShortFilename( pDirName );
		fs::TEmbeddedPath dirSubPath = MakeElementSubPath( pDirName, pParentDir );

		CloseStreamsWithPrefix( pDirName );		// deep delete cached stream states

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( storageName.c_str() );
		if ( FAILED( hResult ) )
			return HandleError( hResult, pDirName );

		return true;
	}

	bool CStructuredStorage::StorageExist( const TCHAR* pStorageName, IStorage* pParentDir /*= NULL*/ )
	{
		CScopedErrorHandling scopedIgnore( this, utl::IgnoreMode );		// failure is not an error
		CComPtr< IStorage > pStorage = OpenDir( pStorageName, pParentDir, STGM_READ );
		return pStorage != NULL;
	}


	CComPtr< IStream > CStructuredStorage::CreateStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( IsOpen() );
		REQUIRE( !IsStreamOpen( fs::TEmbeddedPath( pStreamName ) ) );		// must have been cleaned up

		std::tstring streamName = EncodeStreamName( pStreamName );
		CComPtr< IStream > pFileStream;

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->CreateStream( streamName.c_str(), ToMode( mode ), NULL, 0, &pFileStream );
		if ( FAILED( hResult ) )
			HandleError( hResult, pStreamName );
	
		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::OpenStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring streamName = EncodeStreamName( pStreamName );
		fs::TEmbeddedPath streamPath = MakeElementSubPath( pStreamName, pParentDir );

		if ( IsReadingMode( mode ) )
		{
			if ( const fs::CStreamState* pFoundStreamState = FindOpenedStream( streamPath ) )		// stream already open for reading?
				return CloneStream( pFoundStreamState, streamPath );				// return a clone of the original opened stream
		}
		else
			m_openedStreamStates.Remove( streamPath );

		CComPtr< IStream > pFileStream;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStream( streamName.c_str(), NULL, ToMode( mode ), 0, &pFileStream );

		if ( FAILED( hResult ) )
			HandleError( hResult, pStreamName );
		else if ( IsReadingMode( mode ) )
			CacheStreamState( pStreamName, pParentDir, pFileStream );		// cache the stream metadata (when opened for reading)

		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::CloneStream( const fs::CStreamState* pStreamState, const fs::TEmbeddedPath& streamPath )
	{
		ASSERT_PTR( pStreamState );

		CComPtr< IStream > pStreamReadingDuplicate;
		HRESULT hResult = pStreamState->CloneStream( &pStreamReadingDuplicate );		// clone and rewind the stream origin

		if ( FAILED( hResult ) )
			HandleError( hResult, streamPath.GetPtr() );

		return pStreamReadingDuplicate;
	}

	bool CStructuredStorage::DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/ )
	{
		m_openedStreamStates.Remove( MakeElementSubPath( pStreamName, pParentDir ) );

		std::tstring streamName = EncodeStreamName( pStreamName );

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( streamName.c_str() );
		if ( FAILED( hResult ) )
			return HandleError( hResult, pStreamName );

		return true;
	}

	bool CStructuredStorage::CloseStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/ )
	{
		if ( fs::CStreamState* pCachedStreamState = m_openedStreamStates.Find( MakeElementSubPath( pStreamName, pParentDir ) ) )
			if ( pCachedStreamState->HasStream() )
			{
				pCachedStreamState->ReleaseStream();
				return true;
			}

		return false;
	}

	bool CStructuredStorage::StreamExist( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/ )
	{
		CScopedErrorHandling scopedIgnore( this, utl::IgnoreMode );		// failure is not an error
		CComPtr< IStream > pStream = OpenStream( pStreamName, pParentDir, STGM_READ );
		return pStream != NULL;
	}


	std::auto_ptr< COleStreamFile > CStructuredStorage::CreateStreamFile( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeCreate | CFile::modeWrite*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring streamName = EncodeStreamName( pStreamName );
		std::auto_ptr< COleStreamFile > pNewFile;

		if ( CComPtr< IStream > pNewStream = CreateStream( pStreamName, pParentDir, mode ) )		// use CreateStream() method for seamless error handling
			pNewFile = MakeOleStreamFile( streamName.c_str(), pNewStream );

		//TRACE_COM_ITF( fS::GetSafeStream( pNewFile.get() ), "CStructuredStorage::CreateStreamFile() - exiting" );

		return pNewFile;
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::OpenStreamFile( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeRead*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring streamName = EncodeStreamName( pStreamName );
		std::auto_ptr< COleStreamFile > pOpenedFile;

		if ( CComPtr< IStream > pStream = OpenStream( pStreamName, pParentDir, mode ) )			// use OpenStream() method for seamless error handling
			pOpenedFile = MakeOleStreamFile( streamName.c_str(), pStream );

		//TRACE_COM_ITF( fS::GetSafeStream( pOpenedFile.get() ), "CStructuredStorage::OpenStreamFile() - exiting" );

		return pOpenedFile;
	}


	std::pair< const TCHAR*, size_t > CStructuredStorage::FindAlternate_DirName( const TCHAR* altDirNames[], size_t altCount, IStorage* pParentDir /*= NULL*/ )
	{
		REQUIRE( altCount != 0 );

		for ( size_t i = 0; i != altCount; ++i )
			if ( StorageExist( altDirNames[ i ], pParentDir ) )
				return std::pair< const TCHAR*, size_t >( altDirNames[ i ], i );

		return std::pair< const TCHAR*, size_t >( NULL, utl::npos );			// no storage found
	}

	std::pair< const TCHAR*, size_t > CStructuredStorage::FindAlternate_StreamName( const TCHAR* altStreamNames[], size_t altCount, IStorage* pParentDir /*= NULL*/ )
	{
		REQUIRE( altCount != 0 );

		for ( size_t i = 0; i != altCount; ++i )
			if ( StreamExist( altStreamNames[ i ], pParentDir ) )
				return std::pair< const TCHAR*, size_t >( altStreamNames[ i ], i );

		return std::pair< const TCHAR*, size_t >( NULL, utl::npos );			// no stream found
	}


	std::tstring CStructuredStorage::EncodeStreamName( const TCHAR* pStreamName ) const
	{
		return MakeShortFilename( pStreamName );
	}

	bool CStructuredStorage::RetainOpenedStream( const TCHAR* pStreamName, IStorage* pParentDir ) const
	{
		pStreamName, pParentDir;
		return m_useStreamSharing;
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream /*= NULL*/ ) const
	{
		std::auto_ptr< COleStreamFile > pFile( new CAutoOleStreamFile( pStream ) );

		// store the stream path so it gets assigned in CArchive objects;
		// note: accurate with deep streams as well.
		pFile->SetFilePath( fs::CFlexPath::MakeComplexPath( GetDocFilePath().Get(), pStreamName ).GetPtr() );
		return pFile;
	}

	bool CStructuredStorage::HandleError( HRESULT hResult, const TCHAR* pSubPath, const TCHAR* pDocFilePath /*= NULL*/ ) const
	{
		if ( SUCCEEDED( hResult ) || IsIgnoreMode() )
			return true;				// all good

		// error codes 255 or less are DOS/Win32 error codes
		if ( SEVERITY_ERROR == HRESULT_SEVERITY( hResult ) &&
			 FACILITY_STORAGE == HRESULT_FACILITY( hResult ) &&
			 HRESULT_CODE( hResult ) < 0x100 )
		{
			ASSERT( HRESULT_CODE( hResult ) != 0 );
			// throw an exception matching to the DOS error; NOTE: only the DOS error part of the SCODE becomes m_lOsError
			s_fileError.m_cause = CFileException::OsErrorToException( HRESULT_CODE( hResult ) );
			hResult = HRESULT_CODE( hResult );
		}
		else
		{	// attempt some conversion of storage specific error codes to generic CFileException causes...
			switch ( hResult )
			{
				case STG_E_INUSE:
				case STG_E_SHAREREQUIRED:
					s_fileError.m_cause = CFileException::sharingViolation;
					break;
				case STG_E_NOTCURRENT:
				case STG_E_REVERTED:
				case STG_E_CANTSAVE:
				case STG_E_OLDFORMAT:
				case STG_E_OLDDLL:
				default:
					s_fileError.m_cause = CFileException::genericException;
					break;
			}
		}

		if ( str::IsEmpty( pDocFilePath ) )
			pDocFilePath = GetDocFilePath().GetPtr();

		s_fileError.m_lOsError = (LONG)hResult;

		s_fileError.m_strFileName = pSubPath;
		if ( !str::IsEmpty( pDocFilePath ) )
			if ( !s_fileError.m_strFileName.IsEmpty() )
				s_fileError.m_strFileName = CFlexPath::MakeComplexPath( pDocFilePath, s_fileError.m_strFileName.GetString() ).GetPtr();
			else
				s_fileError.m_strFileName = pDocFilePath;

		if ( !IsThrowMode() )
		{
			app::TraceException( &s_fileError );
			return false;
		}

		AfxThrowFileException( s_fileError.m_cause, s_fileError.m_lOsError, s_fileError.m_strFileName );
	}


	IStorage* CStructuredStorage::GetParentDir( const CStorageTrail* pParentTrail ) const
	{
		REQUIRE( NULL == pParentTrail || m_pRootStorage == pParentTrail->GetRoot() );		// trail refers to this storage?

		IStorage* pParentDir = pParentTrail != NULL ? pParentTrail->GetCurrent() : m_pRootStorage;
		ASSERT_PTR( pParentDir );
		return pParentDir;
	}

	bool CStructuredStorage::CacheStreamState( const TCHAR* pStreamName, IStorage* pParentDir, IStream* pStreamOrigin )
	{
		fs::TEmbeddedPath streamPath = MakeElementSubPath( pStreamName, pParentDir );

		if ( IsStreamOpen( streamPath ) )
			return false;			// only cache once

		fs::CStreamState streamState;
		if ( !streamState.Build( pStreamOrigin, RetainOpenedStream( pStreamName, pParentDir ) ) )
			return false;

		m_openedStreamStates.Add( streamPath, streamState );
		return true;
	}

	fs::TEmbeddedPath CStructuredStorage::MakeElementSubPath( const TCHAR* pElementName, IStorage* pParentDir )
	{
		fs::TEmbeddedPath elementSubPath( pElementName );		// logical embedded path

		if ( pParentDir != NULL )
			elementSubPath = fs::stg::GetElementName( pParentDir ) / elementSubPath;		// prepend the parent storage name (works with shallow sub-dirs with short names)

		return elementSubPath;
	}


	CStructuredStorage::TStorageMap& CStructuredStorage::GetOpenedDocStgs( void )
	{
		static TStorageMap s_openedStgs;
		return s_openedStgs;
	}

	CStructuredStorage* CStructuredStorage::FindOpenedStorage( const fs::CPath& docStgPath )
	{
		const TStorageMap& rOpenedStgs = GetOpenedDocStgs();
		return rOpenedStgs.Find( docStgPath );
	}

	void CStructuredStorage::RegisterDocStg( CStructuredStorage* pDocStg )
	{
		ASSERT_PTR( pDocStg );

		const fs::CPath& docStgPath = pDocStg->GetDocFilePath();
		ENSURE( !docStgPath.IsEmpty() );

		TStorageMap& rOpenedStgs = GetOpenedDocStgs();
		rOpenedStgs.Add( docStgPath, pDocStg );
	}

	void CStructuredStorage::UnregisterDocStg( CStructuredStorage* pDocStg )
	{
		ASSERT_PTR( pDocStg );

		const fs::CPath& docStgPath = pDocStg->GetDocFilePath();
		ASSERT( !docStgPath.IsEmpty() );

		TStorageMap& rOpenedStgs = GetOpenedDocStgs();
		VERIFY( rOpenedStgs.Remove( docStgPath ) );
	}

} //namespace fs


namespace fs
{
	bool SeekToPos( IStream* pStream, UINT64 position, DWORD origin /*= STREAM_SEEK_SET*/ )
	{
		ASSERT_PTR( pStream );
		LARGE_INTEGER startPos;
		startPos.QuadPart = position;
		return HR_OK( pStream->Seek( startPos, origin, NULL ) );
	}


	// CStreamState implementation

	bool CStreamState::Build( IStream* pStreamOrigin, bool keepStreamAlive )
	{
		REQUIRE( 1 == dbg::GetRefCount( pStreamOrigin ) );		// should be the first opening (original stream)

		if ( !fs::stg::GetElementFullState( *this, pStreamOrigin ) )
			return false;

		if ( keepStreamAlive )
			m_pStreamOrigin = pStreamOrigin;

		return true;
	}

	HRESULT CStreamState::CloneStream( IStream** ppStreamDuplicate ) const
	{
		ASSERT_NULL( *ppStreamDuplicate );
		ASSERT_PTR( m_pStreamOrigin );

		HRESULT hResult = m_pStreamOrigin->Clone( ppStreamDuplicate );
		if ( SUCCEEDED( hResult ) )
		{
			ENSURE( 1 == dbg::GetRefCount( *ppStreamDuplicate ) );

			fs::SeekToBegin( *ppStreamDuplicate );			// rewind the stream for reading
			TRACE_COM_PTR( m_pStreamOrigin, str::AsNarrow( str::Format( _T("stream origin after cloning %s"), m_fullPath.GetPtr() ) ).c_str() );
		}
		return hResult;
	}

} //namespace fs


namespace fs
{
	// CAutoOleStreamFile implementation

	CAutoOleStreamFile::CAutoOleStreamFile( IStream* pStreamOpened /*= NULL*/ )
		: COleStreamFile( pStreamOpened )
	{
		if ( pStreamOpened != NULL )
			pStreamOpened->AddRef();		// keep the stream alive when passed on contructor by e.g. OpenStream - it will be released on Close()

		m_bCloseOnDelete = TRUE;			// will close on destructor
	}

	CAutoOleStreamFile::~CAutoOleStreamFile()
	{
		//TRACE_COM_ITF( m_lpStream, "CAutoOleStreamFile::~CAutoOleStreamFile() - before closing m_lpStream" );		// refCount=1 for straight usage
	}


	namespace flex
	{
		CComPtr< IStream > OpenStreamOnFile( const fs::CFlexPath& filePath, DWORD mode /*= STGM_READ*/, DWORD createAttributes /*= 0*/ )
		{
			CComPtr< IStream > pStream;

			if ( !filePath.IsComplexPath() )
				::SHCreateStreamOnFileEx( filePath.GetPtr(), mode, createAttributes, FALSE, NULL, &pStream );
			else
			{
				fs::stg::CAcquireStorage scopedStg( filePath.GetPhysicalPath(), mode );
				if ( scopedStg.Get() != NULL )
					pStream = scopedStg.Get()->OpenStream( filePath.GetEmbeddedPath(), NULL, mode );
			}

			if ( NULL == pStream )
				TRACE( _T(" * fs::flex::OpenStreamOnFile() failed to open file %s\n"), filePath.GetPtr() );

			return pStream;
		}


		UINT64 GetFileSize( const fs::CFlexPath& filePath )
		{
			if ( !filePath.IsComplexPath() )
				return fs::GetFileSize( filePath.GetPtr() );
			else
			{
				fs::stg::CAcquireStorage scopedStg( filePath.GetPhysicalPath() );
				if ( CStructuredStorage* pDocStorage = scopedStg.Get() )
				{
					fs::TEmbeddedPath embeddedPath( filePath.GetEmbeddedPath() );

					if ( const fs::CStreamState* pStreamState = pDocStorage->FindOpenedStream( embeddedPath ) )
						return pStreamState->m_fileSize;		// cached file size

					// try this first: the storage may have a flat representation for a deep embedded path (root streams with encoded deep names)
					CComPtr< IStream > pStream;

					if ( pDocStorage->StreamExist( embeddedPath.GetPtr() ) )
						pStream = pDocStorage->OpenStream( embeddedPath.GetPtr() );
					else
						pStream = fs::CStorageTrail::OpenEmbeddedStream( pDocStorage, embeddedPath );		// second: try to open the deep stream

					if ( pStream != NULL )
						return fs::stg::GetStreamSize( &*pStream );
				}
			}

			TRACE( _T(" * fs::flex::GetFileSize() failed to open file %s\n"), filePath.GetPtr() );
			return 0;
		}

		CTime ReadLastModifyTime( const fs::CFlexPath& filePath )
		{
			return fs::ReadLastModifyTime( filePath.GetPhysicalPath() );		// if embedded path, refer to stg file
		}

		FileExpireStatus CheckExpireStatus( const fs::CFlexPath& filePath, const CTime& lastModifyTime )
		{
			return fs::CheckExpireStatus( filePath, lastModifyTime );
		}
	}

} //namespace fs


namespace fs
{
	namespace stg
	{
		bool MakeFileState( fs::CFileState& rState, const STATSTG& stat )
		{
			if ( !CTime::IsValidFILETIME( stat.mtime ) ||
				 !CTime::IsValidFILETIME( stat.ctime ) ||
				 !CTime::IsValidFILETIME( stat.atime ) )
				return false;

			// odly enough, on IStream-s all of stat data-members mtime, ctime, atime are set to null - CTime(0)
			rState.m_modifTime = CTime( stat.mtime );
			rState.m_creationTime = CTime( stat.ctime );
			rState.m_accessTime = CTime( stat.atime );

			rState.m_fileSize = static_cast< UINT64 >( stat.cbSize.QuadPart );

			rState.m_attributes = 0;
			if ( stat.pwcsName != NULL )						// name was requested?
			{
				rState.m_fullPath.Set( stat.pwcsName );		// assign element name - the physical name of the stream (max 31 characters)
				::CoTaskMemFree( stat.pwcsName );			// free the memory allocated by Stat() 
			}
			else
				rState.m_fullPath.Clear();

			return true;
		}
	}
}
