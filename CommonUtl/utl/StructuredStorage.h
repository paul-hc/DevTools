#ifndef StructuredStorage_h
#define StructuredStorage_h
#pragma once

#include "FlexPath.h"
#include "FileState.h"
#include "FileSystem.h"
#include "PathObjectMap.h"
#include "ThrowMode.h"
#include "RuntimeException.h"
#include <hash_map>
#include <afxole.h>


namespace fs
{
	class CStorageTrack;


	// Structured storage corresponding to a compound document file.
	// Able to handle long filenames for sub-storages (sub directories) and file methods.
	//
	// MSDN: at least STGM_SHARE_EXCLUSIVE/CFile::shareExclusive is mandatory when opening streams/files.

	class CStructuredStorage : public CThrowMode
							 , private utl::noncopyable
	{
		friend class fs::CStorageTrack;
	public:
		enum { MaxFilenameLen = 31 };

		CStructuredStorage( IStorage* pRootStorage = NULL );
		virtual ~CStructuredStorage();

		IStorage* GetStorage( void ) const { return m_pRootStorage; }
		bool IsEmbeddedStorage( void ) const { return m_isEmbeddedStg; }
		bool IsRootStorage( void ) const { return GetStorage() != NULL && !IsEmbeddedStorage(); }

		// storage
		bool IsOpen( void ) const { return m_pRootStorage != NULL; }
		DWORD GetOpenMode( void ) const { return m_openMode; }
		const fs::CPath& GetDocFilePath( void ) const;

		void Close( void );

		// document storage file
		bool CreateDocFile( const TCHAR* pDocFilePath, DWORD mode = STGM_CREATE | STGM_READWRITE );
		bool OpenDocFile( const TCHAR* pDocFilePath, DWORD mode = STGM_READ );
		static bool IsValidDocFile( const TCHAR* pDocFilePath );			// a compound document file?

		// embedded storages (sub-directories)
		CComPtr< IStorage > CreateDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr< IStorage > OpenDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteDir( const TCHAR* pDirName, IStorage* pParentDir = NULL );			// storage
		bool StorageExist( const TCHAR* pStorageName, IStorage* pParentDir = NULL );

		// embedded streams (files)
		CComPtr< IStream > CreateStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr< IStream > OpenStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL );
		bool StreamExist( const TCHAR* pStreamSubPath, IStorage* pParentDir = NULL );

		// embedded files (streams); caller must delete the returned file
		std::auto_ptr< COleStreamFile > CreateFile( const TCHAR* pFileName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeCreate | CFile::modeWrite );
		std::auto_ptr< COleStreamFile > OpenFile( const TCHAR* pFileName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeRead );

		// opened embedded streams file states
		bool IsElementOpen( const fs::CPath& streamName ) const { return m_openedFileStates.Contains( streamName ); }
		const fs::CFileState* FindOpenedElement( const fs::CPath& streamName ) const { return m_openedFileStates.Find( streamName ); }

		static DWORD ToMode( DWORD mode );										// augment mode with default STGM_SHARE_EXCLUSIVE (if no STGM_SHARE_DENY_* flag is present)
		static std::tstring MakeShortFilename( const TCHAR* pFilename );		// make short file name with length limited to MaxFilenameLen

		static CStructuredStorage* FindOpenedStorage( const fs::CPath& docStgPath );
	protected:
		virtual std::tstring EncodeStreamName( const TCHAR* pStreamName ) const;

		std::auto_ptr< COleStreamFile > MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream = NULL ) const;

		bool HandleError( HRESULT hResult, const TCHAR* pSubPath, const TCHAR* pDocFilePath = NULL ) const;
		std::auto_ptr< COleStreamFile > HandleStreamError( const TCHAR* pStreamName ) const;			// may throw s_fileError

		template< typename StgInterfaceT >
		bool CacheElementFileState( const fs::CPath& elementSubPath, StgInterfaceT* pInterface );

		static fs::CPath MakeElementSubPath( const TCHAR* pElementName, IStorage* pParentDir );
	private:
		typedef fs::CPathObjectMap< fs::CPath, CStructuredStorage > TStorageMap;		// document path to open root storage

		static TStorageMap& GetOpenedDocStgs( void );			// singleton: opened document storages - only root storages!

		static void RegisterDocStg( CStructuredStorage* pDocStg );
		static void UnregisterDocStg( CStructuredStorage* pDocStg );
	private:
		CComPtr< IStorage > m_pRootStorage;			// root directory
		const bool m_isEmbeddedStg;					// true for storages passed on constructor (not opened internally)
		DWORD m_openMode;
		mutable fs::CPath m_docFilePath;			// self-encapsulated

		typedef fs::CPathMap< fs::CPath, fs::CFileState > TElementStates;		// keys are embedded logical paths, whereas CFileState::m_fullPath is the physical element path

		TElementStates m_openedFileStates;			// metadata of opened sub-storages & streams (for reading only - immutable)
	protected:
		static mfc::CAutoException< CFileException > s_fileError;
	};
}


