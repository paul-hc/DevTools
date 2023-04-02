#ifndef Path_h
#define Path_h
#pragma once

#include "Path_fwd.h"
#include "ComparePredicates.h"
#include "StringCompare.h"
#include <unordered_set>


namespace path
{
	struct CDelims
	{
	public:
		static const std::tstring s_dirDelims;
		static const std::tstring s_hugePrefix;				// huge prefix syntax: path prefixed with "\\?\"
		static const std::tstring s_wildcards;
		static const std::tstring s_multiSpecDelims;
		static const std::tstring s_fmtNumSuffix;			// suffix "_[%d]"

		static const TCHAR s_complexPathSep = '>';			// '>' character that separates the storage file path from the stream/storage embedded sub-path
	};


	// huge prefix syntax: path prefixed with "\\?\"
	inline bool HasHugePrefix( const TCHAR* pPath ) { return str::HasPrefix( pPath, STRING_SPAN( CDelims::s_hugePrefix ) ); }
	bool AutoHugePrefix( IN OUT std::tstring& rPath );		// return true if modified

	// skips the huge prefix, if any
	const TCHAR* GetStart( const TCHAR* pPath );

	template< typename IteratorT >
	inline IteratorT GetStart( IteratorT itFilePath, IteratorT itEnd ) { return itFilePath + ( itFilePath != itEnd && HasHugePrefix( &*itFilePath ) ? CDelims::s_hugePrefix.length() : 0 ); }
}


namespace func
{
	struct ToWinPathChar				// translate slashes to Windows '\\'
	{
		TCHAR operator()( TCHAR ch ) const { return '/' == ch ? '\\' : ch; }
	};

	struct ToUnixPathChar				// translate slashes to Unix '/'
	{
		TCHAR operator()( TCHAR ch ) const { return '\\' == ch ? '/' : ch; }
	};

	struct ToEquivalentPathChar			// lower-case letters or Windows '\\'
	{
		TCHAR operator()( TCHAR ch ) const { return ( '/' == ch || path::CDelims::s_complexPathSep == ch ) ? '\\' : m_toLower( ch ); }
	public:
		func::ToLower m_toLower;
	};


	typedef func::StrCompareBase<ToEquivalentPathChar> TCompareEquivalentPath;
}


namespace pred
{
	typedef CharEqualBase<func::ToEquivalentPathChar> TCharEqualEquivalentPath;

	typedef StrEqualsBase<func::ToEquivalentPathChar> TEquivalentPath;
	typedef StrEquals<str::IgnoreCase> TEqualsPath;
}


namespace path
{
	inline bool Equivalent( const TCHAR* pLeftPath, const TCHAR* pRightPath ) { return pred::TEquivalentPath()( pLeftPath, pRightPath ); }
	inline bool Equals( const TCHAR* pLeftPath, const TCHAR* pRightPath ) { return str::Equals<str::IgnoreCase>( pLeftPath, pRightPath ); }

	inline pred::CompareResult CompareEquivalent( const TCHAR* pLeftPath, const TCHAR* pRightPath, size_t count = std::tstring::npos )
	{
		return func::TCompareEquivalentPath()( pLeftPath, pRightPath, count );
	}

	pred::CompareResult CompareIntuitive( const TCHAR* pLeft, const TCHAR* pRight );


	typedef str::EvalMatch<func::ToWinPathChar, func::ToEquivalentPathChar> TGetMatch;
}


namespace path
{
	// path API

	inline bool IsSlash( TCHAR ch ) { return '\\' == ch || '/' == ch; }

	size_t GetHashValuePtr( const TCHAR* pPath, size_t count = utl::npos );
	inline size_t GetHashValue( const std::tstring& filePath ) { return GetHashValuePtr( filePath.c_str(), filePath.length() ); }


	// path breaks and segment matching
	inline bool IsBreakChar( TCHAR ch ) { return IsSlash( ch ) || CDelims::s_complexPathSep == ch || '\0' == ch; }

	const TCHAR* FindBreak( const TCHAR* pPath );
	const TCHAR* SkipBreak( const TCHAR* pPathBreak );
	bool MatchSegment( const TCHAR* pPath, const TCHAR* pSegment, OUT size_t* pOutMatchLength = nullptr );


