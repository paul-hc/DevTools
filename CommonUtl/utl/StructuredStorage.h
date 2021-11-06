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
	struct CStreamState;
	struct CStreamLocation;

	typedef bool ( *TElementPred )( const fs::TEmbeddedPath& elementPath, const STATSTG& stgStat );


	// Structured storage corresponding to a compound document file.
	// Able to handle long filenames for sub-storages (sub directories) and stream (file) methods.
	// All methods that access storages and streams work in the context of the current working directory (current storage trail)
	//
	// MSDN: at least STGM_SHARE_EXCLUSIVE/CFile::shareExclusive is mandatory when opening streams/files.

	class CStructuredStorage : public CErrorHandler
							 , private utl::noncopyable
	{
	public:
		CStructuredStorage( void );
		virtual ~CStructuredStorage();

		enum { MaxFilenameLen = 31 };			// limitation imposed by the COM standard implementation of a compound document storage

		bool UseStreamSharing( void ) const { return ::HasFlag( m_stgFlags, ReadingStreamSharing ); }
		void SetUseStreamSharing( bool useStreamSharing = true );

		bool UseFlatStreamNames( void ) const { return ::HasFlag( m_stgFlags, FlatStreamNames ); }
		void SetUseFlatStreamNames( bool useFlatStreamNames = true ) { ::SetFlag( m_stgFlags, FlatStreamNames, useFlatStreamNames ); }

		// document storage file
		bool CreateDocFile( const fs::TStgDocPath& docFilePath, DWORD mode = STGM_CREATE | STGM_READWRITE );
		bool OpenDocFile( const fs::TStgDocPath& docFilePath, DWORD mode = STGM_READ );
		virtual void CloseDocFile( void );			// (!) if derived classes, call this in derived destructor so that virtual delivers the goods

		bool IsOpen( void ) const { return m_pRootStorage != NULL; }
		bool IsOpenForReading( void ) const { return IsOpen() && IsReadingMode( GetOpenMode() ); }
		bool IsOpenForWriting( void ) const { return IsOpen() && IsWritingMode( GetOpenMode() ); }
		DWORD GetOpenMode( void ) const { return m_openMode; }
		const fs::TStgDocPath& GetDocFilePath( void ) const { return m_docFilePath; }

		IStorage* GetRootStorage( void ) const { return m_pRootStorage; }

		// CWD storage: current working directory
		class CStorageTrail;

		CStorageTrail* RefCwd( void ) { return &m_cwdTrail; }

		IStorage* GetCurrentDir( void ) const { ASSERT_PTR( IsOpen() ); return m_cwdTrail.GetCurrent(); }
		const fs::TEmbeddedPath& GetCurrentDirPath( void ) const { return m_cwdTrail.GetCurrentPath(); }
		bool IsRootCurrentDir( void ) const { return m_pRootStorage == GetCurrentDir(); }

		CStructuredStorage& ResetToRootCurrentDir( void ) { ASSERT_PTR( IsOpen() ); m_cwdTrail.Reset(); return *this; }
		bool ChangeCurrentDir( const TCHAR* pDirSubPath, DWORD mode = STGM_READ );		// use STGM_READWRITE for writing (STGM_WRITE seems to be failing)
		bool MakeDirPath( const TCHAR* pDirSubPath, bool enterCurrent, DWORD mode = STGM_CREATE | STGM_READWRITE );

		// enumeration and search
		bool EnumElements( fs::IEnumerator* pEnumerator, RecursionDepth depth = Shallow );		// enumerate storages and streams in the current trail (embedded storage paths, relative from the root)
		bool FindFirstElementThat( fs::TEmbeddedPath& rFoundElementPath, TElementPred pElementPred, RecursionDepth depth = Shallow );		// find the first stream that satisfies the predicate

		// embedded storages (sub-directories) in current storage trail
		CComPtr<IStorage> CreateDir( const TCHAR* pDirName, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr<IStorage> OpenDir( const TCHAR* pDirName, DWORD mode = STGM_READ );		// for writing use STGM_READWRITE (!)
		bool DeleteDir( const TCHAR* pDirName );			// storage
		bool StorageExist( const TCHAR* pStorageName );

		// embedded streams (files) in current storage trail
		CComPtr<IStream> CreateStream( const TCHAR* pStreamName, DWORD mode = STGM_CREATE | STGM_READWRITE );
		CComPtr<IStream> OpenStream( const TCHAR* pStreamName, DWORD mode = STGM_READ );
		bool DeleteStream( const TCHAR* pStreamName );
		bool CloseStream( const TCHAR* pStreamName );		// logical closing, to unload all references to the cached stream and stream clones

		bool StreamExist( const TCHAR* pStreamSubPath );

		std::auto_ptr<fs::CStreamLocation> LocateReadStream( const fs::TEmbeddedPath& streamEmbeddedPath, DWORD mode = STGM_READ );		// try in root storage for flat representation, or deep sub-dir otherwise
		std::auto_ptr<fs::CStreamLocation> LocateWriteStream( const fs::TEmbeddedPath& streamEmbeddedPath, DWORD mode = STGM_CREATE | STGM_READWRITE );	// write or create stream in root or embedded folder path, depending on flat representation mode

		// streams states opened for reading
		bool IsStreamOpen( const fs::TEmbeddedPath& streamPath ) const { return m_openedStreamStates.Contains( streamPath ); }
		const fs::CStreamState* FindOpenedStream( const fs::TEmbeddedPath& streamPath ) const { return m_openedStreamStates.Find( streamPath ); }

		// embedded files (on streams) in current storage trail
		std::auto_ptr<COleStreamFile> CreateStreamFile( const TCHAR* pStreamName, DWORD mode = CFile::modeCreate | CFile::modeWrite );
		std::auto_ptr<COleStreamFile> OpenStreamFile( const TCHAR* pStreamName, DWORD mode = CFile::modeRead );

		std::auto_ptr<COleStreamFile> MakeOleStreamFile( const TCHAR* pStreamName, IStream* pStream = NULL ) const;

		// backwards compatibility: find existing object based on possible alternates (in current storage trail)
		std::pair< const TCHAR*, size_t > FindAlternate_DirName( const TCHAR* altDirNames[], size_t altCount );
		std::pair< const TCHAR*, size_t > FindAlternate_StreamName( const TCHAR* altStreamNames[], size_t altCount );

		static std::tstring MakeShortFilename( const TCHAR* pElementName );		// make short file name with length limited to MaxFilenameLen
		static DWORD ToMode( DWORD mode );										// augment mode with default STGM_SHARE_EXCLUSIVE (if no STGM_SHARE_DENY_* flag is present)
		static bool IsReadingMode( DWORD mode ) { return !IsWritingMode( mode ); }
		static bool IsWritingMode( DWORD mode ) { return HasFlag( mode, WriteableModeMask ); }

		enum { WriteableModeMask = STGM_CREATE | STGM_WRITE | STGM_READWRITE };

		static CStructuredStorage* FindOpenedStorage( const fs::TStgDocPath& docStgPath );
	protected:
		// overridables
		virtual std::tstring EncodeStreamName( const TCHAR* pStreamName ) const;
		virtual TCHAR GetFlattenPathSep( void ) const;
		virtual bool RetainOpenedStream( const fs::TEmbeddedPath& streamPath ) const;		// if true: cache opened streams to allow later shared access (workaround sharing violations caused by STGM_SHARE_EXCLUSIVE)

		bool HandleError( HRESULT hResult, const TCHAR* pElementName, const TCHAR* pDocFilePath = NULL ) const;
		fs::CFlexPath MakeElementFlexPath( const TCHAR* pDocFilePath, const TCHAR* pElementName ) const;
	private:
		bool CacheStreamState( const fs::TEmbeddedPath& streamPath, IStream* pStreamOrigin );
		size_t CloseStreamsWithPrefix( const TCHAR* pSubDirPrefix ) { return m_openedStreamStates.RemoveWithPrefix( pSubDirPrefix ); }

		CComPtr<IStream> CloneStream( const fs::CStreamState* pStreamState, const fs::TEmbeddedPath& streamPath );		// for stream logical re-opening

		fs::TEmbeddedPath MakeElementSubPath( const std::tstring& encodedElementName ) const { return GetCurrentDirPath() / encodedElementName; }

		enum StorageFlags
		{
			ReadingStreamSharing	= BIT_FLAG( 0 ),		// keeps opened streams alive via caching, allowing stream cloning to circumvent exclusive sharing violations
			FlatStreamNames			= BIT_FLAG( 1 )			// store encoded stream deep paths by replacing '\' with '|'
		};

		typedef int TStorageFlags;
	private:
		typedef fs::CPathObjectMap< fs::TStgDocPath, CStructuredStorage > TStorageMap;		// document path to open root storage

		static TStorageMap& GetOpenedDocStgs( void );			// singleton: opened document storages - only root storages!

		static void RegisterDocStg( CStructuredStorage* pDocStg );
		static void UnregisterDocStg( CStructuredStorage* pDocStg );
	public:
		// owning stack of deep opened storages starting from the root, of which the current (deepest) storage is at the back.
		class CStorageTrail
		{
		public:
			CStorageTrail( CStructuredStorage* pDocStorage ) : m_pDocStorage( pDocStorage ) { ASSERT_PTR( m_pDocStorage ); }
			~CStorageTrail() { Reset(); }

			bool IsEmpty( void ) const { return m_openSubStorages.empty(); }
			IStorage* GetRoot( void ) const { return m_pDocStorage->GetRootStorage(); }

			void Reset( const CStorageTrail* pTrail = NULL );
			void Push( IStorage* pSubStorage );				// go to sub-storage
			CComPtr<IStorage> Pop( void );				// go to parent storage

			IStorage* GetCurrent( void ) const { return !IsEmpty() ? m_openSubStorages.back() : GetRoot(); }
			const fs::TEmbeddedPath& GetCurrentPath( void ) const { return m_trailPath; }

			size_t GetDepth( void ) const { return m_openSubStorages.size(); }
			IStorage* GetStorageAtLevel( size_t depthLevel ) const { ASSERT( depthLevel < GetDepth() ); return m_openSubStorages[ depthLevel ]; }
		private:
			CStructuredStorage* m_pDocStorage;
			std::vector< CComPtr<IStorage> > m_openSubStorages;		// opened embedded storages: sub-directory path to the current (deepest) storage
			fs::TEmbeddedPath m_trailPath;
		};


		class CScopedCurrentDir
		{
		public:
			CScopedCurrentDir( CStructuredStorage* pDocStorage, const TCHAR* pDirSubPath = s_rootFolderName, DWORD mode = STGM_READ, bool fromRoot = true );
			CScopedCurrentDir( CStructuredStorage* pDocStorage, IStorage* pSubStorage, bool fromRoot = true );
			~CScopedCurrentDir();

			bool IsValid( void ) const { return m_validDirPath; }
		public:
			CStructuredStorage* m_pDocStorage;
		private:
			CStorageTrail m_origCwdTrail;		// to be resored on destruction
			bool m_validDirPath;
		};
	private:
		CComPtr<IStorage> m_pRootStorage;		// root storage in a compound document file
		CStorageTrail m_cwdTrail;				// current working directory: trail of currently opened storages (initially empty: pointing to the root storage)
		TStorageFlags m_stgFlags;
		DWORD m_openMode;
		fs::TStgDocPath m_docFilePath;				// full path of the document storage file

		typedef fs::CPathMap< fs::TEmbeddedPath, fs::CStreamState > TStreamStates;		// keys are embedded encoded paths, whereas CFileState::m_fullPath is the storage name

		TStreamStates m_openedStreamStates;		// stream states of streams opened for reading (immutable, clonable)
	protected:
		static mfc::CAutoException< CFileException > s_fileError;
	public:
		static const TCHAR s_rootFolderName[];	// ""
	};
}


