
#include "stdafx.h"
#include "StructuredStorage.h"
#include "FlexPath.h"
#include "StringUtilities.h"
#include "utl/AppTools.h"
#include "utl/TimeUtils.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CStructuredStorage implementation

	mfc::CAutoException< CFileException > CStructuredStorage::s_fileError;
	const TCHAR CStructuredStorage::s_rootFolderName[] = _T("");

	CStructuredStorage::CStructuredStorage( void )
		: CErrorHandler( utl::CheckMode )
		, m_cwdTrail( this )
		, m_stgFlags( 0 )			// by default there is no need for shared storage reading access
		, m_openMode( 0 )
	{
	}

	CStructuredStorage::~CStructuredStorage()
	{
		if ( IsOpen() )
			CloseDocFile();
	}

	void CStructuredStorage::SetUseStreamSharing( bool useStreamSharing /*= true*/ )
	{
		::SetFlag( m_stgFlags, ReadingStreamSharing, useStreamSharing );
		m_openedStreamStates.Clear();
	}

	std::tstring CStructuredStorage::MakeShortFilename( const TCHAR* pElementName )
	{
		// pFilename could refer to a dir, file, or a sub-path;
		// encode with a hashed suffix if pFilename longer than the stream limit (compound document limitation).

		REQUIRE( pElementName == path::FindFilename( pElementName ) );			// no slashes, should be encoded
		return path::MakeShortHashedFilename( pElementName, MaxFilenameLen );
	}

	DWORD CStructuredStorage::ToMode( DWORD mode )
	{
		enum { AccessMask = 0x0000000F, ShareMask = 0x000000F0 };		// covers for STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE

		if ( HasFlag( mode, ShareMask ) )
			return mode;

		return mode | STGM_SHARE_EXCLUSIVE;		// MSDN: compound-file storages/streams must be opened at least with STGM_SHARE_EXCLUSIVE
	}


	void CStructuredStorage::CloseDocFile( void )
	{
		if ( !IsOpen() )
			return;

		UnregisterDocStg( this );

		m_cwdTrail.Reset();
		m_openedStreamStates.Clear();

		m_pRootStorage = NULL;			// release the storage
		m_openMode = 0;
		m_docFilePath.Clear();
	}

	bool CStructuredStorage::CreateDocFile( const fs::CPath& docFilePath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		if ( IsOpen() )
			CloseDocFile();

		mode = ToMode( mode );

		HRESULT hResult = ::StgCreateStorageEx( docFilePath.GetPtr(), mode, STGFMT_STORAGE, 0, NULL, 0, IID_PPV_ARGS( &m_pRootStorage ) );
		//HRESULT hResult = ::StgCreateDocfile( docFilePath.GetPtr(), mode, 0, &m_pRootStorage );		// old API NT4-

		if ( FAILED( hResult ) )
			return HandleError( hResult, NULL, docFilePath.GetPtr() );

		m_openMode = mode;
		m_docFilePath = fs::stg::GetElementName( &*m_pRootStorage );
		RegisterDocStg( this );
		return true;
	}

	bool CStructuredStorage::OpenDocFile( const fs::CPath& docFilePath, DWORD mode /*= STGM_READ*/ )
	{
		if ( IsOpen() )
			CloseDocFile();

		mode = ToMode( mode );

		HRESULT hResult = HR_AUDIT( ::StgOpenStorageEx( docFilePath.GetPtr(), mode, STGFMT_STORAGE, 0, NULL, 0, IID_PPV_ARGS( &m_pRootStorage ) ) );
		//HRESULT hResult = HR_AUDIT( ::StgOpenStorage( docFilePath.GetPtr(), NULL, mode, NULL, 0, &m_pRootStorage ) );		// old API NT4-

		if ( FAILED( hResult ) )
			return HandleError( hResult, NULL, docFilePath.GetPtr() );

		m_openMode = mode;
		m_docFilePath = fs::stg::GetElementName( &*m_pRootStorage );
		RegisterDocStg( this );
		return true;
	}


	bool CStructuredStorage::ChangeCurrentDir( const TCHAR* pDirSubPath, DWORD mode /*= STGM_READ*/ )
	{
		if ( str::IsEmpty( pDirSubPath ) )
		{
			m_cwdTrail.Reset();
			return true;			// changed to root storage dir
		}
		REQUIRE( !path::IsSlash( pDirSubPath[ 0 ] ) );			// embedded paths never start with a slash

		CStorageTrail origTrail = m_cwdTrail;
		CScopedErrorHandling scopedIgnore( this, utl::CheckMode );

		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end(); ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );

			if ( CComPtr< IStorage > pStorage = OpenDir( itSubDir->c_str(), mode ) )		// open existing storage
				m_cwdTrail.Push( pStorage );					// advance one level deeper
			else
			{
				m_cwdTrail.Reset( &origTrail );		// restore original CWD
				return false;						// error: could not open sub-storage
			}
		}

		return true;			// all sub-storages successfully opened
	}

	bool CStructuredStorage::MakeDirPath( const TCHAR* pDirSubPath, bool enterCurrent, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		CStorageTrail origTrail = m_cwdTrail;
		CScopedErrorHandling scopedIgnore( this, utl::CheckMode );

		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		DWORD openExistingMode = ::MakeFlag( mode, STGM_CREATE, false );
		bool succeeded = true;

		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end(); ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );

			CComPtr< IStorage > pStorage = StorageExist( itSubDir->c_str() )
				? OpenDir( itSubDir->c_str(), openExistingMode )	// open existing storage
				: CreateDir( itSubDir->c_str(), mode );				// create the sub-storage

			if ( pStorage != NULL )
				m_cwdTrail.Push( pStorage );		// advance one level deeper
			else
			{
				succeeded = false;					// error: could not create sub-storage
				break;
			}
		}

		if ( !succeeded || !enterCurrent )
			m_cwdTrail.Reset( &origTrail );		// restore original CWD

		return succeeded;			// all sub-storages successfully acquired?
	}


	CComPtr< IStorage > CStructuredStorage::CreateDir( const TCHAR* pDirName, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = GetCurrentDir()->CreateStorage( storageName.c_str(), ToMode( mode ), 0, 0, &pDirStorage );

		if ( FAILED( hResult ) )
			HandleError( hResult, pDirName );
		return pDirStorage;
	}

	CComPtr< IStorage > CStructuredStorage::OpenDir( const TCHAR* pDirName, DWORD mode /*= STGM_READ*/ )
	{
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = GetCurrentDir()->OpenStorage( storageName.c_str(), NULL, ToMode( mode ), NULL, 0, &pDirStorage );

		if ( FAILED( hResult ) )
			HandleError( hResult, pDirName );

		return pDirStorage;
	}

	bool CStructuredStorage::DeleteDir( const TCHAR* pDirName )
	{
		std::tstring storageName = MakeShortFilename( pDirName );
		fs::TEmbeddedPath dirSubPath = MakeElementSubPath( storageName );

		CloseStreamsWithPrefix( dirSubPath.GetPtr() );		// deep delete cached stream states

		HRESULT hResult = GetCurrentDir()->DestroyElement( storageName.c_str() );
		if ( FAILED( hResult ) )
			return HandleError( hResult, pDirName );

		return true;
	}

	bool CStructuredStorage::StorageExist( const TCHAR* pStorageName )
	{
		CScopedErrorHandling scopedIgnore( this, utl::IgnoreMode );		// failure is not an error

		CComPtr< IStorage > pStorage = OpenDir( pStorageName, STGM_READ );
		return pStorage != NULL;
	}


	CComPtr< IStream > CStructuredStorage::CreateStream( const TCHAR* pStreamName, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( IsOpen() );

		std::tstring streamName = EncodeStreamName( pStreamName );
		fs::TEmbeddedPath streamPath = MakeElementSubPath( streamName );

		m_openedStreamStates.Remove( streamPath );		// remove any cached reading stream state

		CComPtr< IStream > pFileStream;
		HRESULT hResult = GetCurrentDir()->CreateStream( streamName.c_str(), ToMode( mode ), NULL, 0, &pFileStream );

		if ( FAILED( hResult ) )
			HandleError( hResult, pStreamName );
	
		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::OpenStream( const TCHAR* pStreamName, DWORD mode /*= STGM_READ*/ )
	{
		std::tstring streamName = EncodeStreamName( pStreamName );
		fs::TEmbeddedPath streamPath = MakeElementSubPath( streamName );

		if ( IsReadingMode( mode ) )
		{
			if ( const fs::CStreamState* pFoundStreamState = FindOpenedStream( streamPath ) )		// stream already open for reading?
				if ( pFoundStreamState->HasStream() )							// opened for sharing?
					return CloneStream( pFoundStreamState, streamPath );		// return a clone of the original opened stream
		}
		else
			m_openedStreamStates.Remove( streamPath );

		CComPtr< IStream > pFileStream;
		HRESULT hResult = GetCurrentDir()->OpenStream( streamName.c_str(), NULL, ToMode( mode ), 0, &pFileStream );

		if ( FAILED( hResult ) )
			HandleError( hResult, pStreamName );
		else if ( IsReadingMode( mode ) )
			CacheStreamState( streamPath, pFileStream );		// cache the stream metadata (when opened for reading)

		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::CloneStream( const fs::CStreamState* pStreamState, const fs::TEmbeddedPath& streamPath )
	{
		ASSERT_PTR( pStreamState );

		CComPtr< IStream > pStreamReadingDuplicate;
		HRESULT hResult = pStreamState->CloneStream( &pStreamReadingDuplicate );		// clone and rewind the stream origin

		if ( FAILED( hResult ) )
			HandleError( hResult, streamPath.GetNameExt() );

		return pStreamReadingDuplicate;
	}

	bool CStructuredStorage::DeleteStream( const TCHAR* pStreamName )
	{
		std::tstring streamName = EncodeStreamName( pStreamName );

		m_openedStreamStates.Remove( MakeElementSubPath( streamName ) );

		HRESULT hResult = GetCurrentDir()->DestroyElement( streamName.c_str() );
		if ( FAILED( hResult ) )
			return HandleError( hResult, pStreamName );

		return true;
	}

	bool CStructuredStorage::CloseStream( const TCHAR* pStreamName )
	{
		fs::TEmbeddedPath streamPath = MakeElementSubPath( EncodeStreamName( pStreamName ) );

		if ( fs::CStreamState* pCachedStreamState = m_openedStreamStates.Find( streamPath ) )
			if ( pCachedStreamState->HasStream() )
			{
				pCachedStreamState->ReleaseStream();
				return true;
			}

		return false;
	}

	bool CStructuredStorage::StreamExist( const TCHAR* pStreamName )
	{
		CScopedErrorHandling scopedIgnore( this, utl::IgnoreMode );		// failure is not an error

		CComPtr< IStream > pStream = OpenStream( pStreamName, STGM_READ );
		return pStream != NULL;
	}


	std::auto_ptr< fs::CStreamLocation > CStructuredStorage::LocateReadStream( const fs::TEmbeddedPath& streamEmbeddedPath, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT( IsOpenForReading() && IsReadingMode( mode ) );

		std::auto_ptr< fs::CStreamLocation > pStreamLocation( new CStreamLocation() );

		if ( UseFlatStreamNames() )		// this storage uses flat stream representation?
		{	// first: try to locate it in root storage
			pStreamLocation->m_pCurrDir.reset( new CScopedCurrentDir( this ) );

			if ( StreamExist( streamEmbeddedPath.GetPtr() ) )
			{
				pStreamLocation->m_pStream = OpenStream( streamEmbeddedPath.GetPtr(), mode );		// open the encoded root stream
				if ( pStreamLocation->IsValid() )
					return pStreamLocation;
			}
		}

		// second: try to open the deep stream
		pStreamLocation->m_pCurrDir.reset( new CScopedCurrentDir( this, streamEmbeddedPath.GetParentPath().GetPtr() ) );
		pStreamLocation->m_pStream = OpenStream( streamEmbeddedPath.GetNameExt(), mode );			// open the deep stream
		if ( !pStreamLocation->IsValid() )
			pStreamLocation.reset();

		return pStreamLocation;
	}

	std::auto_ptr< fs::CStreamLocation > CStructuredStorage::LocateWriteStream( const fs::TEmbeddedPath& streamEmbeddedPath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		ASSERT( IsOpenForWriting() && IsWritingMode( mode ) );

		std::auto_ptr< fs::CStreamLocation > pStreamLocation( new CStreamLocation() );

		if ( UseFlatStreamNames() )
			pStreamLocation->m_pCurrDir.reset( new CScopedCurrentDir( this, s_rootFolderName, STGM_READWRITE ) );			// encoded stream located in root storage location
		else
		{
			fs::TEmbeddedPath folderPath = streamEmbeddedPath.GetParentPath();

			if ( MakeDirPath( folderPath.GetPtr(), false ) )		// ensure the parent storage path is created
				pStreamLocation->m_pCurrDir.reset( new CScopedCurrentDir( this, folderPath.GetPtr(), STGM_READWRITE ) );	// deep stream located in its parent storage path location
		}

		if ( pStreamLocation->m_pCurrDir.get() != NULL )		// do we have a valid location?
			if ( HasFlag( mode, STGM_CREATE ) )
				pStreamLocation->m_pStream = CreateStream( streamEmbeddedPath.GetPtr(), mode );
			else
				pStreamLocation->m_pStream = OpenStream( streamEmbeddedPath.GetPtr(), mode );

		if ( !pStreamLocation->IsValid() )
			pStreamLocation.reset();

		return pStreamLocation;
	}


	std::auto_ptr< COleStreamFile > CStructuredStorage::CreateStreamFile( const TCHAR* pStreamName, DWORD mode /*= CFile::modeCreate | CFile::modeWrite*/ )
	{
		std::auto_ptr< COleStreamFile > pNewFile;

		if ( CComPtr< IStream > pNewStream = CreateStream( pStreamName, mode ) )		// use CreateStream() method for seamless error handling
			pNewFile = MakeOleStreamFile( pStreamName, pNewStream );

		//TRACE_COM_ITF( fS::GetSafeStream( pNewFile.get() ), "CStructuredStorage::CreateStreamFile() - exiting" );
		return pNewFile;
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::OpenStreamFile( const TCHAR* pStreamName, DWORD mode /*= CFile::modeRead*/ )
	{
		std::auto_ptr< COleStreamFile > pOpenedFile;

		if ( CComPtr< IStream > pStream = OpenStream( pStreamName, mode ) )				// use OpenStream() method for seamless error handling, stream sharing
			pOpenedFile = MakeOleStreamFile( pStreamName, pStream );

		//TRACE_COM_ITF( fS::GetSafeStream( pOpenedFile.get() ), "CStructuredStorage::OpenStreamFile() - exiting" );
		return pOpenedFile;
	}


	std::pair< const TCHAR*, size_t > CStructuredStorage::FindAlternate_DirName( const TCHAR* altDirNames[], size_t altCount )
	{
		REQUIRE( altCount != 0 );

		for ( size_t i = 0; i != altCount; ++i )
			if ( StorageExist( altDirNames[ i ] ) )
				return std::pair< const TCHAR*, size_t >( altDirNames[ i ], i );

		return std::pair< const TCHAR*, size_t >( NULL, utl::npos );			// no storage found
	}

	std::pair< const TCHAR*, size_t > CStructuredStorage::FindAlternate_StreamName( const TCHAR* altStreamNames[], size_t altCount )
	{
		REQUIRE( altCount != 0 );

		for ( size_t i = 0; i != altCount; ++i )
			if ( StreamExist( altStreamNames[ i ] ) )
				return std::pair< const TCHAR*, size_t >( altStreamNames[ i ], i );

		return std::pair< const TCHAR*, size_t >( NULL, utl::npos );			// no stream found
	}


	std::tstring CStructuredStorage::EncodeStreamName( const TCHAR* pStreamName ) const
	{
		std::tstring streamName = pStreamName;

		// note: always flatten deep stream names
		std::replace( streamName.begin(), streamName.end(), _T('\\'), GetFlattenPathSep() );			// '\' -> '|'
		return MakeShortFilename( streamName.c_str() );
	}

	TCHAR CStructuredStorage::GetFlattenPathSep( void ) const
	{
		return _T('|');
	}

	bool CStructuredStorage::RetainOpenedStream( const fs::TEmbeddedPath& streamPath ) const
	{
		streamPath;
		return UseStreamSharing();
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream /*= NULL*/ ) const
	{
		std::auto_ptr< COleStreamFile > pFile( new CManagedOleStreamFile( pStream, fs::CFlexPath::MakeComplexPath( GetDocFilePath(), GetCurrentDirPath() / pStreamName ) ) );
		return pFile;
	}

	bool CStructuredStorage::HandleError( HRESULT hResult, const TCHAR* pElementName, const TCHAR* pDocFilePath /*= NULL*/ ) const
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

		s_fileError.m_lOsError = (LONG)hResult;
		s_fileError.m_strFileName = MakeElementFlexPath( pDocFilePath, pElementName ).GetPtr();

		if ( !IsThrowMode() )
		{
			app::TraceException( &s_fileError );
			return false;
		}

		AfxThrowFileException( s_fileError.m_cause, s_fileError.m_lOsError, s_fileError.m_strFileName );
	}

	fs::CFlexPath CStructuredStorage::MakeElementFlexPath( const TCHAR* pDocFilePath, const TCHAR* pElementName ) const
	{
		fs::TEmbeddedPath elementPath = GetCurrentDirPath();
		if ( !str::IsEmpty( pElementName ) )
			elementPath /= pElementName;

		fs::CFlexPath elementFullPath = !str::IsEmpty( pDocFilePath ) ? fs::CFlexPath( pDocFilePath ) : GetDocFilePath().Get();
		if ( !elementPath.IsEmpty() )
			elementFullPath = CFlexPath::MakeComplexPath( elementFullPath, elementPath );

		return elementFullPath;
	}


	bool CStructuredStorage::CacheStreamState( const fs::TEmbeddedPath& streamPath, IStream* pStreamOrigin )
	{
		if ( IsStreamOpen( streamPath ) )
			return false;			// only cache once

		fs::CStreamState streamState;
		bool keepStreamAlive = RetainOpenedStream( streamPath );

		if ( !streamState.Build( pStreamOrigin, keepStreamAlive ) )
			return false;

		m_openedStreamStates.Add( streamPath, streamState );
		return true;
	}

	bool CStructuredStorage::EnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth /*= Shallow*/ )
	{
		ASSERT_PTR( pEnumerator );

		IStorage* pCurrStorage = GetCurrentDir();
		fs::TEmbeddedPath embeddedPath = GetCurrentDirPath();

		CComPtr< IEnumSTATSTG > pEnumStat;

		HRESULT hResult = pCurrStorage->EnumElements( 0, NULL, 0, &pEnumStat );
		if ( FAILED( hResult ) )
			return HandleError( hResult, embeddedPath.GetPtr() );

		STATSTG stat;
		std::vector< fs::CPath > subStgNames;

		for ( ;; )
		{
			hResult = pEnumStat->Next( 1, &stat, NULL );
			if ( FAILED( hResult ) )
				return HandleError( hResult, embeddedPath.GetPtr() );
			else if ( S_FALSE == hResult )		// done?
				break;

			switch ( stat.type )
			{
				case STGTY_STORAGE:
					subStgNames.push_back( fs::CPath( stat.pwcsName ) );
					break;
				case STGTY_STREAM:
				{
					fs::TEmbeddedPath streamPath = embeddedPath / stat.pwcsName;
					pEnumerator->AddFoundFile( streamPath.GetPtr() );
					break;
				}
			}
			::CoTaskMemFree( stat.pwcsName );
		}

		fs::SortPaths( subStgNames );		// natural path order

		bool succeeded = true;

		for ( std::vector< fs::CPath >::const_iterator itSubStgName = subStgNames.begin(); itSubStgName != subStgNames.end(); ++itSubStgName )
		{
			fs::TEmbeddedPath subDirPath = embeddedPath / *itSubStgName;
			if ( pEnumerator->AddFoundSubDir( subDirPath.GetPtr() ) )		// sub-storage is not ignored?
				if ( Deep == depth && !pEnumerator->MustStop() )
				{
					m_cwdTrail.Push( OpenDir( itSubStgName->GetPtr() ) );

					if ( !EnumElements( pEnumerator, Deep ) )
						succeeded = false;

					m_cwdTrail.Pop();
				}
		}
		return succeeded;
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
		TStorageMap& rOpenedStgs = GetOpenedDocStgs();

		REQUIRE( !docStgPath.IsEmpty() );
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


	// CStructuredStorage::CStorageTrail implementation

	void CStructuredStorage::CStorageTrail::Reset( const CStorageTrail* pTrail /*= NULL*/ )
	{
		if ( pTrail != NULL )
		{
			ASSERT( m_pDocStorage == pTrail->m_pDocStorage );

			m_openSubStorages = pTrail->m_openSubStorages;
			m_trailPath = pTrail->m_trailPath;
		}
		else
		{
			while ( !m_openSubStorages.empty() )
				m_openSubStorages.pop_back();

			m_trailPath.Clear();
		}
	}

	void CStructuredStorage::CStorageTrail::Push( IStorage* pSubStorage )
	{
		ASSERT_PTR( pSubStorage );

		m_openSubStorages.push_back( pSubStorage );
		m_trailPath /= fs::stg::GetElementName( pSubStorage );		// go deeper one level
	}

	CComPtr< IStorage > CStructuredStorage::CStorageTrail::Pop( void )
	{
		ASSERT( !m_openSubStorages.empty() );

		CComPtr< IStorage > pDeepStorage = m_openSubStorages.back();

		m_openSubStorages.pop_back();
		m_trailPath = m_trailPath.GetParentPath();					// rewind one level up
		return pDeepStorage;
	}


	// CStructuredStorage::CScopedCurrentDir implementation

	CStructuredStorage::CScopedCurrentDir::CScopedCurrentDir( CStructuredStorage* pDocStorage, const TCHAR* pDirSubPath /*= s_rootFolderName*/, DWORD mode /*= STGM_READ*/, bool fromRoot /*= true*/ )
		: m_pDocStorage( pDocStorage )
		, m_origCwdTrail( *m_pDocStorage->RefCwd() )
	{
		if ( fromRoot )
			m_pDocStorage->ResetToRootCurrentDir();

		m_validDirPath = m_pDocStorage->ChangeCurrentDir( pDirSubPath, mode );
	}

	CStructuredStorage::CScopedCurrentDir::CScopedCurrentDir( CStructuredStorage* pDocStorage, IStorage* pSubStorage, bool fromRoot /*= true*/ )
		: m_pDocStorage( pDocStorage )
		, m_origCwdTrail( *m_pDocStorage->RefCwd() )
		, m_validDirPath( true )
	{
		ASSERT_PTR( pSubStorage );

		if ( fromRoot )
			m_pDocStorage->ResetToRootCurrentDir();

		if ( pSubStorage != NULL )
			m_pDocStorage->RefCwd()->Push( pSubStorage );
	}

	CStructuredStorage::CScopedCurrentDir::~CScopedCurrentDir()
	{
		if ( m_validDirPath )
			m_pDocStorage->RefCwd()->Reset( &m_origCwdTrail );
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
			//TRACE_COM_PTR( m_pStreamOrigin, str::AsNarrow( str::Format( _T("stream origin after cloning %s"), m_fullPath.GetPtr() ) ).c_str() );
		}
		return hResult;
	}

} //namespace fs