	bool MatchWildcard( const TCHAR* pPath, const TCHAR* pWildSpec, const std::tstring& multiSpecDelims = CDelims::s_multiSpecDelims );
	bool IsMultipleWildcard( const TCHAR* pWildSpec, const std::tstring& multiSpecDelims = CDelims::s_multiSpecDelims );
	bool ContainsWildcards( const TCHAR* pWildSpec, const std::tstring& wildcards = CDelims::s_wildcards );

	enum SpecMatch { NoMatch, Match_Any, Match_Prefix, Match_Spec };

	SpecMatch MatchesPrefix( const TCHAR* pFilePath, const TCHAR* pPrefixOrSpec );

	bool IsValid( const std::tstring& path );
	const TCHAR* GetInvalidChars( void );
	const TCHAR* GetReservedChars( void );

	bool IsAbsolute( const TCHAR* pPath );
	bool IsRelative( const TCHAR* pPath );
	bool IsDirectory( const TCHAR* pPath );
	bool IsFilename( const TCHAR* pPath );				// "basefilename[.ext]"

	bool HasDirectory( const TCHAR* pPath );

	const TCHAR* Find( const TCHAR* pPath, const TCHAR* pSubString );
	size_t FindPos( const TCHAR* pPath, const TCHAR* pSubString, size_t offset = 0 );
	inline bool Contains( const TCHAR* pPath, const TCHAR* pSubString ) { return !str::IsEmpty( Find( pPath, pSubString ) ); }

	const TCHAR* FindFilename( const TCHAR* pPath );
	const TCHAR* FindExt( const TCHAR* pPath );
	const TCHAR* SkipRoot( const TCHAR* pPath );		// ignore the drive letter or UNC (Universal Naming Convention) server/share path parts

	inline bool MatchExt( const TCHAR* pPath, const TCHAR* pExt ) { return path::Equivalent( FindExt( pPath ), pExt ); }		// pExt: ".txt"
	bool MatchAnyExt( const TCHAR* pPath, const TCHAR* pMultiExt, TCHAR sepChr = '|' );


	// complex path
	inline bool IsComplex( const TCHAR* pPath ) { return pPath != nullptr && str::Contains( pPath, CDelims::s_complexPathSep ); }
	bool IsWellFormed( const TCHAR* pFilePath );
	inline size_t FindComplexSepPos( const TCHAR* pPath ) { return str::Find<str::Case>( pPath, CDelims::s_complexPathSep ); }
	std::tstring ExtractPhysical( const std::tstring& filePath );	// "C:\Images\fruit.stg" <- "C:\Images\fruit.stg>apple.jpg";  "C:\Images\orange.png" <- "C:\Images\orange.png"
	const TCHAR* GetEmbedded( const TCHAR* pPath );					// "Normal\apple.jpg" <- "C:\Images\fruit.stg>Normal\apple.jpg";  "" <- "C:\Images\orange.png"
	const TCHAR* GetSubPath( const TCHAR* pPath );					// IsComplex() ? GetEmbedded() : pPath

	std::tstring MakeComplex( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath );
	bool SplitComplex( OUT std::tstring& rPhysicalPath, OUT std::tstring& rEmbeddedPath, const std::tstring& filePath );
	bool NormalizeComplexPath( IN OUT std::tstring& rFlexPath, TCHAR chNormalSep = '\\' );			// treat storage document as a directory: make a deep complex path look like a normal full path


	enum TrailSlash { PreserveSlash, AddSlash, RemoveSlash };

	std::tstring& SetBackslash( IN OUT std::tstring& rDirPath, TrailSlash trailSlash = AddSlash );
	inline std::tstring& SetBackslash( IN OUT std::tstring& rDirPath, bool doSet ) { return SetBackslash( rDirPath, doSet ? AddSlash : RemoveSlash ); }
	bool HasTrailingSlash( const TCHAR* pPath );				// true for non-root paths with a trailing slash

	std::tstring GetParentPath( const TCHAR* pPath, TrailSlash trailSlash = PreserveSlash );


	// normal: backslashes only
	bool IsWindows( const TCHAR* pPath );
	std::tstring MakeWindows( const TCHAR* pPath );
	inline std::tstring& Normalize( IN OUT std::tstring& rPath ) { return rPath = MakeWindows( rPath.c_str() ); }

	std::tstring MakeCanonical( const TCHAR* pPath );						// relative to absolute normalized: "X:/A\./B\..\C" -> "X:\A\C"
	inline std::tstring& Canonicalize( IN OUT std::tstring& rPath ) { return rPath = MakeCanonical( rPath.c_str() ); }

