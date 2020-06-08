#ifndef StorageTrack_h
#define StorageTrack_h
#pragma once

#include <afxole.h>			// for COleStreamFile
#include "FileSystem_fwd.h"


namespace fs
{
	class CStructuredStorage;
	typedef fs::CPath TEmbeddedPath;


	// An owning stack of deep opened storages starting from the root, of which the current (deepest) storage is at back.
	// It represents an open sub-directory path to the current (deepest) storage.
	//
	class CStorageTrack : private utl::noncopyable
	{
	public:
		CStorageTrack( CStructuredStorage* pRootStorage );
		~CStorageTrack();

		bool IsEmpty( void ) const { return m_openSubStorages.empty(); }
		IStorage* GetRoot( void ) const;

		void Clear( void );							// pop all sub-storages
		void Push( IStorage* pSubStorage );			// push a sub-storage
		CComPtr< IStorage > Pop( void );

		IStorage* GetCurrent( void ) const { return !IsEmpty() ? m_openSubStorages.back() : GetRoot(); }
		fs::TEmbeddedPath MakeSubPath( void ) const;

		size_t GetDepth( void ) const { return m_openSubStorages.size(); }
		IStorage* GetStorageAtLevel( size_t depthLevel ) const { ASSERT( depthLevel < GetDepth() ); return m_openSubStorages[ depthLevel ]; }

		// enumerate storages and streams (embedded storage paths, relative from the root)
		void EnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth = Shallow ) throws_( CFileException* );

		// deep storage access
		bool CreateDeepSubPath( const TCHAR* pDirSubPath, DWORD mode = STGM_CREATE | STGM_READWRITE );
		bool OpenDeepSubPath( const TCHAR* pDirSubPath, DWORD mode = STGM_READ );		// use STGM_READWRITE for writing (STGM_WRITE seems to be failing)

		static CComPtr< IStorage > OpenEmbeddedStorage( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& dirSubPath, DWORD mode = STGM_READ );
		static CComPtr< IStream > OpenEmbeddedStream( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode = STGM_READ );

		static std::auto_ptr< COleStreamFile > OpenEmbeddedFile( CStructuredStorage* pRootStorage, const fs::TEmbeddedPath& streamSubPath, DWORD mode = CFile::modeRead );
	private:
		void DoEnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth ) throws_( CFileException* );
	private:
		CStructuredStorage* m_pRootStorage;
		std::vector< CComPtr< IStorage > > m_openSubStorages;		// opened embedded storages
	};
}


#endif // StorageTrack_h