namespace fs
{
	// CManagedOleStreamFile implementation

	CManagedOleStreamFile::CManagedOleStreamFile( IStream* pStreamOpened, const fs::CFlexPath& streamFullPath )
		: COleStreamFile( pStreamOpened )
	{
		Init( pStreamOpened, streamFullPath );
	}

	CManagedOleStreamFile::CManagedOleStreamFile( std::auto_ptr< fs::CStreamLocation > pStreamLocation, const fs::CFlexPath& streamFullPath )
		: COleStreamFile( pStreamLocation->m_pStream )
		, m_pStreamLocation( pStreamLocation )
	{
		REQUIRE( m_pStreamLocation.get() != NULL && m_pStreamLocation->IsValid() );

		Init( m_pStreamLocation->m_pStream, streamFullPath );
		m_pStreamLocation->m_pStream = NULL;					// release the stream since is managed by COleStreamFile base class, but keep location alive
	}

	CManagedOleStreamFile::~CManagedOleStreamFile()
	{
		ASSERT( m_bCloseOnDelete || NULL == GetStream() );
		//TRACE_COM_ITF( m_lpStream, "CManagedOleStreamFile::~CManagedOleStreamFile() - before closing m_lpStream" );		// refCount=1 for straight usage
	}

	void CManagedOleStreamFile::Init( IStream* pStreamOpened, const fs::CFlexPath& streamFullPath )
	{
		m_bCloseOnDelete = TRUE;			// will close on destructor

		if ( pStreamOpened != NULL )
			pStreamOpened->AddRef();		// keep the stream alive when passed on contructor by e.g. OpenStream - it will be released on Close()

		// store the stream path so it gets assigned in CArchive objects;
		SetFilePath( streamFullPath.GetPtr() );
	}