	std::tstring Combine( const TCHAR* pDirPath, const TCHAR* pRightPath );	// canonic merge, pRightPath could be a relative path, name.ext, sub-dir path, or other combinations


	bool IsRoot( const TCHAR* pPath );
	bool IsNetwork( const TCHAR* pPath );
	bool IsURL( const TCHAR* pPath );
	bool IsUNC( const TCHAR* pPath );
	bool IsUNCServer( const TCHAR* pPath );
	bool IsUNCServerShare( const TCHAR* pPath );

	bool HasPrefix( const TCHAR* pPath, const TCHAR* pPrefix );
	bool MatchPrefix( const TCHAR* pPath, const TCHAR* pPrefix );						// normalized HasPrefix

	bool HasRoot( const TCHAR* pPath );
	bool HasSameRoot( const TCHAR* pLeftPath, const TCHAR* pRightPath );				// true for "C:\win\desktop\temp.txt" and "c:\win\tray\sample.txt"
	std::tstring GetRootPath( const TCHAR* pPath );										// "C:\" for "C:\win\desktop\temp.txt"

	size_t FindCommonPrefixLength( const TCHAR* pPath, const TCHAR* pPrefix );

	std::tstring FindCommonPrefix( const TCHAR* pLeftPath, const TCHAR* pRightPath );	// "C:\win" for "C:\win\desktop\temp.txt" and "c:\win\tray\sample.txt"
	std::tstring StripCommonPrefix( const TCHAR* pFullPath, const TCHAR* pDirPath );	// "desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win\system"
	bool StripPrefix( IN OUT std::tstring& rPath, const TCHAR* pPrefix );				// "desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win"
}


namespace fs
{
	enum AccessMode { Exist = 0, Write = 2, Read = 4, ReadWrite = 6 };

	bool FileExist( const TCHAR* pFilePath, AccessMode accessMode = Exist );
	bool IsValidDirectory( const TCHAR* pDirPath );


	struct CPathParts
	{
		CPathParts( void ) {}
		CPathParts( const std::tstring& filePath ) { SplitPath( filePath ); }

		bool IsEmpty( void ) const { return m_drive.empty() && m_dir.empty() && m_fname.empty() && m_ext.empty(); }
		void Clear( void );

		std::tstring GetFilename( void ) const { return m_fname + m_ext; }
		fs::CPath GetDirPath( void ) const;
		CPathParts& SetFilename( const std::tstring& nameExt );
		CPathParts& SetDirPath( const std::tstring& dirPath );

		fs::CPath MakePath( void ) const;
		void SplitPath( const std::tstring& filePath );

		bool MatchExt( const TCHAR* pExt ) const { return path::Equivalent( m_ext.c_str(), pExt ); }		// pExt: ".txt"

		static std::tstring MakeFullPath( const TCHAR* pDrive, const TCHAR* pDir, const TCHAR* pFname, const TCHAR* pExt );
	public:
		std::tstring m_drive;		// "C:"
		std::tstring m_dir;			// "\dev\Tools\"
		std::tstring m_fname;		// "ReadMe"
		std::tstring m_ext;			// ".txt"
	};


	enum ExtensionMatch { MatchExt, MatchDiffCaseExt, MismatchDotsExt, MismatchExt };


	class CPath
	{
	public:
		CPath( void ) {}
		CPath( const std::tstring& filePath ) { Set( filePath ); }
		CPath( const CPath* pFilePath ) : m_filePath( safe_ptr( pFilePath )->Get() ) { Canonicalize(); }		// canonic conversion ctor

		template< typename ToCharFunc >
		CPath( const std::tstring& filePath, ToCharFunc toCharFunc ) { Set( filePath ); Convert( toCharFunc ); }

		template< typename ToCharFunc >
		CPath& Convert( ToCharFunc toCharFunc )
		{
			std::transform( m_filePath.begin(), m_filePath.end(), m_filePath.begin(), toCharFunc );	// write to the same location
			return *this;
		}

		bool IsEmpty( void ) const { return m_filePath.empty(); }
		bool IsHuge( void ) const { return path::HasHugePrefix( m_filePath.c_str() ); }
		void Clear( void ) { m_filePath.clear(); }
		void Swap( CPath& rOther ) { m_filePath.swap( rOther.m_filePath ); }

