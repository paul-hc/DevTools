
#include "stdafx.h"
#include "StructuredStorage.h"
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

	const fs::CPath& CStructuredStorage::GetDocFilePath( void ) const
	{
		if ( m_docFilePath.IsEmpty() && IsOpen() )
			m_docFilePath.Set( fs::GetElementName( &*m_pRootStorage ) );

		return m_docFilePath;
	}

	DWORD CStructuredStorage::ToMode( DWORD mode )
	{
		enum { ShareMask = 0x000000F0 };		// covers for STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE

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

	bool CStructuredStorage::Create( const TCHAR* pStgFilePath, DWORD mode /*= STGM_CREATE | STGM_WRITE*/ )
	{
		if ( IsOpen() )
			Close();

		HRESULT hResult = HR_AUDIT( ::StgCreateDocfile( CStringW( pStgFilePath ), ToMode( mode ), 0, &m_pRootStorage ) );
		if ( SUCCEEDED( hResult ) && !IsEmbeddedStorage() )
			RegisterDocStg( this );

		return HandleError( hResult, pStgFilePath );
	}

	bool CStructuredStorage::Open( const TCHAR* pStgFilePath, DWORD mode /*= STGM_READ*/ )
	{
		if ( IsOpen() )
			Close();

		HRESULT hResult = HR_AUDIT( ::StgOpenStorage( CStringW( pStgFilePath ), NULL, ToMode( mode ), NULL, 0, &m_pRootStorage ) );		// StgOpenStorageEx
		if ( FAILED( hResult ) )
			return HandleError( hResult, pStgFilePath );

		m_openMode = ToMode( mode );
		if ( !IsEmbeddedStorage() )
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
	}

	bool CStructuredStorage::StorageExist( const TCHAR* pStorageName, IStorage* pParentDir /*= NULL*/ )
	{
		CPushThrowMode pushNoThrow( this, false );
		CComPtr< IStorage > pStorage = OpenDir( pStorageName, pParentDir, STGM_READ );
		return pStorage != NULL;
	}

	bool CStructuredStorage::StreamExist( const TCHAR* pStreamSubPath, IStorage* pParentDir /*= NULL*/ )
	{
		CPushThrowMode pushNoThrow( this, false );
		CComPtr< IStream > pStream = OpenStream( pStreamSubPath, pParentDir, STGM_READ );
		return pStream != NULL;
	}

	CComPtr< IStorage > CStructuredStorage::CreateDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_WRITE*/ )
	{
		ASSERT( IsOpen() );
		CStringW storageName = MakeShortFilename( pDirName ).c_str();

		ASSERT( IsOpen() );
		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->CreateStorage( storageName, ToMode( mode ), 0, 0, &pDirStorage );
		HandleError( hResult );
		return pDirStorage;
	}

	CComPtr< IStorage > CStructuredStorage::OpenDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT( IsOpen() );
		CStringW storageName = MakeShortFilename( pDirName ).c_str();

		ASSERT( IsOpen() );
		CComPtr< IStorage > pDirStorage;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStorage( storageName, NULL, ToMode( mode ), NULL, 0, &pDirStorage );
		HandleError( hResult );
		return pDirStorage;
	}

	CComPtr< IStorage > CStructuredStorage::OpenDeepDir( const TCHAR* pDirSubPath, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		CComPtr< IStorage > pSubDir = pParentDir != NULL ? pParentDir : m_pRootStorage;
		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end() && pSubDir != NULL; ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );
			pSubDir = OpenDir( itSubDir->c_str(), pSubDir, mode );
		}
		return pSubDir;
	}

	bool CStructuredStorage::DeleteDir( const TCHAR* pDirName, IStorage* pParentDir /*= NULL*/ )
	{
		CStringW storageName = MakeShortFilename( pDirName ).c_str();
		return HandleError( ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( storageName ) );
	}

	CComPtr< IStream > CStructuredStorage::CreateStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_CREATE | STGM_WRITE*/ )
	{
		ASSERT( IsOpen() );
		CStringW streamName = EncodeStreamName( pStreamName );
		CComPtr< IStream > pFileStream;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->CreateStream( streamName, ToMode( mode ), NULL, 0, &pFileStream );
		HandleError( hResult );
		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::OpenStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT( IsOpen() );
		CStringW streamName = EncodeStreamName( pStreamName );
		CComPtr< IStream > pFileStream;
		HRESULT hResult = ( pParentDir != NULL ? pParentDir : m_pRootStorage )->OpenStream( streamName, NULL, ToMode( mode ), 0, &pFileStream );
		HandleError( hResult );
		return pFileStream;
	}

	CComPtr< IStream > CStructuredStorage::OpenDeepStream( const TCHAR* pStreamSubPath, IStorage* pParentDir /*= NULL*/, DWORD mode /*= STGM_READ*/ )
	{
		fs::CPathParts parts( pStreamSubPath );
		ASSERT( parts.m_drive.empty() );

		CComPtr< IStorage > pSubDir = OpenDeepDir( parts.GetDirPath().GetPtr(), pParentDir );		// use default mode for sub-dir storages
		if ( NULL == pSubDir )
			return NULL;
		return OpenStream( parts.GetNameExt().c_str(), pSubDir, mode );
	}

	bool CStructuredStorage::DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir /*= NULL*/ )
	{
		CStringW streamName = EncodeStreamName( pStreamName );
		return HandleError( ( pParentDir != NULL ? pParentDir : m_pRootStorage )->DestroyElement( streamName ) );
	}

	COleStreamFile* CStructuredStorage::CreateFile( const TCHAR* pFileName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeCreate | CFile::modeWrite*/ )
	{
		ASSERT( IsOpen() );
		CStringW streamName = EncodeStreamName( pFileName );
		std::auto_ptr< COleStreamFile > pFile( NewOleStreamFile( streamName ) );

		if ( !pFile->CreateStream( pParentDir != NULL ? pParentDir : m_pRootStorage, streamName, ToMode( mode ), &s_fileError ) )
			return HandleStreamError( pFileName );

		return pFile.release();
	}

	COleStreamFile* CStructuredStorage::OpenFile( const TCHAR* pFileName, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeRead*/ )
	{
		ASSERT( IsOpen() );
		CStringW streamName = EncodeStreamName( pFileName );
		std::auto_ptr< COleStreamFile > pFile( NewOleStreamFile( streamName ) );

		if ( !pFile->OpenStream( pParentDir != NULL ? pParentDir : m_pRootStorage, streamName, ToMode( mode ), &s_fileError ) )
			return HandleStreamError( pFileName );

		return pFile.release();
	}

	COleStreamFile* CStructuredStorage::OpenDeepFile( const TCHAR* pFileSubPath, IStorage* pParentDir /*= NULL*/, DWORD mode /*= CFile::modeRead*/ )
	{
		ASSERT( IsOpen() );

		CComPtr< IStream > pStream = OpenDeepStream( pFileSubPath, pParentDir, mode );		// CFile::OpenFlags match STGM_* flags
		return pStream != NULL ? NewOleStreamFile( pFileSubPath, pStream ) : NULL;
	}

	CStringW CStructuredStorage::EncodeStreamName( const TCHAR* pStreamName ) const
	{
		return MakeShortFilename( pStreamName ).c_str();
	}

	COleStreamFile* CStructuredStorage::NewOleStreamFile( const TCHAR* pStreamName, IStream* pStream /*= NULL*/ ) const
	{
		COleStreamFile* pFile = new CAutoOleStreamFile( pStream );

		// store the stream path so it gets assigned in CArchive objects;
		// note: accurate with deep streams as well.
		pFile->SetFilePath( fs::CFlexPath::MakeComplexPath( GetDocFilePath().Get(), pStreamName ).GetPtr() );
		return pFile;
	}

	COleStreamFile* CStructuredStorage::HandleStreamError( const TCHAR* pStreamName ) const
	{
		s_fileError.m_strFileName = CFlexPath::MakeComplexPath( GetDocFilePath().Get(), pStreamName ).GetPtr();
		if ( IsThrowMode() )
			throw &s_fileError;
		else
		{
			app::TraceException( &s_fileError );
			return NULL;
		}
	}

	bool CStructuredStorage::HandleError( HRESULT hResult, const TCHAR* pStgFilePath /*= NULL*/ ) const
	{
		if ( SUCCEEDED( hResult ) )
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

		if ( !IsThrowMode() )
		{
			app::TraceException( &s_fileError );
			return false;
		}

		AfxThrowFileException( s_fileError.m_cause, s_fileError.m_lOsError, !str::IsEmpty( pStgFilePath ) ? pStgFilePath : GetDocFilePath().GetPtr() );
	}

	stdext::hash_map< fs::CPath, CStructuredStorage* >& CStructuredStorage::GetDocStgs( void )
	{
		static stdext::hash_map< fs::CPath, CStructuredStorage* > openedStgs;
		return openedStgs;
	}

	CStructuredStorage* CStructuredStorage::FindOpenedStorage( const fs::CPath& stgFilePath )
	{
		const stdext::hash_map< fs::CPath, CStructuredStorage* >& rDocStgs = GetDocStgs();
		stdext::hash_map< fs::CPath, CStructuredStorage* >::const_iterator itFound = rDocStgs.find( stgFilePath );
		return itFound != rDocStgs.end() ? itFound->second : NULL;
	}

	void CStructuredStorage::RegisterDocStg( CStructuredStorage* pDocStg )
	{
		ASSERT_PTR( pDocStg );
		ASSERT( !pDocStg->IsEmbeddedStorage() );

		const fs::CPath& docFilePath = pDocStg->GetDocFilePath();
		ASSERT( NULL == FindOpenedStorage( docFilePath ) );
		ASSERT( !docFilePath.IsEmpty() );

		stdext::hash_map< fs::CPath, CStructuredStorage* >& rDocStgs = GetDocStgs();
		rDocStgs[ docFilePath ] = pDocStg;
	}

	void CStructuredStorage::UnregisterDocStg( CStructuredStorage* pDocStg )
	{
		ASSERT_PTR( pDocStg );
		ASSERT( !pDocStg->IsEmbeddedStorage() );

		const fs::CPath& docFilePath = pDocStg->GetDocFilePath();
		ASSERT( !docFilePath.IsEmpty() );

		stdext::hash_map< fs::CPath, CStructuredStorage* >& rDocStgs = GetDocStgs();
		VERIFY( 1 == rDocStgs.erase( docFilePath ) );
	}

} //namespace fs