#include "FileState.h"


namespace fs
{
	// stream utilities

	bool SeekToPos( IStream* pStream, UINT64 position, DWORD origin = STREAM_SEEK_SET );
	inline bool SeekToBegin( IStream* pStream ) { return SeekToPos( pStream, 0 ); }
	inline bool SeekToEnd( IStream* pStream ) { return SeekToPos( pStream, 0, STREAM_SEEK_END ); }

	inline IStream* GetSafeStream( const COleStreamFile* pFile ) { return pFile != NULL ? pFile->m_lpStream : NULL; }


	struct CStreamState : public fs::CFileState
	{
		CStreamState( void ) {}

		bool Build( IStream* pStreamOrigin, bool keepStreamAlive );

		bool HasStream( void ) const { return m_pStreamOrigin != NULL; }
		void ReleaseStream( void ) { m_pStreamOrigin = NULL; }
		HRESULT CloneStream( IStream** ppStreamDuplicate ) const;
	private:
		CComPtr<IStream> m_pStreamOrigin;			// for stream sharing: optional, keeps the original opened stream, that can be cloned for subsequent stream reading
	};


	// keeps an open stream valid, in the context of its folder trail location (all sub-storages starting from root)
	struct CStreamLocation : private utl::noncopyable
	{
		CStreamLocation( void ) {}