		bool IsValid( void ) const { return path::IsValid( m_filePath ); }
		bool IsComplexPath( void ) const { return path::IsComplex( GetPtr() ); }
		bool IsPhysicalPath( void ) const { return !IsComplexPath(); }

		const std::tstring& Get( void ) const { return m_filePath; }
		std::tstring& Ref( void ) { return m_filePath; }
		void Set( const std::tstring& filePath );

		const TCHAR* GetPtr( void ) const { return m_filePath.c_str(); }
		const TCHAR* GetStart( void )const { return path::GetStart( m_filePath.c_str() ); }		// skips the huge prefix, if any
		std::string GetUtf8( void ) const { return str::ToUtf8( GetPtr() ); }

		CPath MakeUnixPath( void ) const { return CPath( m_filePath, func::ToUnixPathChar() ); }
		CPath MakeWindowsPath( void ) const { return CPath( m_filePath, func::ToWinPathChar() ); }

		size_t GetDepth( void ) const;		// count of path elements up to the root

		bool HasParentPath( void ) const { return GetFilenamePtr() != GetPtr(); }		// has a directory path?
		TDirPath GetParentPath( bool trailSlash = false ) const;						// always a directory path
		CPath& SetBackslash( bool trailSlash = true );

		std::tstring GetFilename( void ) const { return GetFilenamePtr(); }
		const TCHAR* GetFilenamePtr( void ) const { return path::FindFilename( m_filePath.c_str() ); }
		void SetFilename( const std::tstring& filename );

		const TCHAR* GetExt( void ) const { return path::FindExt( m_filePath.c_str() ); }
		bool HasExt( void ) const { return !str::IsEmpty( GetExt() ); }
		bool ExtEquals( const TCHAR* pExt ) const { return path::Equivalent( GetExt(), pExt ); }
		ExtensionMatch GetExtensionMatch( const fs::CPath& right ) const;

		void SplitFilename( std::tstring& rFname, std::tstring& rExt ) const;

		std::tstring GetFname( void ) const;
		void ReplaceFname( const TCHAR* pFname ) { ASSERT_PTR( pFname ); SetFilename( std::tstring( pFname ) + GetExt() ); }

		void ReplaceExt( const TCHAR* pExt ) { Set( GetRemoveExt().Get() + pExt ); }
		void RemoveExt( void ) { Set( GetRemoveExt().Get() ); }
		CPath GetRemoveExt( void ) const;

		void SetDirPath( const std::tstring& dirPath );

		void Normalize( void ) { path::Normalize( m_filePath ); }
		void Canonicalize( void ) { path::Canonicalize( m_filePath ); }

		CPath operator/( const CPath& right ) const { return CPath( path::Combine( GetStart(), right.GetStart() ) ); }
		CPath operator/( const TCHAR* pRight ) const { return CPath( path::Combine( GetStart(), path::GetStart( pRight ) ) ); }

		CPath& operator/=( const CPath& right );
		CPath& operator/=( const TCHAR* pRight ) { Set( path::Combine( GetStart(), path::GetStart( pRight ) ) ); return *this; }

		bool operator==( const CPath& right ) const { return Equivalent( right ); }
		bool operator!=( const CPath& right ) const { return !operator==( right ); }

		bool operator<( const CPath& right ) const;

		bool Equivalent( const CPath& right ) const { return path::Equivalent( GetStart(), right.GetStart() ); }
		bool Equals( const CPath& right ) const { return path::Equals( GetStart(), right.GetStart() ); }

		bool FileExist( AccessMode accessMode = Exist ) const { return fs::FileExist( m_filePath.c_str(), accessMode ); }

		bool LocateFile( CFileFind& rFindFile ) const;
		CPath LocateExistingFile( void ) const;

		inline size_t GetHashValue( void ) const { return path::GetHashValue( m_filePath ); }
	private:
		std::tstring m_filePath;
	};


	fs::CPath GetShortFilePath( const fs::CPath& filePath );
	fs::CPath GetLongFilePath( const fs::CPath& filePath );

	// pattern path utils (potentially with wildcards):

	enum PatternResult { ValidFile, ValidDirectory, InvalidPattern };

	PatternResult SplitPatternPath( fs::CPath* pPath, std::tstring* pWildSpec, const fs::TPatternPath& patternPath );	// a valid file or valid directory path with a wildcards?
}


