#ifndef Path_h
#define Path_h
#pragma once

#include <io.h>
#include <map>
#include <set>
#include <xhash>
#include "StringUtilities.h"


namespace path
{
	inline bool IsSlash( TCHAR ch ) { return _T('\\') == ch || _T('/') == ch; }
	inline TCHAR ToNormalChar( TCHAR ch ) { return _T('/') == ch ? _T('\\') : ch; }
	inline TCHAR ToEquivalentChar( TCHAR ch ) { return _T('/') == ch ? _T('\\') : ::towlower( ch ); }
	const TCHAR* DirDelims( void );

	bool EquivalentPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath );

	bool Equivalent( const std::tstring& leftPath, const std::tstring& rightPath );
	bool Equal( const std::tstring& leftPath, const std::tstring& rightPath );

	inline pred::CompareResult CompareNPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath, size_t count = std::tstring::npos )
	{
		return str::CompareN( pLeftPath, pRightPath, &path::ToEquivalentChar, count );
	}

	inline pred::CompareResult CompareN( const std::tstring& leftPath, const std::tstring& rightPath, size_t count = std::tstring::npos )
	{
		return CompareNPtr( leftPath.c_str(), rightPath.c_str(), count );
	}


	enum PathMatch { MatchEqual, MatchEqualDiffCase, MatchNotEqual };

	PathMatch Match( const TCHAR* pLeftPath, const TCHAR* pRightPath );

	template< typename CharType >
	PathMatch MatchChar( CharType left, CharType right )
	{
		if ( ToNormalChar( left ) == ToNormalChar( right ) )
			return MatchEqual;

		if ( ToEquivalentChar( left ) == ToEquivalentChar( right ) )
			return MatchEqualDiffCase;

		return MatchNotEqual;
	}


	const TCHAR* Wildcards( void );
	const TCHAR* MultiSpecDelims( void );

	bool MatchWildcard( const TCHAR* pPath, const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims = MultiSpecDelims() );
	bool IsMultipleWildcard( const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims = MultiSpecDelims() );
	bool ContainsWildcards( const TCHAR* pWildSpec, const TCHAR* pWildcards = Wildcards() );

	bool IsValid( const std::tstring& path );
	const TCHAR* GetInvalidChars( void );
	const TCHAR* GetReservedChars( void );

	bool IsAbsolute( const TCHAR* pPath );
	bool IsRelative( const TCHAR* pPath );
	bool IsDirectory( const TCHAR* pPath );
	bool IsNameExt( const TCHAR* pPath );

	const TCHAR* Find( const TCHAR* pPath, const TCHAR* pSubString );
	const TCHAR* FindFilename( const TCHAR* pPath );
	const TCHAR* FindExt( const TCHAR* pPath );

	inline bool IsFnameExt( const TCHAR* pPath ) { return pPath != NULL && pPath == FindFilename( pPath ); }				// true for "file.txt", "file", ".txt";  false for "dir\\file.txt"
	inline bool MatchExt( const TCHAR* pPath, const TCHAR* pExt ) { return EquivalentPtr( FindExt( pPath ), pExt ); }		// pExt: ".txt"

	// complex path
	extern const TCHAR s_complexPathSep;				// character that separates the storage file path from the stream/storage embedded sub-path

	inline bool IsComplex( const TCHAR* pPath ) { return pPath != NULL && _tcschr( pPath, s_complexPathSep ) != NULL; }
	bool IsWellFormed( const TCHAR* pFilePath );
	size_t FindComplexSepPos( const TCHAR* pPath );
	std::tstring GetPhysical( const std::tstring& filePath );	// "C:\Images\fruit.stg" <- "C:\Images\fruit.stg>apple.jpg";  "C:\Images\orange.png" <- "C:\Images\orange.png"
	const TCHAR* GetEmbedded( const TCHAR* pPath );				// "Normal\apple.jpg" <- "C:\Images\fruit.stg>Normal\apple.jpg";  "" <- "C:\Images\orange.png"
	const TCHAR* GetSubPath( const TCHAR* pPath );				// IsComplex() ? GetEmbedded() : pPath

	std::tstring MakeComplex( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath );
	bool SplitComplex( std::tstring& rPhysicalPath, std::tstring& rEmbeddedPath, const std::tstring& filePath );


	enum TrailSlash { PreserveSlash, AddSlash, RemoveSlash };

	std::tstring& SetBackslash( std::tstring& rDirPath, TrailSlash trailSlash = AddSlash );
	inline std::tstring& SetBackslash( std::tstring& rDirPath, bool set ) { return SetBackslash( rDirPath, set ? AddSlash : RemoveSlash ); }

	std::tstring GetDirPath( const TCHAR* pPath, TrailSlash trailSlash = PreserveSlash );
	std::tstring GetParentDirPath( const TCHAR* pPath, TrailSlash trailSlash = PreserveSlash );


	std::tstring MakeNormal( const TCHAR* pPath );							// backslashes only
	inline std::tstring& Normalize( std::tstring& rPath ) { return rPath = MakeNormal( rPath.c_str() ); }

	std::tstring MakeCanonical( const TCHAR* pPath );						// relative to absolute normalized: "X:/A\./B\..\C" -> "X:\A\C"
	inline std::tstring& Canonicalize( std::tstring& rPath ) { return rPath = MakeCanonical( rPath.c_str() ); }

	std::tstring Combine( const TCHAR* pDirPath, const TCHAR* pFile );		// canonic merge, pFile could be a relative path, name.ext, or other combinations


	bool IsRoot( const TCHAR* pPath );
	bool IsNetwork( const TCHAR* pPath );
	bool IsURL( const TCHAR* pPath );
	bool IsUNC( const TCHAR* pPath );
	bool IsUNCServer( const TCHAR* pPath );
	bool IsUNCServerShare( const TCHAR* pPath );

	bool HasPrefix( const TCHAR* pPath, const TCHAR* pPrefix );
	bool MatchPrefix( const TCHAR* pPath, const TCHAR* pPrefix );						// normalized HasPrefix

	bool HasSameRoot( const TCHAR* pLeftPath, const TCHAR* pRightPath );				// true for "C:\win\desktop\temp.txt" and "c:\win\tray\sample.txt"
	std::tstring GetRootPath( const TCHAR* pPath );										// "C:\" for "C:\win\desktop\temp.txt"

	std::tstring FindCommonPrefix( const TCHAR* pLeftPath, const TCHAR* pRightPath );	// "C:\win" for "C:\win\desktop\temp.txt" and "c:\win\tray\sample.txt"
	std::tstring StripCommonPrefix( const TCHAR* pFullPath, const TCHAR* pDirPath );	// "desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win\system"
	bool StripPrefix( std::tstring& rPath, const TCHAR* pPrefix );						// "desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win"


	UINT ToHashValue( const TCHAR* pPath );
}