		bool IsValid( void ) const { return m_pStream != NULL; }

		fs::CStructuredStorage::CStorageTrail* GetLocation( void ) const { return GetDocStorage()->RefCwd(); }
		fs::CStructuredStorage* GetDocStorage( void ) const { ASSERT_PTR( m_pCurrDir.get() ); return m_pCurrDir->m_pDocStorage; }
	public:
		std::auto_ptr<fs::CStructuredStorage::CScopedCurrentDir> m_pCurrDir;		// temporary CWD
		CComPtr<IStream> m_pStream;
	};


	// managed stream file: auto-close on destructor
	class CManagedOleStreamFile : public COleStreamFile
	{
	public:
		CManagedOleStreamFile( IStream* pStreamOpened, const fs::CFlexPath& streamFullPath );
		CManagedOleStreamFile( std::auto_ptr<fs::CStreamLocation> pStreamLocation, const fs::CFlexPath& streamFullPath );
		virtual ~CManagedOleStreamFile();
	private:
		void Init( IStream* pStreamOpened, const fs::CFlexPath& streamFullPath );
	private:
		std::auto_ptr<fs::CStreamLocation> m_pStreamLocation;
	};
}


namespace fs
{
	namespace stg
	{
		// switches to Throw Mode, and ensures the compound document file gets closed when exiting the scope (incl. exceptions are thrown) - useful when CREATING document
		class CScopedCreateDocMode : public CScopedErrorHandling
		{
		public:
			CScopedCreateDocMode( CStructuredStorage* pDocStorage, const fs::TStgDocPath* pDocFilePath, const CErrorHandler* pSrcHandler = CErrorHandler::Thrower() );
			~CScopedCreateDocMode();
		protected:
			CStructuredStorage* m_pDocStorage;
			fs::TStgDocPath m_docFilePath;
		};