template<>
struct std::hash<fs::CPath>
{
	inline std::size_t operator()( const fs::CPath& filePath ) const /*noexcept*/
	{
		return filePath.GetHashValue();
	}
};


namespace func
{
	struct ToNameExt
	{
		const TCHAR* operator()( const fs::CPath& fullPath ) const { return fullPath.GetFilenamePtr(); }
		const TCHAR* operator()( const TCHAR* pFullPath ) const { return path::FindFilename( pFullPath ); }
	};


	inline const std::tstring& StringOf( const fs::CPath& filePath ) { return filePath.Get(); }		// for uniform string algorithms

	inline const fs::CPath& PathOf( const fs::CPath& keyPath ) { return keyPath; }
}


namespace fs { struct CFileState; }


namespace pred
{
	// path binary predicates

	struct CompareNaturalPath : public BaseComparator		// equivalent | natural-intuitive
	{
		pred::CompareResult operator()( const TCHAR* pLeftPath, const TCHAR* pRightPath ) const
		{
			if ( path::Equivalent( pLeftPath, pRightPath ) )
				return pred::Equal;

			return path::CompareIntuitive( pLeftPath, pRightPath );
		}

		pred::CompareResult operator()( const std::tstring& leftPath, const std::tstring& rightPath ) const
		{
			return operator()( leftPath.c_str(), rightPath.c_str() );
		}

		pred::CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			if ( leftPath == rightPath )
				return pred::Equal;

			return path::CompareIntuitive( leftPath.GetPtr(), rightPath.GetPtr() );
		}

		pred::CompareResult operator()( const fs::CFileState& leftState, const fs::CFileState& rightState ) const;
	};


	typedef CompareAdapter<CompareNaturalPath, func::ToNameExt> _CompareNameExt;		// filename | fullpath
	typedef JoinCompare<_CompareNameExt, CompareNaturalPath> TCompareNameExt;

	typedef LessValue<CompareNaturalPath> TLess_NaturalPath;
	typedef LessValue<TCompareNameExt> TLess_NameExt;
}


namespace pred
{
	// path unary predicates

	template<>
	inline bool IsEmpty::operator()( const fs::CPath& object ) const
	{
		return object.IsEmpty();
	}


	struct HasExtension : public std::unary_function<TCHAR, bool>
	{
		HasExtension( const TCHAR* pPath ) : m_pExtension( path::FindExt( pPath ) ) { ASSERT_PTR( m_pExtension ); }

		bool operator()( const TCHAR* pExt ) const
		{
			ASSERT_PTR( pExt );
			return path::Equivalent( m_pExtension, pExt );
		}

		bool operator()( const std::tstring& ext ) const
		{
			return path::Equivalent( m_pExtension, ext.c_str() );
		}
	public:
		const TCHAR* m_pExtension;
	};


	struct FileExist
	{
		bool operator()( const TCHAR* pFilePath ) const
		{
			return fs::FileExist( pFilePath );
		}

		template< typename PathKey >
		bool operator()( const PathKey& pathKey ) const
		{
			return func::PathOf( pathKey ).FileExist();
		}
	};


	struct FileExistStr
	{
		bool operator()( const std::tstring& path ) const
		{
			return fs::FileExist( path.c_str() );
		}
	};
}


#include <map>
#include <set>


namespace fs
{
	// forward declarations
	interface IEnumerator;


	typedef std::pair<fs::CPath, fs::CPath> TPathPair;


	// orders path keys using path equivalence (rather than CPath::operator< intuitive ordering)
	typedef std::map<fs::CPath, fs::CPath, pred::TLess_NaturalPath> TPathPairMap;
	typedef std::set<fs::CPath, pred::TLess_NaturalPath> TPathSet;


	// path sort for std::tstring, fs::CPath, fs::CFlexPath

	template< typename IteratorT >
	inline void SortPaths( IteratorT itFirst, IteratorT itLast, bool ascending = true )
	{
		std::sort( itFirst, itLast, pred::OrderByValue<pred::CompareNaturalPath>( ascending ) );
	}

	template< typename ContainerT >
	inline void SortPaths( ContainerT& rStrPaths, bool ascending = true )
	{
		SortPaths( rStrPaths.begin(), rStrPaths.end(), ascending );
	}

	void SortByPathDepth( std::vector<fs::CPath>& rFilePaths, bool ascending = true );
}


