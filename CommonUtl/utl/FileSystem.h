#ifndef FileSystem_h
#define FileSystem_h
#pragma once

#include "FileSystem_fwd.h"


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath );

	bool IsValidDirectory( const TCHAR* pDirPath );
	bool IsValidEmptyDirectory( const TCHAR* pDirPath );

	bool IsValidShellLink( const TCHAR* pFilePath );
	bool IsValidStructuredStorage( const TCHAR* pDocFilePath );			// a compound document file that exists?

	bool IsReadOnlyFile( const TCHAR* pFilePath );
	bool IsProtectedFile( const TCHAR* pFilePath );

	void QueryFolderPaths( OUT std::vector<fs::TDirPath>& rFolderPaths, const std::vector<fs::CPath>& filePaths, bool uniqueOnly = true );

	fs::TDirPath GetCurrentDirectory( void );				// current working directory
	fs::CPath GetModuleFilePath( HINSTANCE hInstance );		// pass AfxGetApp()->m_hInstance in GUI apps, or NULL for the .exe
	fs::TDirPath GetTempDirPath( void );


	fs::CPath MakeAbsoluteToCWD( const TCHAR* pRelativePath );							// normalize, strip "./", "../", relative to CWD - current working directory
	fs::CPath MakeRelativeTo( const TCHAR* pFromPath, const fs::TDirPath& dirPath );	// normalize, make relative to dirPath (such as "..\\.." etc)

	inline fs::CPath& CvtAbsoluteToCWD( OUT fs::CPath& rPath ) { return rPath = MakeAbsoluteToCWD( rPath.GetPtr() ); }
	inline fs::CPath& CvtRelativeTo( OUT fs::CPath& rPath, const fs::TDirPath& dirPath ) { return rPath = MakeRelativeTo( rPath.GetPtr(), dirPath ); }


	fs::AcquireResult MakeFileWritable( const TCHAR* pFilePath );		// make file writable if read-only
	bool DeleteFile( const TCHAR* pFilePath );							// delete even if read-only file

	bool CreateDir( const TCHAR* pDirPath );
	bool CreateDirPath( const TCHAR* pDirPath );						// creates deep directory path, returns true if a valid directory path
	bool DeleteDir( const TCHAR* pDirPath, RecursionDepth depth = Deep );
	bool DeleteAllFiles( const TCHAR* pDirPath );						// delete even if read-only sub-directories or embedded files


	bool LocateExistingFile( OUT fs::CPath* pFoundPath, const std::vector<fs::TDirPath>& searchPaths, const TCHAR* pFilenames, const TCHAR* pSep = _T("|") );


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


	// MFC CFile helpers

	std::auto_ptr<CFile> OpenFile( const fs::CPath& filePath, bool throwMode = false, DWORD mode = CFile::modeRead | CFile::typeBinary ) throws_( CFileException* );

	UINT64 BufferedCopy( OUT CFile& rDestFile, CFile& srcFile, size_t chunkSize = 4 * KiloByte );		// 4096 works well because it's quicker to allocate


	class CTextFileWriter
	{
	public:
		CTextFileWriter( OUT CFile* pDestFile, bool isBinaryFile = true );

		void WriteText( const std::tstring& text );
		void WriteLine( const std::tstring& text ) { WriteText( text + m_lineEnd ); }
		void WriteEmptyLine( void ) { WriteText( m_lineEnd ); }

		void SeekToEnd( void ) { m_pDestFile->SeekToEnd(); }		// for appending
	public:
		CFile* m_pDestFile;
		const std::tstring m_lineEnd;
	};
}


namespace fs
{
	const WIN32_FIND_DATA* GetFindData( const CFileFind& foundFile );
	inline DWORD GetFileAttributes( const CFileFind& foundFile ) { return GetFindData( foundFile )->dwFileAttributes; }