namespace fs
{
	class CAutoOleStreamFile : public COleStreamFile		// auto-close on destructor
	{
	public:
		CAutoOleStreamFile( IStream* pStreamOpened = NULL );
		virtual ~CAutoOleStreamFile();
	};


	namespace stg
	{
		class CAcquireStorage : private utl::noncopyable
		{
		public:
			CAcquireStorage( const fs::CPath& docStgPath, DWORD mode = STGM_READ )
				: m_pFoundOpenStg( CStructuredStorage::FindOpenedStorage( docStgPath ) )
			{
				if ( NULL == m_pFoundOpenStg )
					m_tempStorage.OpenDocFile( docStgPath.GetPtr(), mode );
			}

			~CAcquireStorage()
			{
			}

			CStructuredStorage* Get( void )
			{
				if ( m_pFoundOpenStg != NULL )
					return m_pFoundOpenStg;
				else if ( m_tempStorage.IsOpen() )
					return &m_tempStorage;

				return NULL;
			}
		private:
			CStructuredStorage* m_pFoundOpenStg;
			fs::CStructuredStorage m_tempStorage;
		};
	}

} //namespace fs


namespace fs
{
	namespace flex
	{
		// API for file streams and embedded file streams
		CComPtr< IStream > OpenStreamOnFile( const fs::CFlexPath& filePath, DWORD mode = STGM_READ, DWORD createAttributes = 0 );

		UINT64 GetFileSize( const fs::CFlexPath& filePath );
		CTime ReadLastModifyTime( const fs::CFlexPath& filePath );			// for embedded images use the stg doc time

		FileExpireStatus CheckExpireStatus( const fs::CFlexPath& filePath, const CTime& lastModifyTime );
	}
}


namespace fs
{
	struct CFileState;


	namespace stg
	{
		bool MakeFileState( fs::CFileState& rState, const STATSTG& stat );


		// functions that work uniformly for IStorage/IStream/ILockBytes

		template< typename StgInterfaceT >
		bool GetElementState( fs::CFileState& rState, StgInterfaceT* pInterface, STATFLAG flag = STATFLAG_NONAME )		// works for IStorage/IStream/ILockBytes
		{
			ASSERT_PTR( pInterface );
			STATSTG stat;
			return
				HR_OK( pInterface->Stat( &stat, flag ) ) &&
				MakeFileState( rState, stat );
		}

		template< typename StgInterfaceT >
		inline bool GetElementFullState( fs::CFileState& rState, StgInterfaceT* pInterface ) { return GetElementState( rState, pInterface, STATFLAG_DEFAULT ); }

		template< typename StgInterfaceT >
		inline fs::CPath GetElementName( StgInterfaceT* pInterface )
		{
			fs::CFileState state;
			GetElementState( state, pInterface, STATFLAG_DEFAULT );
			return state.m_fullPath;
		}

		template< typename StgInterfaceT >
		inline UINT64 GetStreamSize( StgInterfaceT* pInterface )
		{
			ASSERT( is_a< IStream >( pInterface ) || is_a< ILockBytes >( pInterface ) );		// it reports 0 size on sub-storages
			fs::CFileState state;
			GetElementState( state, pInterface, STATFLAG_DEFAULT );
			return state.m_fileSize;
		}

		template< typename StgInterfaceT >
		inline CTime GetLastModifiedTime( StgInterfaceT* pInterface )
		{
			fs::CFileState state;
			GetElementState( state, pInterface, STATFLAG_DEFAULT );
			return state.m_modifTime;
		}
	}
}


#endif // StructuredStorage_h