namespace utl
{
	// FWD:

	template< typename ContainerT, typename IteratorT >
	size_t JoinUnique( ContainerT& rDest, IteratorT itStart, IteratorT itEnd );
}


namespace path
{
	fs::CPath StripWildcards( const fs::TPatternPath& patternPath );

	inline fs::CPath StripDirPrefix( const fs::CPath& filePath, const fs::TDirPath& dirPath ) { return path::StripCommonPrefix( filePath.GetPtr(), dirPath.GetPtr() ); }


	template< typename ContainerT, typename SetT >
	size_t UniquifyPaths( ContainerT& rPaths, SetT& rUniquePathIndex )
	{
		ContainerT tempPaths;
		tempPaths.reserve( rPaths.size() );

		size_t duplicateCount = 0;

		for ( typename ContainerT::const_iterator itPath = rPaths.begin(); itPath != rPaths.end(); ++itPath )
			if ( rUniquePathIndex.insert( func::PathOf( *itPath ) ).second )		// path is unique?
				tempPaths.push_back( *itPath );
			else
				++duplicateCount;

		rPaths.swap( tempPaths );
		return duplicateCount;
	}


	template< typename ContainerT >
	size_t UniquifyPaths( ContainerT& rPaths )
	{
		std::unordered_set<typename ContainerT::value_type> uniquePathIndex;
		return UniquifyPaths( rPaths, uniquePathIndex );
	}


	template< typename ContainerT, typename SrcContainerT >
	size_t JoinUniquePaths( ContainerT& rDestPaths, const SrcContainerT& newPaths )
	{
		size_t oldCount = rDestPaths.size();

		if ( oldCount + newPaths.size() < 100 )											// small total number?
			return utl::JoinUnique( rDestPaths, newPaths.begin(), newPaths.end() );		// use linear unquifying

		// many items optimization: add all and use hash unquifying
		rDestPaths.insert( rDestPaths.end(), newPaths.begin(), newPaths.end() );
		UniquifyPaths( rDestPaths );

		return rDestPaths.size() - oldCount;		// added count
	}

} // namespace path


namespace fs
{
	namespace traits
	{
		// for path makers

		inline const fs::CPath& GetPath( const fs::CPath& filePath ) { return filePath; }

		template< typename DestType >
		inline void SetPath( DestType& rDest, const fs::CPath& filePath ) { rDest.Set( filePath.Get() ); }
	}
}


namespace str
{
	namespace traits
	{
		inline const TCHAR* GetCharPtr( const fs::CPath& filePath ) { return filePath.Get().c_str(); }
		inline size_t GetLength( const fs::CPath& filePath ) { return filePath.Get().length(); }
	}
}


namespace func
{
	struct AppendPath		// e.g. used to append the same wildcard spec to some paths
	{
		AppendPath( const fs::CPath& subPath ) : m_subPath( subPath ) {}

		void operator()( fs::TDirPath& rPath ) const
		{
			rPath /= m_subPath;
		}
	private:
		const fs::CPath& m_subPath;
	};


	struct PrefixPath
	{
		PrefixPath( const fs::TDirPath& dirPath ) : m_dirPath( dirPath ) {}

		void operator()( fs::CPath& rPath ) const
		{
			rPath = m_dirPath / rPath;
		}
	private:
		const fs::TDirPath& m_dirPath;
	};


	struct AppendToDirPath		// e.g. used to append the same wildcard spec to some dir paths
	{
		AppendToDirPath( const fs::CPath& subPath ) : m_subPath( subPath ) {}

		void operator()( fs::CPath& rPath ) const
		{
			if ( fs::IsValidDirectory( rPath.GetPtr() ) )
				rPath /= m_subPath;
		}
	private:
		const fs::CPath& m_subPath;
	};


	struct HasCommonPathPrefix
	{
		HasCommonPathPrefix( const TCHAR* pDirPrefix ) : m_pDirPrefix( pDirPrefix ) { ASSERT( !str::IsEmpty( m_pDirPrefix ) ); }

		template< typename StringyT >
		bool operator()( const StringyT& filePath )
		{
			return path::HasPrefix( StringOf( filePath ).c_str(), m_pDirPrefix );
		}
	public:
		const TCHAR* m_pDirPrefix;
	};


