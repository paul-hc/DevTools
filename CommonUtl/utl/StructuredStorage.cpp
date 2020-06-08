
#include "stdafx.h"
#include "StructuredStorage.h"
#include "StorageTrack.h"
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

	CStructuredStorage::CStructuredStorage( IStorage* pRootStorage /*= NULL*/ )
		: CThrowMode( false )
		, m_pRootStorage( pRootStorage )
		, m_isEmbeddedStg( m_pRootStorage != NULL )
		, m_openMode( 0 )
	{
	}

	CStructuredStorage::~CStructuredStorage()
	{
		Close();
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

	bool CStructuredStorage::CreateDocFile( const TCHAR* pDocFilePath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( !IsEmbeddedStorage() );
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
		REQUIRE( !IsEmbeddedStorage() );
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

	void CStructuredStorage::Close( void )
	{
		if ( !IsOpen() )
			return;

		if ( !IsEmbeddedStorage() )
			UnregisterDocStg( this );

		m_pRootStorage = NULL;			// release the storage
		m_openMode = 0;
		m_docFilePath.Clear();
		m_openedFileStates.Clear();
	}

	bool CStructuredStorage::StorageExist( const TCHAR* pStorageName, IStorage* pParentDir /*= NULL*/ )
	{
		CPushIgnoreMode pushNoThrow( this );		// failure is not an error
		CComPtr< IStorage > pStorage = OpenDir( pStorageName, pParentDir, STGM_READ );
		return pStorage != NULL;
	}

	bool CStructuredStorage::StreamExist( const TCHAR* pStreamSubPath, IStorage* pParentDir /*= NULL*/ )
	{
		CPushIgnoreMode pushNoThrow( this );		// failure is not an error
		CComPtr< IStream > pStream = OpenStream( pStreamSubPath, pParentDir, STGM_READ );
		return pStream != NULL;
	}

	CComPtr< IStorage > CStructuredStorage::CreateDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->CreateStorage( storageName.c_str(), ToMode( mode ), 0, 0, &pDirStorage );
		HandleError( hResult, pDirName );
		return pDirStorage;
	}

	CComPtr< IStorage > CStructuredStorage::OpenDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring storageName = MakeShortFilename( pDirName );

		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStorage( storageName.c_str(), NULL, ToMode( mode ), NULL, 0, &pDirStorage );
		HandleError( hResult, pDirName );

		if ( pDirStorage != NULL && !HasFlag( mode, STGM_WRITE | STGM_READWRITE ) )		// opened for reading?
			CacheElementFileState( MakeElementSubPath( pDirName, pParentDir ), &*pDirStorage );		// cache the sub-storage metadata

		return pDirStorage;
	}

	bool CStructuredStorage::DeleteDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/ )
	{
		std::tstring storageName = MakeShortFilename( pDirName );
		fs::CPath dirSubPath = MakeElementSubPath( pDirName, pParentDir );

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( storageName.c_str() );

		if ( SUCCEEDED( hResult ) )
			m_openedFileStates.Remove( dirSubPath );

		return HandleError( hResult, pDirName );
	}

	CComPtr< IStream > CStructuredStorage::CreateStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		REQUIRE( IsOpen() );
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
		CComPtr< IStream > pFileStream;

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStream( streamName.c_str(), NULL, ToMode( mode ), 0, &pFileStream );

		if ( FAILED( hResult ) )
			HandleError( hResult, pStreamName );
		else if ( !HasFlag( mode, STGM_WRITE | STGM_READWRITE ) )		// opened for reading?
			CacheElementFileState( MakeElementSubPath( pStreamName, pParentDir ), &*pFileStream );		// cache the stream metadata

		return pFileStream;
	}

	bool CStructuredStorage::DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/ )
	{
		std::tstring streamName = EncodeStreamName( pStreamName );
		fs::CPath streamSubPath = MakeElementSubPath( pStreamName, pParentDir );

		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( streamName.c_str() );

		if ( SUCCEEDED( hResult ) )
			m_openedFileStates.Remove( streamSubPath );

		return HandleError( hResult, pStreamName );
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::CreateFile( const TCHAR* pFileName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeCreate | CFile::modeWrite*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring streamName = EncodeStreamName( pFileName );
		std::auto_ptr< COleStreamFile > pFile = MakeOleStreamFile( streamName.c_str() );

		if ( !pFile->CreateStream( pParentDir != NULL ? pParentDir : m_pRootStorage, streamName.c_str(), ToMode( mode ), &s_fileError ) )
			return HandleStreamError( pFileName );

		return pFile;
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::OpenFile( const TCHAR* pFileName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeRead*/ )
	{
		REQUIRE( IsOpen() );
		std::tstring streamName = EncodeStreamName( pFileName );
		std::auto_ptr< COleStreamFile > pFile = MakeOleStreamFile( streamName.c_str() );

		if ( !pFile->OpenStream( pParentDir != NULL ? pParentDir : m_pRootStorage, streamName.c_str(), ToMode( mode ), &s_fileError ) )
			return HandleStreamError( pFileName );

		return pFile;
	}

	std::tstring CStructuredStorage::EncodeStreamName( const TCHAR* pStreamName ) const
	{
		return MakeShortFilename( pStreamName );
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream /*= NULL*/ ) const
	{
		std::auto_ptr< COleStreamFile > pFile( new CAutoOleStreamFile( pStream ) );

		// store the stream path so it gets assigned in CArchive objects;
		// note: accurate with deep streams as well.
		pFile->SetFilePath( fs::CFlexPath::MakeComplexPath( GetDocFilePath().Get(), pStreamName ).GetPtr() );
		return pFile;
	}

	std::auto_ptr< COleStreamFile > CStructuredStorage::HandleStreamError( const TCHAR* pStreamName ) const
	{
		s_fileError.m_strFileName = CFlexPath::MakeComplexPath( GetDocFilePath().Get(), pStreamName ).GetPtr();
		if ( IsThrowMode() )
			throw &s_fileError;

		app::TraceException( &s_fileError );
		return std::auto_ptr< COleStreamFile >();
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

		s_fileError.m_lOsError = (LONG)hResult;
		s_fileError.m_strFileName = CFlexPath::MakeComplexPath( !str::IsEmpty( pDocFilePath ) ? pDocFilePath : GetDocFilePath().GetPtr(), pSubPath ).GetPtr();		// assign error file path

		if ( !IsThrowMode() )
		{
			app::TraceException( &s_fileError );
			return false;
		}

		AfxThrowFileException( s_fileError.m_cause, s_fileError.m_lOsError, s_fileError.m_strFileName );
	}


	template< typename StgInterfaceT >
	bool CStructuredStorage::CacheElementFileState( const fs::CPath& elementSubPath, StgInterfaceT* pInterface )
	{
		if ( IsElementOpen( elementSubPath ) )
			return false;			// only cache once

		fs::CFileState elemState;
		if ( !fs::stg::GetElementFullState( elemState, pInterface ) )
			return false;

		m_openedFileStates.Add( elementSubPath, elemState );
		return true;
	}

	fs::CPath CStructuredStorage::MakeElementSubPath( const TCHAR* pElementName, IStorage* pParentDir )
	{
		fs::CPath elementSubPath( pElementName );		// logical embedded path

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
		REQUIRE( !pDocStg->IsEmbeddedStorage() );

		const fs::CPath& docStgPath = pDocStg->GetDocFilePath();
		ENSURE( !docStgPath.IsEmpty() );

		TStorageMap& rOpenedStgs = GetOpenedDocStgs();
		rOpenedStgs.Add( docStgPath, pDocStg );
	}

	void CStructuredStorage::UnregisterDocStg( CStructuredStorage* pDocStg )
	{
		ASSERT_PTR( pDocStg );
		REQUIRE( !pDocStg->IsEmbeddedStorage() );

		const fs::CPath& docStgPath = pDocStg->GetDocFilePath();
		ASSERT( !docStgPath.IsEmpty() );

		TStorageMap& rOpenedStgs = GetOpenedDocStgs();
		VERIFY( rOpenedStgs.Remove( docStgPath ) );
	}

} //namespace fs


namespace fs
{
	// CAutoOleStreamFile implementation

	CAutoOleStreamFile::CAutoOleStreamFile( IStream* pStreamOpened /*= NULL*/ )
		: COleStreamFile( pStreamOpened )
	{
		if ( pStreamOpened != NULL )
		{
			pStreamOpened->AddRef();		// keep the stream alive when passed on contructor by e.g. OpenStream - it will be released on Close()
			TRACE( _T(" CAutoOleStreamFile::CAutoOleStreamFile() - after opening m_lpStream: ") ); TRACE_ITF( m_lpStream );
		}

		m_bCloseOnDelete = TRUE;			// will close on destructor
	}

	CAutoOleStreamFile::~CAutoOleStreamFile()
	{
		TRACE( _T(" CAutoOleStreamFile::~CAutoOleStreamFile() - before closing m_lpStream: ") ); TRACE_ITF( m_lpStream );
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

					if ( const fs::CFileState* pStreamState = pDocStorage->FindOpenedElement( embeddedPath ) )
						return pStreamState->m_fileSize;

					// try this first: the storage may have a flat representation for a deep embedded path (root streams with encoded deep names)
					CComPtr< IStream > pStream = pDocStorage->OpenStream( embeddedPath.GetPtr() );

					if ( pStream == NULL )
						pStream = fs::CStorageTrack::OpenEmbeddedStream( pDocStorage, embeddedPath );		// second: try to open the deep stream

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
