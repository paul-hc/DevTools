
#include "stdafx.h"
#include "StorageTrack.h"
#include "StructuredStorage.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	CStorageTrack::CStorageTrack( CStructuredStorage* pRootStorage )
		: m_pRootStorage( pRootStorage )
	{
		ASSERT_PTR( m_pRootStorage );
		REQUIRE( m_pRootStorage->IsOpen() );
	}

	CStorageTrack::~CStorageTrack()
	{
	}

	IStorage* CStorageTrack::GetRoot( void ) const
	{
		return m_pRootStorage->GetStorage();
	}

	void CStorageTrack::Push( IStorage* pSubStorage )
	{
		ASSERT_PTR( pSubStorage );
		m_openSubStorages.push_back( pSubStorage );
	}

	CComPtr< IStorage > CStorageTrack::Pop( void )
	{
		ASSERT( !m_openSubStorages.empty() );

		CComPtr< IStorage > pDeepStorage = m_openSubStorages.back();

		m_openSubStorages.pop_back();
		return pDeepStorage;
	}

	std::tstring CStorageTrack::MakeSubPath( void ) const
	{
		std::tstring subPath;

		if ( !IsEmpty() )
		{
			for ( std::vector< CComPtr< IStorage > >::const_iterator itSubStorage = m_openSubStorages.begin(); itSubStorage != m_openSubStorages.end(); ++itSubStorage )
			{
				STATSTG stat;
				( *itSubStorage )->Stat( &stat, STATFLAG_DEFAULT );

				if ( !subPath.empty() )
					subPath += _T('/');

				subPath += stat.pwcsName;

				::CoTaskMemFree( stat.pwcsName );			// free the memory allocated by Stat()
			}
		}

		return subPath;
	}

	bool CStorageTrack::CreateDeepSubPath( const TCHAR* pDirSubPath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		DWORD openMode = MakeFlag( mode, STGM_CREATE, false );

		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end(); ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );

			CComPtr< IStorage > pStorage;
			{	// first: try to open if it already exists
				CPushIgnoreMode pushNoThrow( m_pRootStorage );		// failure is not an error
				pStorage = m_pRootStorage->OpenDir( itSubDir->c_str(), GetCurrent(), openMode );
			}

			if ( pStorage == NULL )
				pStorage = m_pRootStorage->CreateDir( itSubDir->c_str(), GetCurrent(), mode );		// second: create the sub-storage

			if ( pStorage != NULL )
				Push( pStorage );					// advance one level deeper
			else
				return false;						// could not open/create sub-storage
		}

		return true;			// all sub-storages successfully opened/created
	}

	bool CStorageTrack::OpenDeepSubPath( const TCHAR* pDirSubPath, DWORD mode /*= STGM_READ*/ )
	{
		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end(); ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );

			if ( CComPtr< IStorage > pStorage = m_pRootStorage->OpenDir( itSubDir->c_str(), GetCurrent(), mode ) )		// open existing storage
				Push( pStorage );					// advance one level deeper
			else
				return false;						// could not open sub-storage
		}

		return true;			// all sub-storages successfully opened
	}


	CComPtr< IStorage > CStorageTrack::OpenEmbeddedStorage( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& dirSubPath, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT_PTR( pRootStorage );
		REQUIRE( path::IsRelative( dirSubPath.GetPtr() ) );

		CStorageTrack storageTrack( pRootStorage );
		if ( !storageTrack.OpenDeepSubPath( dirSubPath.GetPtr(), mode ) )
			return NULL;

		return storageTrack.GetCurrent();
	}

	CComPtr< IStream > CStorageTrack::OpenEmbeddedStream( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT_PTR( pRootStorage );
		REQUIRE( path::IsRelative( streamSubPath.GetPtr() ) );

		if ( CComPtr< IStorage > pParentStorage = OpenEmbeddedStorage( pRootStorage, streamSubPath.GetParentPath(), mode ) )
			return pRootStorage->OpenStream( streamSubPath.GetNameExt(), pParentStorage, mode );

		return NULL;
	}

	std::auto_ptr< COleStreamFile > CStorageTrack::OpenEmbeddedFile( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode /*= CFile::modeRead*/ )
	{
		if ( CComPtr< IStream > pStream = OpenEmbeddedStream( pRootStorage, streamSubPath, mode ) )		// CFile::OpenFlags match STGM_* flags
			return pRootStorage->MakeOleStreamFile( streamSubPath.GetNameExt(), pStream );

		return std::auto_ptr< COleStreamFile >();
	}
}