	struct StripPathCommonPrefix
	{
		StripPathCommonPrefix( const TCHAR* pDirPrefix ) : m_pDirPrefix( pDirPrefix ), m_count( 0 ) { ASSERT_PTR( m_pDirPrefix ); }

		bool operator()( std::tstring& rFilePath )
		{
			if ( !path::StripPrefix( rFilePath, m_pDirPrefix ) )
				return false;

			++m_count;
			return true;
		}

		bool operator()( fs::CPath& rPath ) { return operator()( rPath.Ref() ); }
	public:
		const TCHAR* m_pDirPrefix;
		size_t m_count;
	};


	struct StripComplexPath
	{
		void operator()( std::tstring& rFilePath )
		{
			const TCHAR* pSrcEmbedded = path::GetEmbedded( rFilePath.c_str() );
			if ( !str::IsEmpty( pSrcEmbedded ) )
				rFilePath = pSrcEmbedded;
		}

		void operator()( fs::CPath& rPath ) { return operator()( rPath.Ref() ); }
	};


	struct StripRootPrefix
	{
		void operator()( std::tstring& rFilePath ) const
		{
			rFilePath = path::SkipRoot( rFilePath.c_str() );
		}

		void operator()( fs::CPath& rPath ) { operator()( rPath.Ref() ); }
	};


	struct StripToFilename			// remove directory path
	{
		void operator()( std::tstring& rFilePath ) const
		{
			rFilePath = path::FindFilename( rFilePath.c_str() );
		}

		void operator()( fs::CPath& rPath ) { rPath.Set( rPath.GetFilenamePtr() ); }
	};
}


namespace path
{
	void QueryParentPaths( std::vector<fs::CPath>& rParentPaths, const std::vector<fs::CPath>& filePaths, bool uniqueOnly = true );


	template< typename ContainerT >
	bool HasMultipleDirPaths( const ContainerT& paths )			// uses func::PathOf() to extract path of element - for container versatility (map, vector, etc)
	{
		if ( !paths.empty() )
		{
			typename ContainerT::const_iterator it = paths.begin();
			const fs::TDirPath dirPath = func::PathOf( *it ).GetParentPath();

			for ( ; it != paths.end(); ++it )
				if ( func::PathOf( *it ).GetParentPath() != dirPath )
					return true;
		}

		return false;
	}

	template< typename ContainerT >
	fs::TDirPath ExtractCommonParentPath( const ContainerT& paths )	// uses func::PathOf() to extract path of element - for container versatility (map, vector, etc)
	{
		std::tstring commonPrefix;
		if ( !paths.empty() )
		{
			typename ContainerT::const_iterator it = paths.begin();
			commonPrefix = func::PathOf( *it ).GetParentPath().Get();

			for ( ; it != paths.end(); ++it )
			{
				commonPrefix = path::FindCommonPrefix( commonPrefix.c_str(), func::PathOf( *it ).GetParentPath().GetPtr() );
				if ( commonPrefix.empty() )
					break;						// no common prefix, abort search
			}
		}

		return commonPrefix;
	}


	template< typename ContainerT >
	inline size_t StripDirPrefixes( ContainerT& rPaths, const TCHAR* pDirPrefix )
	{
		return std::for_each( rPaths.begin(), rPaths.end(), func::StripPathCommonPrefix( pDirPrefix ) ).m_count;		// count of stripped prefixes
	}

	template< typename ContainerT >
	inline size_t StripCommonParentPath( ContainerT& rPaths )
	{
		fs::TDirPath commonDirPath = ExtractCommonParentPath( rPaths );
		return !commonDirPath.IsEmpty() ? StripDirPrefixes( rPaths, commonDirPath.GetPtr() ) : 0;
	}


	template< typename ContainerT >
	inline void StripRootPrefixes( ContainerT& rPaths )		// cut the drive letter or UNC server/share
	{
		std::for_each( rPaths.begin(), rPaths.end(), func::StripRootPrefix() );
	}

	template< typename ContainerT >
	inline void StripToFilename( ContainerT& rPaths )		// cut the directory path
	{
		std::for_each( rPaths.begin(), rPaths.end(), func::StripToFilename() );
	}
}


#include <iosfwd>


inline std::ostream& operator<<( std::ostream& os, const fs::CPath& path )
{
	return os << path.GetPtr();
}

inline std::wostream& operator<<( std::wostream& os, const fs::CPath& path )
{
	return os << path.GetPtr();
}


#endif // Path_h