	namespace stg
	{
		// CScopedCreateDocMode implementation

		CScopedCreateDocMode::CScopedCreateDocMode( CStructuredStorage* pDocStorage, const fs::CPath* pDocFilePath, const CErrorHandler* pSrcHandler /*= CErrorHandler::Thrower()*/ )
			: CScopedErrorHandling( pDocStorage, pSrcHandler )
			, m_pDocStorage( pDocStorage )
			, m_docFilePath( pDocFilePath != NULL ? *pDocFilePath : m_pDocStorage->GetDocFilePath() )
		{
			if ( pDocFilePath != NULL )
				VERIFY( m_pDocStorage->CreateDocFile( m_docFilePath ) );
		}

		CScopedCreateDocMode::~CScopedCreateDocMode()
		{
			if ( m_pDocStorage != NULL )			// not restored by derived class?
				m_pDocStorage->CloseDocFile();
		}


		// CScopedWriteDocMode implementation

		CScopedWriteDocMode::CScopedWriteDocMode( CStructuredStorage* pDocStorage, const fs::CPath* pDocFilePath, DWORD writeMode /*= STGM_READWRITE*/,
												  const CErrorHandler* pSrcHandler /*= CErrorHandler::Thrower()*/ )
			: CScopedCreateDocMode( pDocStorage, NULL /*avoid creation*/, pSrcHandler )
			, m_origReadingMode( s_closedMode )
		{
			__super::m_docFilePath = pDocFilePath != NULL ? *pDocFilePath : m_pDocStorage->GetDocFilePath();

			if ( m_pDocStorage->IsOpen() )
			{
				m_origReadingMode = m_pDocStorage->GetOpenMode();

				ASSERT( CStructuredStorage::IsReadingMode( m_origReadingMode ) );	// ensure that we don't override an existing WRITING mode, and at destruction-time we will restore the READING mode
				ASSERT( m_docFilePath == m_pDocStorage->GetDocFilePath() );			// ensure the same document file
			}

			VERIFY( m_pDocStorage->OpenDocFile( m_docFilePath, writeMode ) );
		}

