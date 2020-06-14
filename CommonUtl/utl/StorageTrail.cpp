
#include "stdafx.h"
#include "StorageTrail.h"
#include "StructuredStorage.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	CStorageTrail::CStorageTrail( CStructuredStorage* pRootStorage )
		: m_pRootStorage( pRootStorage )
	{
		ASSERT_PTR( m_pRootStorage );
		REQUIRE( m_pRootStorage->IsOpen() );
	}

	CStorageTrail::~CStorageTrail()
	{
	}

	IStorage* CStorageTrail::GetRoot( void ) const
	{
		return m_pRootStorage->GetRootStorage();
	}

	void CStorageTrail::Clear( void )
	{
		while ( !m_openSubStorages.empty() )
			m_openSubStorages.pop_back();
	}

	void CStorageTrail::Push( IStorage* pSubStorage )
	{
		ASSERT_PTR( pSubStorage );
		m_openSubStorages.push_back( pSubStorage );
	}

	CComPtr< IStorage > CStorageTrail::Pop( void )
	{
		ASSERT( !m_openSubStorages.empty() );

		CComPtr< IStorage > pDeepStorage = m_openSubStorages.back();

		m_openSubStorages.pop_back();
		return pDeepStorage;
	}

	fs::TEmbeddedPath CStorageTrail::MakePath( void ) const
	{
		fs::TEmbeddedPath subPath;

		for ( std::vector< CComPtr< IStorage > >::const_iterator itSubStorage = m_openSubStorages.begin(); itSubStorage != m_openSubStorages.end(); ++itSubStorage )
			subPath /= fs::stg::GetElementName( &**itSubStorage );

		return subPath;
	}


	void CStorageTrail::EnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth /*= Shallow*/ ) throws_( CFileException* )
	{
		CScopedErrorHandling scopedThrow( m_pRootStorage, utl::ThrowMode );		// to simplify recovery from error

		DoEnumElements( pEnumerator, depth );
	}

	void CStorageTrail::DoEnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth ) throws_( CFileException* )
	{
		ASSERT_PTR( pEnumerator );

		IStorage* pCurrStorage = GetCurrent();
		fs::TEmbeddedPath embeddedPath = MakePath();

		CComPtr< IEnumSTATSTG > pEnumStat;

		HRESULT hResult = pCurrStorage->EnumElements( 0, NULL, 0, &pEnumStat );
		if ( FAILED( hResult ) )
			m_pRootStorage->HandleError( hResult, embeddedPath.GetPtr() );

		STATSTG stat;
		std::vector< fs::CPath > subStgNames;

		for ( ;; )
		{
			hResult = pEnumStat->Next( 1, &stat, NULL );
			if ( FAILED( hResult ) )
				m_pRootStorage->HandleError( hResult, embeddedPath.GetPtr() );
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

		for ( std::vector< fs::CPath >::const_iterator itSubStgName = subStgNames.begin(); itSubStgName != subStgNames.end(); ++itSubStgName )
		{
			fs::TEmbeddedPath subDirPath = embeddedPath / *itSubStgName;
			if ( pEnumerator->AddFoundSubDir( subDirPath.GetPtr() ) )		// sub-storage is not ignored?
				if ( Deep == depth && !pEnumerator->MustStop() )
				{
					Push( m_pRootStorage->OpenDir( itSubStgName->GetPtr(), pCurrStorage ) );
					DoEnumElements( pEnumerator, Deep );
					Pop();
				}
		}
	}


	bool CStorageTrail::CreateDeepSubPath( const TCHAR* pDirSubPath, DWORD mode /*= STGM_CREATE | STGM_READWRITE*/ )
	{
		std::vector< std::tstring > subDirs;
		str::Tokenize( subDirs, pDirSubPath, path::DirDelims() );

		DWORD openMode = MakeFlag( mode, STGM_CREATE, false );

		for ( std::vector< std::tstring >::const_iterator itSubDir = subDirs.begin(); itSubDir != subDirs.end(); ++itSubDir )
		{
			ASSERT( !itSubDir->empty() );

			CComPtr< IStorage > pStorage;
			{	// first: try to open if it already exists
				CScopedErrorHandling scopedCheck( m_pRootStorage, utl::CheckMode );		// failure should not throw

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

	bool CStorageTrail::OpenDeepSubPath( const TCHAR* pDirSubPath, DWORD mode /*= STGM_READ*/ )
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


	CComPtr< IStorage > CStorageTrail::OpenEmbeddedStorage( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& dirSubPath, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT_PTR( pRootStorage );
		REQUIRE( path::IsRelative( dirSubPath.GetPtr() ) );

		CStorageTrail storageTrack( pRootStorage );
		if ( !storageTrack.OpenDeepSubPath( dirSubPath.GetPtr(), mode ) )
			return NULL;

		return storageTrack.GetCurrent();
	}

	CComPtr< IStream > CStorageTrail::OpenEmbeddedStream( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode /*= STGM_READ*/ )
	{
		ASSERT_PTR( pRootStorage );
		REQUIRE( path::IsRelative( streamSubPath.GetPtr() ) );

		if ( CComPtr< IStorage > pParentStorage = OpenEmbeddedStorage( pRootStorage, streamSubPath.GetParentPath(), mode ) )
			return pRootStorage->OpenStream( streamSubPath.GetNameExt(), pParentStorage, mode );

		return NULL;
	}

	std::auto_ptr< COleStreamFile > CStorageTrail::OpenEmbeddedFile( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode /*= CFile::modeRead*/ )
	{
		if ( CComPtr< IStream > pStream = OpenEmbeddedStream( pRootStorage, streamSubPath, mode ) )		// CFile::OpenFlags match STGM_* flags
			return pRootStorage->MakeOleStreamFile( streamSubPath.GetNameExt(), pStream );

		return std::auto_ptr< COleStreamFile >();
	}
}