namespace fs
{
	enum AccessMode { Exist = 0, Write = 2, Read = 4, ReadWrite = 6 };

	inline bool FileExist( const TCHAR* pFilePath, AccessMode accessMode = Exist ) { return !str::IsEmpty( pFilePath ) && 0 == _taccess( pFilePath, accessMode ); }


	struct CPathParts
	{
		CPathParts( void ) {}
		CPathParts( const std::tstring& filePath ) { SplitPath( filePath ); }

		bool IsEmpty( void ) const { return m_drive.empty() && m_dir.empty() && m_fname.empty() && m_ext.empty(); }
		void Clear( void );

		std::tstring GetNameExt( void ) const { return m_fname + m_ext; }
		std::tstring GetDirPath( bool trailSlash = false ) const;
		CPathParts& SetNameExt( const std::tstring& nameExt );
		CPathParts& SetDirPath( const std::tstring& dirPath );

		std::tstring MakePath( void ) const;
		void SplitPath( const std::tstring& filePath );

		bool MatchExt( const TCHAR* pExt ) const { return path::EquivalentPtr( m_ext.c_str(), pExt ); }		// pExt: ".txt"

		static std::tstring MakeFullPath( const TCHAR* pDrive, const TCHAR* pDir, const TCHAR* pFname, const TCHAR* pExt );
	public:
		std::tstring m_drive;		// "C:"
		std::tstring m_dir;			// "\dev\Tools\"
		std::tstring m_fname;		// "ReadMe"
		std::tstring m_ext;			// ".txt"
	};


	class CPath
	{
	public:
		CPath( void ) {}
		CPath( const std::tstring& filePath ) { Set( filePath ); }

		bool IsEmpty( void ) const { return m_filePath.empty(); }
		void Clear( void ) { m_filePath.clear(); }

		bool IsValid( void ) const { return path::IsValid( m_filePath ); }
		bool IsComplexPath( void ) const { return path::IsComplex( GetPtr() ); }

		const std::tstring& Get( void ) const { return m_filePath; }
		const TCHAR* GetPtr( void ) const { return m_filePath.c_str(); }
		void Set( const std::tstring& filePath );