		CScopedWriteDocMode::~CScopedWriteDocMode()
		{
			if ( m_pDocStorage != NULL )			// not restored by derived class?
			{
				m_pDocStorage->CloseDocFile();

				if ( m_origReadingMode != s_closedMode )	// was originally open for reading?
				{
					CScopedErrorHandling scopedCheck( m_pDocStorage, CErrorHandler::Checker() );		// no throwing in destructor

					// restore previous reading mode
					m_pDocStorage->OpenDocFile( m_docFilePath, m_origReadingMode );
				}

				m_pDocStorage = NULL;				// prevent base destructor from restoring
			}
		}


		// CMirrorStorageSave implementation

		CMirrorStorageSave::CMirrorStorageSave( const fs::CPath& docStgPath, const fs::CPath& oldDocStgPath )
			: m_docStgPath( docStgPath )
		{
			REQUIRE( fs::IsValidStructuredStorage( m_docStgPath.GetPtr() ) );

			if ( m_docStgPath == oldDocStgPath )
			{
				std::tstring mirrorFname = m_docStgPath.GetFname() + _T("_saving");

				m_mirrorDocStgPath = m_docStgPath;
				m_mirrorDocStgPath.ReplaceFname( mirrorFname.c_str() );
			}
		}

		void CMirrorStorageSave::Commit( void ) throws_( CFileException* )
		{
			if ( UseMirroring() )
			{
				CloseStorage();

				// backup original file properties
				CTime createdTime = fs::ReadFileTime( m_docStgPath, fs::CreatedDate );
				fs::CSecurityDescriptor securityDescriptor( m_docStgPath );

				// rename mirror file as the original storage document
				CFile::Remove( m_docStgPath.GetPtr() );
				CFile::Rename( m_mirrorDocStgPath.GetPtr(), m_docStgPath.GetPtr() );

				// restore file properties to the new document
				if ( time_utl::IsValid( createdTime ) )
					fs::thr::TouchFile( m_docStgPath, createdTime, fs::CreatedDate, fs::MfcExc );

				if ( securityDescriptor.IsValid() )
					securityDescriptor.WriteToFile( m_docStgPath );

				m_mirrorDocStgPath.Clear();			// prevent rollback on destructor
			}
		}