	bool ModifyFileAttributes( const fs::CPath& filePath, DWORD removeAttributes, DWORD addAttributes );
	inline bool AddFileAttributes( const fs::CPath& filePath, DWORD addAttributes ) { return ModifyFileAttributes( filePath, 0, addAttributes ); }
	inline bool RemoveFileAttributes( const fs::CPath& filePath, DWORD removeAttributes ) { return ModifyFileAttributes( filePath, removeAttributes, 0 ); }


	UINT64 GetFileSize( const TCHAR* pFilePath );

	CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField );
	inline CTime ReadLastModifyTime( const fs::CPath& filePath ) { return ReadFileTime( filePath, ModifiedDate ); }


	FileExpireStatus CheckExpireStatus( const fs::CPath& filePath, const CTime& lastModifyTime );


	FILETIME* MakeFileTime( OUT FILETIME& rOutFileTime, const CTime& time );


	namespace thr
	{
		CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField,
							fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void WriteFileTime( const TCHAR* pFilePath, TimeField timeField, const CTime& time,
							fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		FILETIME* MakeFileTime( OUT FILETIME& rOutFileTime, const CTime& time, const TCHAR* pFilePath,
								fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void TouchFile( const fs::CPath& filePath, const CTime& time = CTime::GetCurrentTime(), TimeField timeField = ModifiedDate,
						fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );

		void TouchFileBy( const fs::CPath& filePath, const CTimeSpan& bySpan, TimeField timeField = ModifiedDate,
						  fs::ExcPolicy policy = fs::RuntimeExc ) throws_( CRuntimeException, CFileException* );
	}
}


namespace fs
{
	namespace impl
	{
		CFileException* NewLastErrorException( const TCHAR* pFilePath, LONG lastError = ::GetLastError() );
		CFileException* NewErrnoException( const TCHAR* pFilePath, unsigned long errNo );	// pass _doserrno or the DOS function result

		void __declspec(noreturn) ThrowMfcErrorAs( ExcPolicy policy, CFileException* pExc ) throws_( CRuntimeException, CFileException* );
		void __declspec(noreturn) ThrowFileException( const std::tstring& description, const TCHAR* pFilePath, const TCHAR sep[] = _T(": ") ) throws_( CRuntimeException );
	}


	template< typename PathType >		// PathType could be any of: const char*, const TCHAR*, std::tstring, fs::CPath
	void __declspec(noreturn) ThrowFileException( const std::tstring& description, const PathType& filePath,
		fs::ExcPolicy policy = fs::RuntimeExc )
		throws_( CRuntimeException, CFileException* )
	{
		fs::RuntimeExc == policy
			? impl::ThrowFileException( description, str::ValueToString<std::tstring>( filePath ).c_str() )
			: impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( str::traits::GetCharPtr( filePath ), ERROR_INVALID_ACCESS ) );
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


	struct ComparePathRank : public BaseComparator
	{
		CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			return Compare_Scalar( GetPathRank( leftPath ), GetPathRank( rightPath ) );
		}

		static int GetPathRank( const fs::CPath& filePath )
		{
			return fs::IsValidDirectory( filePath.GetPtr() ) ? 0 : 1;
		}
	};

	typedef JoinCompare<ComparePathRank, CompareNaturalPath> TComparePathDirsFirst;
}


namespace fs
{
	template< typename PathContainerT >
	inline typename PathContainerT::iterator GroupDirectoriesFirst( PathContainerT& rPaths )
	{
		return std::stable_partition( rPaths.begin(), rPaths.end(), pred::IsValidDirectory() );
	}

	template< typename PathContainerT >
	void SortPathsDirsFirst( PathContainerT& rPaths, bool ascending /*= true*/ )
	{
		typename PathContainerT::iterator itFirstFile = GroupDirectoriesFirst( rPaths );

		fs::SortPaths( rPaths.begin(), itFirstFile, ascending );		// directories come first
		fs::SortPaths( itFirstFile, rPaths.end(), ascending );
	}
}


#endif // FileSystem_h