		const TCHAR* GetNameExt( void ) const { return path::FindFilename( m_filePath.c_str() ); }
		void SetNameExt( const std::tstring& nameExt );

		std::tstring GetDirPath( bool trailSlash = false ) const;
		void SetDirPath( const std::tstring& dirPath );

		void Normalize( void ) { path::Normalize( m_filePath ); }
		void Canonicalize( void ) { path::Canonicalize( m_filePath ); }

		bool operator==( const CPath& right ) const { return path::Equivalent( m_filePath, right.m_filePath ); }
		bool operator!=( const CPath& right ) const { return !operator==( right ); }

		bool operator<( const CPath& right ) const;

		bool Equivalent( const CPath& right ) const { return path::Equivalent( m_filePath, right.m_filePath ); }
		bool Equal( const CPath& right ) const { return path::Equal( m_filePath, right.m_filePath ); }

		bool FileExist( AccessMode accessMode = Exist ) const { return fs::FileExist( m_filePath.c_str(), accessMode ); }

		CPath ExtractExistingFilePath( void ) const;
		bool QualifyWithSameDirPathIfEmpty( CPath& rOutFilePath ) const;

		inline size_t hash_value( void ) const { return stdext::hash_value( m_filePath ); }
	protected:
		std::tstring& Ref( void ) { return m_filePath; }
	private:
		std::tstring m_filePath;
	};

} // namespace fs


namespace stdext
{
	inline size_t hash_value( const fs::CPath& path ) { return path.hash_value(); }
}


namespace func
{
	struct ToNameExt
	{
		const TCHAR* operator()( const fs::CPath& fullPath ) const { return fullPath.GetNameExt(); }
		const TCHAR* operator()( const TCHAR* pFullPath ) const { return path::FindFilename( pFullPath ); }
	};
}


namespace pred
{
	struct FileExist
	{
		bool operator()( const TCHAR* pFilePath ) const
		{
			return fs::FileExist( pFilePath );
		}

		bool operator()( const fs::CPath& path ) const
		{
			return path.FileExist();
		}
	};


	struct FileExistStr
	{
		bool operator()( const std::tstring& path ) const
		{
			return fs::FileExist( path.c_str() );
		}
	};


	struct EquivalentPathChar
	{
		bool operator()( TCHAR left, TCHAR right ) const
		{
			return path::ToEquivalentChar( left ) == path::ToEquivalentChar( right );
		}
	};


	struct EquivalentPathString
	{
		EquivalentPathString( const std::tstring& path ) : m_path( path ) {}

		bool operator()( const std::tstring& path ) const
		{
			return path::Equivalent( m_path, path );
		}
	private:
		const std::tstring& m_path;
	};


	struct EquivalentPath
	{
		EquivalentPath( const fs::CPath& path ) : m_path( path ) {}

		bool operator()( const fs::CPath& path ) const
		{
			return m_path.Equivalent( path );
		}
	private:
		const fs::CPath& m_path;
	};


	struct ComparePath					// equivalent
	{
		pred::CompareResult operator()( const std::tstring& leftPath, const std::tstring& rightPath ) const
		{
			return str::CompareN( leftPath.c_str(), rightPath.c_str(), &path::ToEquivalentChar );
		}
	};


	struct CompareEquivalentPath		// equivalent | intuitive
	{
		pred::CompareResult operator()( const TCHAR* pLeftPath, const TCHAR* pRightPath ) const
		{
			if ( path::EquivalentPtr( pLeftPath, pRightPath ) )
				return pred::Equal;

			return str::IntuitiveCompare( pLeftPath, pRightPath );
		}

		pred::CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			if ( leftPath == rightPath )
				return pred::Equal;

			return str::IntuitiveCompare( leftPath.GetPtr(), rightPath.GetPtr() );
		}
	};


	typedef CompareAdapter< CompareEquivalentPath, func::ToNameExt > _CompareNameExt;		// equivalent | intuitive
	typedef JoinCompare< _CompareNameExt, CompareEquivalentPath > CompareNameExt;

	typedef LessBy< ComparePath > LessPath;
	typedef LessBy< CompareEquivalentPath > Less_EquivalentPath;
}


namespace fs
{
	// forward declarations
	interface IEnumerator;


	// orders path keys using path equivalence (rather than CPath::operator< intuitive ordering)
	typedef std::map< fs::CPath, fs::CPath, pred::Less_EquivalentPath > PathPairMap;
	typedef std::set< fs::CPath, pred::Less_EquivalentPath > PathSet;
}


#endif // Path_h