namespace fs
{
	CComPtr< IStream > OpenStreamOnFile( const fs::CFlexPath& filePath, DWORD mode /*= STGM_READ*/, DWORD createAttributes /*= 0*/ )
	{
		CComPtr< IStream > pStream;

		if ( !filePath.IsComplexPath() )
			::SHCreateStreamOnFileEx( filePath.GetPtr(), mode, createAttributes, FALSE, NULL, &pStream );
		else
		{
			CAcquireStorage stg( filePath.GetPhysicalPath(), mode );
			if ( stg.Get() != NULL )
				pStream = stg.Get()->OpenStream( filePath.GetEmbeddedPath(), NULL, mode );
		}
		if ( NULL == pStream )
			TRACE( _T(" * fs::OpenStreamOnFile() failed to open file %s\n"), filePath.GetPtr() );
		return pStream;
	}


	namespace flex
	{
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
	bool MakeFileStatus( CFileStatus& rStatus, const STATSTG& statStg )
	{
		if ( !CTime::IsValidFILETIME( statStg.mtime ) ||
			 !CTime::IsValidFILETIME( statStg.ctime ) ||
			 !CTime::IsValidFILETIME( statStg.atime ) )
			return false;

		// odly enough, on IStream-s all of statStg data-members mtime, ctime, atime are set to null - CTime(0)
		rStatus.m_mtime = CTime( statStg.mtime );
		rStatus.m_ctime = CTime( statStg.ctime );
		rStatus.m_atime = CTime( statStg.atime );
		ASSERT( 0 == statStg.cbSize.HighPart );			// not a huge stream
		rStatus.m_size = statStg.cbSize.LowPart;
		rStatus.m_attribute = 0;
		rStatus.m_szFullName[ 0 ] = _T('\0');
		if ( statStg.pwcsName != NULL )
		{
			const CString path( statStg.pwcsName );

			// name was returned -- copy and free it
			Checked::tcsncpy_s( rStatus.m_szFullName, COUNT_OF( rStatus.m_szFullName ), path.GetString(), _TRUNCATE );
			::CoTaskMemFree( statStg.pwcsName );
		}
		return true;
	}
}
