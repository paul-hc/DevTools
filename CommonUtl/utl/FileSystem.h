#ifndef FileSystem_h
#define FileSystem_h
#pragma once

#include "FileSystem_fwd.h"


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath );
	bool IsValidDirectory( const TCHAR* pDirPath );

	bool IsReadOnlyFile( const TCHAR* pFilePath );
	bool IsProtectedFile( const TCHAR* pFilePath );

	fs::CPath MakeAbsoluteToCWD( const TCHAR* pRelativePath );						// normalize, strip "./", "../", relative to CWD - current working directory
	fs::CPath MakeRelativeTo( const TCHAR* pFromPath, const fs::CPath& dirPath );	// normalize, make relative to dirPath (such as "..\\.." etc)

	inline fs::CPath& CvtAbsoluteToCWD( fs::CPath& rPath ) { return rPath = MakeAbsoluteToCWD( rPath.GetPtr() ); }
	inline fs::CPath& CvtRelativeTo( fs::CPath& rPath, const fs::CPath& dirPath ) { return rPath = MakeRelativeTo( rPath.GetPtr(), dirPath ); }

	std::tstring GetTempDirPath( void );
	std::tstring MakeTempDirPath( const TCHAR* pSubPath );


	fs::AcquireResult MakeFileWritable( const TCHAR* pFilePath );		// make file writable if read-only
	bool DeleteFile( const TCHAR* pFilePath );							// delete even if read-only file

	bool CreateDir( const TCHAR* pDirPath );
	bool CreateDirPath( const TCHAR* pDirPath );						// creates deep directory path, returns true if a valid directory path
	bool DeleteDir( const TCHAR* pDirPath );
	bool DeleteAllFiles( const TCHAR* pDirPath );						// delete even if read-only sub-directories or embedded files


	enum ExcPolicy { RuntimeExc, MfcExc };

	namespace thr
	{
		// functions that throw on error

		void MakeFileWritable( const TCHAR* pFilePath ) throws_( CRuntimeException );
		void CopyFile( const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath, bool failIfExists ) throws_( CRuntimeException );
		void MoveFile( const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath, DWORD flags = MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) throws_( CRuntimeException );
		void DeleteFile( const TCHAR* pFilePath ) throws_( CRuntimeException );		// delete even if read-only file

		void CreateDirPath( const TCHAR* pDirPath,
							fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void ThrowFileOpLastError( DWORD lastError, const TCHAR operationTag[], const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath ) throws_( CRuntimeException );
	}


	UINT64 BufferedCopy( CFile& rDestFile, CFile& srcFile, size_t chunkSize = 4 * KiloByte );		// 4096 works well because it's quicker to allocate
}


namespace fs
{
	const WIN32_FIND_DATA* GetFindData( const CFileFind& foundFile );
	inline DWORD GetFileAttributes( const CFileFind& foundFile ) { return GetFindData( foundFile )->dwFileAttributes; }

	UINT64 GetFileSize( const TCHAR* pFilePath );

	CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField );
	inline CTime ReadLastModifyTime( const fs::CPath& filePath ) { return ReadFileTime( filePath, ModifiedDate ); }

	enum FileExpireStatus { FileNotExpired, ExpiredFileModified, ExpiredFileDeleted };
	const CEnumTags& GetTags_FileExpireStatus( void );

	FileExpireStatus CheckExpireStatus( const fs::CPath& filePath, const CTime& lastModifyTime );


	namespace thr
	{
		CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField,
							fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void WriteFileTime( const TCHAR* pFilePath, TimeField timeField, const CTime& time,
							fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		FILETIME* MakeFileTime( FILETIME& rOutFileTime, const CTime& time, const TCHAR* pFilePath,
								fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void TouchFile( const fs::CPath& filePath, const CTime& time = CTime::GetCurrentTime(), TimeField timeField = ModifiedDate,
						fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void TouchFileBy( const fs::CPath& filePath, const CTimeSpan& bySpan, TimeField timeField = ModifiedDate,
						  fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );
	}
}


namespace fs
{
	template< typename PathType >		// PathType could be any of: const char*, const TCHAR*, std::tstring, fs::CPath
	void __declspec( noreturn ) ThrowFileException( const std::tstring& description, const PathType& filePath,
													fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* )
	{
		fs::RuntimeExc == policy
			? impl::ThrowFileException( description, str::ValueToString< std::tstring >( filePath ).c_str() )
			: impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( str::traits::GetCharPtr( filePath ), ERROR_INVALID_ACCESS ) );
	}

	namespace impl
	{
		CFileException* NewLastErrorException( const TCHAR* pFilePath, LONG lastError = ::GetLastError() );
		CFileException* NewErrnoException( const TCHAR* pFilePath, unsigned long errNo );	// pass _doserrno or the DOS function result

		void __declspec( noreturn ) ThrowMfcErrorAs( ExcPolicy policy, CFileException* pExc ) throws_( CRuntimeException, CFileException* );
		void __declspec( noreturn ) ThrowFileException( const std::tstring& description, const TCHAR* pFilePath, const TCHAR sep[] = _T(": ") ) throws_( CRuntimeException );
	}
}


namespace fs
{
	class CScopedWriteableFile
	{
	public:
		CScopedWriteableFile( const TCHAR* pFilePath );
		~CScopedWriteableFile();
	private:
		const TCHAR* m_pFilePath;
		DWORD m_origAttr;
	};

	class CScopedFileTime
	{
	public:
		CScopedFileTime( const fs::CPath& filePath, TimeField timeField = fs::ModifiedDate );
		~CScopedFileTime();
	private:
		fs::CPath m_filePath;
		TimeField m_timeField;
		CTime m_origFileTime;
	};
}


namespace pred
{
	struct IsValidFile
	{
		bool operator()( const TCHAR* pFilePath ) const { return fs::IsValidFile( pFilePath ); }
		bool operator()( const std::tstring& filePath ) const { return fs::IsValidFile( filePath.c_str() ); }
		bool operator()( const fs::CPath& filePath ) const { return fs::IsValidFile( filePath.GetPtr() ); }
	};

	struct IsValidDirectory
	{
		bool operator()( const TCHAR* pFilePath ) const { return fs::IsValidDirectory( pFilePath ); }
		bool operator()( const std::tstring& filePath ) const { return fs::IsValidDirectory( filePath.c_str() ); }
		bool operator()( const fs::CPath& filePath ) const { return fs::IsValidDirectory( filePath.GetPtr() ); }
	};
}


namespace fs
{
	template< typename PathContainerT >
	inline typename PathContainerT::iterator GroupDirectoriesFirst( PathContainerT& rPaths )
	{
		return std::stable_partition( rPaths.begin(), rPaths.end(), pred::IsValidDirectory() );
	}

	template< typename PathContainerT >
	void SortDirectoriesFirst( PathContainerT& rPaths, bool ascending = true )
	{
		typename PathContainerT::iterator itFirstFile = GroupDirectoriesFirst( rPaths );

		fs::SortPaths( rPaths.begin(), itFirstFile, ascending );		// directories come first
		fs::SortPaths( itFirstFile, rPaths.end(), ascending );
	}
}


namespace fs
{
	// files and sub-dirs as std::tstring; relative to m_refDirPath if specified

	struct CEnumerator : public IEnumerator, private utl::noncopyable
	{
		CEnumerator( const std::tstring refDirPath = std::tstring() ) : m_pChainEnum( NULL ), m_refDirPath( refDirPath ) {}
		CEnumerator( IEnumerator* pChainEnum ) : m_pChainEnum( pChainEnum ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		std::tstring m_refDirPath;					// to remove if common prefix
	public:
		std::vector< std::tstring > m_filePaths;
		std::vector< std::tstring > m_subDirPaths;
	};


	// files and sub-dirs as fs::CPath, sorted intuitively; relative to m_refDirPath if specified

	struct CPathEnumerator : public IEnumerator
	{
		CPathEnumerator( const std::tstring refDirPath = std::tstring() ) : m_pChainEnum( NULL ), m_refDirPath( refDirPath ) {}
		CPathEnumerator( IEnumerator* pChainEnum ) : m_pChainEnum( pChainEnum ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		std::tstring m_refDirPath;					// to remove if common prefix
	public:
		fs::TPathSet m_filePaths;					// sorted intuitively
		fs::TPathSet m_subDirPaths;
	};


	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	void EnumFiles( std::vector< std::tstring >& rFilePaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
	void EnumSubDirs( std::vector< std::tstring >& rSubDirPaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	fs::CPath FindFirstFile( const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
}


namespace fs
{
	// generate a path of a unique filename using a suffix, by avoiding collisions with existing files
	std::tstring MakeUniqueNumFilename( const TCHAR* pFilePath, const TCHAR fmtNumSuffix[] = _T("_[%d]") ) throws_( CRuntimeException );	// with numeric suffix
	std::tstring MakeUniqueHashedFilename( const TCHAR* pFilePath, const TCHAR fmtHashSuffix[] = _T("_%08X") );								// with hash suffix
}


#endif // FileSystem_h
