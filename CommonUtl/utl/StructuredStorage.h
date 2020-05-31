#ifndef StructuredStorage_h
#define StructuredStorage_h
#pragma once

#include "FlexPath.h"
#include "FileSystem.h"
#include "PathObjectMap.h"
#include "ThrowMode.h"
#include "RuntimeException.h"
#include <hash_map>
#include <afxole.h>


namespace fs
{
	// Structured storage corresponding to a compound document file.
	// Able to handle long filenames for sub-storages (sub directories) and file methods.
	//
	// MSDN: at least STGM_SHARE_EXCLUSIVE/CFile::shareExclusive is mandatory when opening streams/files.

	class CStructuredStorage : public CThrowMode, private utl::noncopyable
	{
	public:
		enum { MaxFilenameLen = 31 };

		CStructuredStorage( IStorage* pRootStorage = NULL );
		virtual ~CStructuredStorage();

		static DWORD ToMode( DWORD mode );										// augment mode with default STGM_SHARE_EXCLUSIVE
		static std::tstring MakeShortFilename( const TCHAR* pFilename );		// make short file name with length limited to MaxFilenameLen

		IStorage* GetStorage( void ) const { return m_pRootStorage; }
		bool IsEmbeddedStorage( void ) const { return m_isEmbeddedStg; }
		bool IsRootStorage( void ) const { return GetStorage() != NULL && !IsEmbeddedStorage(); }

		// document storage file
		DWORD GetOpenMode( void ) const { return m_openMode; }
		const fs::CPath& GetDocFilePath( void ) const;

		bool IsOpen( void ) const { return m_pRootStorage != NULL; }
		bool Create( const TCHAR* pStgFilePath, DWORD mode = STGM_CREATE | STGM_WRITE );
		bool Open( const TCHAR* pStgFilePath, DWORD mode = STGM_READ );
		void Close( void );

		// embedded storages (sub-directories)
		CComPtr< IStorage > CreateDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_WRITE );
		CComPtr< IStorage > OpenDir( const TCHAR* pDirName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		CComPtr< IStorage > OpenDeepDir( const TCHAR* pDirSubPath, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteDir( const TCHAR* pDirName, IStorage* pParentDir = NULL );			// storage

		// embedded streams (files)
		CComPtr< IStream > CreateStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_CREATE | STGM_WRITE );
		CComPtr< IStream > OpenStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		CComPtr< IStream > OpenDeepStream( const TCHAR* pStreamSubPath, IStorage* pParentDir = NULL, DWORD mode = STGM_READ );
		bool DeleteStream( const TCHAR* pStreamName, IStorage* pParentDir = NULL );

		// embedded files (streams); caller must delete the returned file
		COleStreamFile* CreateFile( const TCHAR* pFileName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeCreate | CFile::modeWrite );
		COleStreamFile* OpenFile( const TCHAR* pFileName, IStorage* pParentDir = NULL, DWORD mode = CFile::modeRead );
		COleStreamFile* OpenDeepFile( const TCHAR* pFileSubPath, IStorage* pParentDir = NULL, DWORD mode = CFile::modeRead );

		bool StorageExist( const TCHAR* pStorageName, IStorage* pParentDir = NULL );
		bool StreamExist( const TCHAR* pStreamSubPath, IStorage* pParentDir = NULL );

		static CStructuredStorage* FindOpenedStorage( const fs::CPath& docStgPath );
	private:
		typedef fs::CPathObjectMap< fs::CPath, CStructuredStorage > TStorageMap;		// document path to open root storage

		static TStorageMap& GetOpenedDocStgs( void );			// singleton: opened document storages - only root storages!

		static void RegisterDocStg( CStructuredStorage* pDocStg );
		static void UnregisterDocStg( CStructuredStorage* pDocStg );
	protected:
		virtual std::tstring EncodeStreamName( const TCHAR* pStreamName ) const;
		COleStreamFile* NewOleStreamFile( const TCHAR* pStreamName, IStream* pStream = NULL ) const;

		bool HandleError( HRESULT hResult, const TCHAR* pStgFilePath = NULL ) const;
		COleStreamFile* HandleStreamError( const TCHAR* pStreamName ) const;		// may throw s_fileError
	private:
		CComPtr< IStorage > m_pRootStorage;			// root directory
		const bool m_isEmbeddedStg;					// true for storages passed on constructor (not opened internally)
		DWORD m_openMode;
		mutable fs::CPath m_docFilePath;			// self-encapsulated
	protected:
		static mfc::CAutoException< CFileException > s_fileError;
	};


	class CAutoOleStreamFile : public COleStreamFile		// auto-close on destructor
	{
	public:
		CAutoOleStreamFile( IStream* pStream = NULL )
			: COleStreamFile( pStream )
		{
			if ( pStream != NULL )
				pStream->AddRef();				// keep the stream alive when passed on contructor (e.g. OpenStream)

			m_bCloseOnDelete = TRUE;			// will close on destructor
		}
	};


	class CAcquireStorage : private utl::noncopyable
	{
	public:
		CAcquireStorage( const fs::CPath& docStgPath, DWORD mode = STGM_READ )
			: m_pFoundOpenStg( CStructuredStorage::FindOpenedStorage( docStgPath ) )
		{
			if ( NULL == m_pFoundOpenStg )
				m_tempStorage.Open( docStgPath.GetPtr(), mode );
		}

		~CAcquireStorage()
		{
		}

		CStructuredStorage* Get( void )
		{
			if ( m_pFoundOpenStg != NULL )
				return m_pFoundOpenStg;

			return m_tempStorage.IsOpen() ? &m_tempStorage : NULL;
		}
	private:
		CStructuredStorage* m_pFoundOpenStg;
		fs::CStructuredStorage m_tempStorage;
	};

} //namespace fs


namespace fs
{
	// API for file streams and embedded file streams

	CComPtr< IStream > OpenStreamOnFile( const fs::CFlexPath& filePath, DWORD mode = STGM_READ, DWORD createAttributes = 0 );


	namespace flex
	{
		CTime ReadLastModifyTime( const fs::CFlexPath& filePath );			// for embedded images use the stg doc time
		FileExpireStatus CheckExpireStatus( const fs::CFlexPath& filePath, const CTime& lastModifyTime );
	}
}


namespace fs
{
	bool MakeFileStatus( CFileStatus& rStatus, const STATSTG& statStg );

	// functions that work uniformly for IStorage/IStream/ILockBytes

	template< typename InterfaceType >
	bool GetElementStatus( CFileStatus& rStatus, InterfaceType* pInterface, STATFLAG flag = STATFLAG_NONAME )		// works for IStorage/IStream/ILockBytes
	{
		ASSERT_PTR( pInterface );
		STATSTG statStg;
		return
			HR_OK( pInterface->Stat( &statStg, flag ) ) &&
			MakeFileStatus( rStatus, statStg );
	}

	template< typename InterfaceType >
	inline std::tstring GetElementName( InterfaceType* pInterface )
	{
		CFileStatus status;
		fs::GetElementStatus( status, pInterface, STATFLAG_DEFAULT );
		return status.m_szFullName;
	}

	template< typename InterfaceType >
	inline CTime GetLastModifiedTime( InterfaceType* pInterface )
	{
		CFileStatus status;
		fs::GetElementStatus( status, pInterface, STATFLAG_DEFAULT );
		return status.m_mtime;
	}
}


#endif // StructuredStorage_h