		// switches to Throw Mode, and restores document in the previous reading mode if previously opened - useful when WRITING document
		class CScopedWriteDocMode : public CScopedCreateDocMode
		{
		public:
			CScopedWriteDocMode( CStructuredStorage* pDocStorage, const fs::TStgDocPath* pDocFilePath, DWORD writeMode = STGM_READWRITE, const CErrorHandler* pSrcHandler = CErrorHandler::Thrower() );
			~CScopedWriteDocMode();
		private:
			DWORD m_origReadingMode;

			static const DWORD s_closedMode = static_cast<DWORD>( -1 );
		};


		// uses storage mirroring on Save (storage to itself), or no mirroring on SaveAs
		class CMirrorStorageSave
		{
		public:
			CMirrorStorageSave( const fs::TStgDocPath& docStgPath, const fs::TStgDocPath& oldDocStgPath );
			~CMirrorStorageSave() { Rollback(); }

			bool UseMirroring( void ) const { return !m_mirrorDocStgPath.IsEmpty(); }
			const fs::TStgDocPath& GetDocStgPath( void ) const { return UseMirroring() ? m_mirrorDocStgPath : m_docStgPath; }

			void Commit( void ) throws_( CFileException* );		// call at the end of a successful transaction (may throw)
			void Rollback( void );								// delete the temporary mirror file in case of errors
		protected:
			virtual bool CloseStorage( void );
		protected:
			const fs::TStgDocPath& m_docStgPath;
		private:
			fs::TStgDocPath m_mirrorDocStgPath;
		};
	}
}


namespace fs
{
	namespace flex
	{
		// API for file streams and embedded file streams
		CComPtr<IStream> OpenStreamOnFile( const fs::CFlexPath& filePath, DWORD mode = STGM_READ, DWORD createAttributes = 0 );

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
			ASSERT( is_a<IStream>( pInterface ) || is_a<ILockBytes>( pInterface ) );		// it reports 0 size on sub-storages
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