		void CMirrorStorageSave::Rollback( void )
		{
			if ( UseMirroring() )
				if ( !::DeleteFile( m_mirrorDocStgPath.GetPtr() ) )
					TRACE( _T(" * CMirrorStorageSave::Rollback(): error deleting mirror storage file '%s'\n"), m_mirrorDocStgPath.GetPtr() );
		}

		bool CMirrorStorageSave::CloseStorage( void )
		{
			if ( fs::CStructuredStorage* pFoundStorage = fs::CStructuredStorage::FindOpenedStorage( m_docStgPath ) )
			{
				pFoundStorage->CloseDocFile();
				return true;
			}
			return false;
		}
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
				if ( CStructuredStorage* pFoundDocStorage = fs::CStructuredStorage::FindOpenedStorage( filePath.GetPhysicalPath() ) )
					pStream = pFoundDocStorage->OpenStream( filePath.GetEmbeddedPathPtr(), mode );
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
				if ( CStructuredStorage* pDocStorage = fs::CStructuredStorage::FindOpenedStorage( filePath.GetPhysicalPath() ) )
				{
					fs::TEmbeddedPath streamEmbeddedPath( filePath.GetEmbeddedPath() );

					if ( const fs::CStreamState* pStreamState = pDocStorage->FindOpenedStream( streamEmbeddedPath ) )
						return pStreamState->m_fileSize;		// cached file size

					std::auto_ptr< fs::CStreamLocation > pStreamLocation = pDocStorage->LocateReadStream( streamEmbeddedPath );
					if ( pStreamLocation.get() != NULL )
						return fs::stg::GetStreamSize( &*pStreamLocation->m_pStream );
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
