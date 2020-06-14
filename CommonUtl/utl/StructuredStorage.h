#ifndef StructuredStorage_h
#define StructuredStorage_h
#pragma once

#include "FlexPath.h"
#include "FileSystem.h"
#include "PathMap.h"
#include "ErrorHandler.h"
#include "RuntimeException.h"
#include <hash_map>
#include <afxole.h>


namespace fs
{
	class CStorageTrail;
	struct CStreamState;


	// Structured storage corresponding to a compound document file.
	// Able to handle long filenames for sub-storages (sub directories) and stream (file) methods.
	//
	// MSDN: at least STGM_SHARE_EXCLUSIVE/CFile::shareExclusive is mandatory when opening streams/files.

	class CStructuredStorage : public CErrorHandler
							 , private utl::noncopyable
	{
		friend class fs::CStorageTrail;
	public:
		CStructuredStorage( void );
		virtual ~CStructuredStorage();

		enum { MaxFilenameLen = 31 };			// limitation imposed by the COM standard implementation of a compound document storage

		void SetUseStreamSharing( bool useStreamSharing = true );

		IStorage* GetRootStorage( void ) const { return m_pRootStorage; }

		// storage
		bool IsOpen( void ) const { return m_pRootStorage != NULL; }
		DWORD GetOpenMode( void ) const { return m_openMode; }
		const fs::CPath& GetDocFilePath( void ) const;

		void Close( void );

		// document storage file
		bool CreateDocFile( const TCHAR* pDocFilePath, DWORD mode = STGM_CREATE | STGM_READWRITE );
		bool OpenDocFile( const TCHAR* pDocFilePath, DWORD mode = STGM_READ );
		static bool IsValidDocFile( const TCHAR* pDocFilePath );						// an compound document file that exists?

		// embedded storages (sub-directories)
		CComPtr< IStorage > CreateDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr< IStorage > OpenDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteDir( const TCHAR* pDirName, IStorage* pParentDir = NULL );			// storage
		bool StorageExist( const TCHAR* pStorageName, IStorage* pParentDir = NULL );

		// embedded streams (files)
		CComPtr< IStream > CreateStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr< IStream > OpenStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL );

		bool CloseStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL );		// logical closing, to unload all references to the cached stream and stream clones
		bool StreamExist( const TCHAR* pStreamSubPath, IStorage* pParentDir = NULL );

		// opened embedded streams states
		bool IsStreamOpen( const fs::TEmbeddedPath& streamPath ) const { return m_openedStreamStates.Contains( streamPath ); }
		const fs::CStreamState* FindOpenedStream( const fs::TEmbeddedPath& streamPath ) const { return m_openedStreamStates.Find( streamPath ); }

		// embedded files (on streams); caller must delete the returned file
		std::auto_ptr< COleStreamFile > CreateStreamFile( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeCreate | CFile::modeWrite );
		std::auto_ptr< COleStreamFile > OpenStreamFile( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeRead );

		// backwards compatibility: find existing object based on possible alternates
		std::pair< const TCHAR*, size_t > FindAlternate_DirName( const TCHAR* altDirNames[], size_t altCount, IStorage* pParentDir = NULL );
		std::pair< const TCHAR*, size_t > FindAlternate_StreamName( const TCHAR* altStreamNames[], size_t altCount, IStorage* pParentDir = NULL );

		static DWORD ToMode( DWORD mode );										// augment mode with default STGM_SHARE_EXCLUSIVE (if no STGM_SHARE_DENY_* flag is present)
		static std::tstring MakeShortFilename( const TCHAR* pFilename );		// make short file name with length limited to MaxFilenameLen

		static CStructuredStorage* FindOpenedStorage( const fs::CPath& docStgPath );
	protected:
		// overridables
		virtual std::tstring EncodeStreamName( const TCHAR* pStreamName ) const;
		virtual bool RetainOpenedStream( const TCHAR* pStreamName, IStorage* pParentDir ) const;	// if true: cache opened streams to allow later shared access (workaround sharing violations caused by STGM_SHARE_EXCLUSIVE)

		std::auto_ptr< COleStreamFile > MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream = NULL ) const;

		bool HandleError( HRESULT hResult, const TCHAR* pSubPath, const TCHAR* pDocFilePath = NULL ) const;

		static bool IsReadingMode( DWORD mode ) { return !HasFlag( mode, STGM_WRITE | STGM_READWRITE ); }
	private:
		IStorage* GetParentDir( const CStorageTrail* pParentTrail ) const;

		bool CacheStreamState( const TCHAR* pStreamName, IStorage* pParentDir, IStream* pStreamOrigin );
		size_t CloseStreamsWithPrefix( const TCHAR* pSubDirPrefix ) { return m_openedStreamStates.RemoveWithPrefix( pSubDirPrefix ); }

		CComPtr< IStream > CloneStream( const fs::CStreamState* pStreamState, const fs::TEmbeddedPath& streamPath );		// for stream logical re-opening

		static fs::TEmbeddedPath MakeElementSubPath( const TCHAR* pElementName, IStorage* pParentDir );
	private:
		typedef fs::CPathObjectMap< fs::CPath, CStructuredStorage > TStorageMap;		// document path to open root storage

		static TStorageMap& GetOpenedDocStgs( void );			// singleton: opened document storages - only root storages!

		static void RegisterDocStg( CStructuredStorage* pDocStg );
		static void UnregisterDocStg( CStructuredStorage* pDocStg );
	private:
		CComPtr< IStorage > m_pRootStorage;		// root storage in a compound document file
		bool m_useStreamSharing;				// keeps opened streams alive via caching, allowing stream cloning to circumvent exclusive sharing violations
		DWORD m_openMode;
		mutable fs::CPath m_docFilePath;		// self-encapsulated: full path of the document storage file

		typedef fs::CPathMap< fs::TEmbeddedPath, fs::CStreamState > TStreamStates;		// keys are embedded logical paths, whereas CFileState::m_fullPath is the physical element path

		TStreamStates m_openedStreamStates;		// metadata of opened streams (for reading only - immutable)
	protected:
		static mfc::CAutoException< CFileException > s_fileError;
	};
}


#include "FileState.h"


namespace fs
{
	struct CStreamState : public fs::CFileState
	{
		CStreamState( void ) {}

		bool Build( IStream* pStreamOrigin, bool keepStreamAlive );

		bool HasStream( void ) const { return m_pStreamOrigin != NULL; }
		void ReleaseStream( void ) { m_pStreamOrigin = NULL; }
		HRESULT CloneStream( IStream** ppStreamDuplicate ) const;
	private:
		CComPtr< IStream > m_pStreamOrigin;			// stream sharing: optional, keeps the original opened stream, that can be cloned for subsequent stream reading
	};


	// stream utilities

	bool SeekToPos( IStream* pStream, UINT64 position, DWORD origin = STREAM_SEEK_SET );
	inline bool SeekToBegin( IStream* pStream ) { return SeekToPos( pStream, 0 ); }
	inline bool SeekToEnd( IStream* pStream ) { return SeekToPos( pStream, 0, STREAM_SEEK_END ); }

	inline IStream* GetSafeStream( const COleStreamFile* pFile ) { return pFile != NULL ? pFile->m_lpStream : NULL; }
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
