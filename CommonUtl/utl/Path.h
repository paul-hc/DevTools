#ifndef Path_h
#define Path_h
#pragma once

#include <io.h>
#include <map>
#include <set>
#include <xhash>
#include "ComparePredicates.h"
#include "StringCompare.h"


namespace func
{
	// Translates characters to generate a natural order, useful for sorting paths and filenames.
	// Natural order: intuitive (case insensitive, order numeric sequences by value) | punctuation first (shortest filename first in a tie).
	// Note: This is closer to Explorer.exe sort order (which varies with Windows version), yet different than ::StrCmpLogicalW from <shlwapi.h>
	//
	struct ToNaturalPathChar
	{
		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return static_cast< CharType >( Translate( ch ) );
		}

		static int Translate( int charCode );
	};
}


namespace path
{
	extern const TCHAR s_complexPathSep;			// '>' character that separates the storage file path from the stream/storage embedded sub-path

	inline bool IsSlash( TCHAR ch ) { return _T('\\') == ch || _T('/') == ch; }
	inline TCHAR ToNormalChar( TCHAR ch ) { return _T('/') == ch ? _T('\\') : ch; }
	inline TCHAR ToEquivalentChar( TCHAR ch ) { return _T('/') == ch || s_complexPathSep == ch ? _T('\\') : ::towlower( ch ); }
	const TCHAR* DirDelims( void );

	bool EquivalentPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath );
	bool Equivalent( const std::tstring& leftPath, const std::tstring& rightPath );

	bool EqualsPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath );
	inline bool Equals( const std::tstring& leftPath, const std::tstring& rightPath ) { return EqualsPtr( leftPath.c_str(), rightPath.c_str() ); }

	pred::CompareResult CompareNaturalPtr( const TCHAR* pLeft, const TCHAR* pRight );

	const TCHAR* StdFormatNumSuffix( void );		// "_[%d]"
	size_t GetHashValue( const TCHAR* pPath );


	struct ToNormal
	{
		TCHAR operator()( TCHAR ch ) const { return ToNormalChar( ch ); }
	};

	struct ToEquivalent
	{
		TCHAR operator()( TCHAR ch ) const { return ToEquivalentChar( ch ); }
	};


	inline pred::CompareResult CompareNPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath, size_t count = std::tstring::npos )
	{
		return str::_CompareN( pLeftPath, pRightPath, path::ToEquivalent(), count );
	}

	typedef str::EvalMatch< ToNormal, ToEquivalent > GetMatch;


	// path breaks and segment matching
	inline bool IsBreakChar( TCHAR ch ) { return IsSlash( ch ) || s_complexPathSep == ch || _T('\0') == ch; }

	const TCHAR* FindBreak( const TCHAR* pPath );
	const TCHAR* SkipBreak( const TCHAR* pPathBreak );
	bool MatchSegment( const TCHAR* pPath, const TCHAR* pSegment, size_t* pMatchLength = NULL );


	const TCHAR* Wildcards( void );
	const TCHAR* MultiSpecDelims( void );

	bool MatchWildcard( const TCHAR* pPath, const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims = MultiSpecDelims() );
	bool IsMultipleWildcard( const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims = MultiSpecDelims() );
	bool ContainsWildcards( const TCHAR* pWildSpec, const TCHAR* pWildcards = Wildcards() );

	enum SpecMatch { NoMatch, Match_Any, Match_Prefix, Match_Spec };

	SpecMatch MatchesPrefix( const TCHAR* pFilePath, const TCHAR* pPrefixOrSpec );

	bool IsValid( const std::tstring& path );
	const TCHAR* GetInvalidChars( void );
	const TCHAR* GetReservedChars( void );

	bool IsAbsolute( const TCHAR* pPath );
	bool IsRelative( const TCHAR* pPath );
	bool IsDirectory( const TCHAR* pPath );
	bool IsFilename( const TCHAR* pPath );		// "basefilename[.ext]"

	bool HasDirectory( const TCHAR* pPath );

	const TCHAR* Find( const TCHAR* pPath, const TCHAR* pSubString );
	inline bool Contains( const TCHAR* pPath, const TCHAR* pSubString ) { return !str::IsEmpty( Find( pPath, pSubString ) ); }

	const TCHAR* FindFilename( const TCHAR* pPath );
	const TCHAR* FindExt( const TCHAR* pPath );
	const TCHAR* SkipRoot( const TCHAR* pPath );		// ignore the drive letter or Universal Naming Convention (UNC) server/share path parts

	inline bool MatchExt( const TCHAR* pPath, const TCHAR* pExt ) { return EquivalentPtr( FindExt( pPath ), pExt ); }		// pExt: ".txt"


	// huge prefix syntax: path prefixed with "\\?\"
	const std::tstring& GetHugePrefix( void );
	bool HasHugePrefix( const TCHAR* pPath );			// uses path syntax with "\\?\" prefix?
	const TCHAR* SkipHugePrefix( const TCHAR* pPath );
	bool SetHugePrefix( std::tstring& rPath, bool useHugePrefixSyntax = true );		// return true if modified


	// complex path
	inline bool IsComplex( const TCHAR* pPath ) { return pPath != NULL && _tcschr( pPath, s_complexPathSep ) != NULL; }
	bool IsWellFormed( const TCHAR* pFilePath );
	size_t FindComplexSepPos( const TCHAR* pPath );
	std::tstring ExtractPhysical( const std::tstring& filePath );	// "C:\Images\fruit.stg" <- "C:\Images\fruit.stg>apple.jpg";  "C:\Images\orange.png" <- "C:\Images\orange.png"
	const TCHAR* GetEmbedded( const TCHAR* pPath );					// "Normal\apple.jpg" <- "C:\Images\fruit.stg>Normal\apple.jpg";  "" <- "C:\Images\orange.png"
	const TCHAR* GetSubPath( const TCHAR* pPath );					// IsComplex() ? GetEmbedded() : pPath

	std::tstring MakeComplex( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath );
	bool SplitComplex( std::tstring& rPhysicalPath, std::tstring& rEmbeddedPath, const std::tstring& filePath );
	bool NormalizeComplexPath( std::tstring& rFlexPath, TCHAR chNormalSep = _T('\\') );			// treat storage document as a directory: make a deep complex path look like a normal full path


	enum TrailSlash { PreserveSlash, AddSlash, RemoveSlash };

	std::tstring& SetBackslash( std::tstring& rDirPath, TrailSlash trailSlash = AddSlash );
	inline std::tstring& SetBackslash( std::tstring& rDirPath, bool doSet ) { return SetBackslash( rDirPath, doSet ? AddSlash : RemoveSlash ); }
	bool HasTrailingSlash( const TCHAR* pPath );				// true for non-root paths with a trailing slash

	std::tstring GetParentPath( const TCHAR* pPath, TrailSlash trailSlash = PreserveSlash );


	// normal: backslashes only
	bool IsNormal( const TCHAR* pPath );
	std::tstring MakeNormal( const TCHAR* pPath );
	inline std::tstring& Normalize( IN OUT std::tstring& rPath ) { return rPath = MakeNormal( rPath.c_str() ); }

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
	bool StripPrefix( std::tstring& rPath, const TCHAR* pPrefix );						// "desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win"
}


namespace fs
{
	enum AccessMode { Exist = 0, Write = 2, Read = 4, ReadWrite = 6 };

	inline bool FileExist( const TCHAR* pFilePath, AccessMode accessMode = Exist ) { return !str::IsEmpty( pFilePath ) && 0 == _taccess( pFilePath, accessMode ); }


	class CPath;


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

		bool MatchExt( const TCHAR* pExt ) const { return path::EquivalentPtr( m_ext.c_str(), pExt ); }		// pExt: ".txt"

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

		bool IsEmpty( void ) const { return m_filePath.empty(); }
		void Clear( void ) { m_filePath.clear(); }
		void Swap( CPath& rOther ) { m_filePath.swap( rOther.m_filePath ); }

		bool IsValid( void ) const { return path::IsValid( m_filePath ); }
		bool IsComplexPath( void ) const { return path::IsComplex( GetPtr() ); }
		bool IsPhysicalPath( void ) const { return !IsComplexPath(); }

		const std::tstring& Get( void ) const { return m_filePath; }
		std::tstring& Ref( void ) { return m_filePath; }
		void Set( const std::tstring& filePath );

		const TCHAR* GetPtr( void ) const { return m_filePath.c_str(); }
		std::string GetUtf8( void ) const { return str::ToUtf8( GetPtr() ); }

		size_t GetDepth( void ) const;		// count of path elements up to the root

		bool HasParentPath( void ) const { return GetFilenamePtr() != GetPtr(); }		// has a directory path?
		CPath GetParentPath( bool trailSlash = false ) const;						// always a directory path
		CPath& SetBackslash( bool trailSlash = true );

		std::tstring GetFilename( void ) const { return GetFilenamePtr(); }
		const TCHAR* GetFilenamePtr( void ) const { return path::FindFilename( m_filePath.c_str() ); }
		void SetFilename( const std::tstring& filename );

		const TCHAR* GetExt( void ) const { return path::FindExt( m_filePath.c_str() ); }
		bool HasExt( const TCHAR* pExt ) const { return path::EquivalentPtr( GetExt(), pExt ); }
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

		bool HasHugePrefix( void ) const { return path::HasHugePrefix( m_filePath.c_str() ); }
		bool SetHugePrefix( bool useHugePrefixSyntax = true ) { return path::SetHugePrefix( m_filePath, useHugePrefixSyntax ); }

		CPath operator/( const CPath& right ) const { return CPath( path::Combine( GetPtr(), right.GetPtr() ) ); }
		CPath operator/( const TCHAR* pRight ) const { return CPath( path::Combine( GetPtr(), pRight ) ); }

		CPath& operator/=( const CPath& right );
		CPath& operator/=( const TCHAR* pRight ) { Set( path::Combine( GetPtr(), pRight ) ); return *this; }

		bool operator==( const CPath& right ) const { return path::Equivalent( m_filePath, right.m_filePath ); }
		bool operator!=( const CPath& right ) const { return !operator==( right ); }

		bool operator<( const CPath& right ) const;

		bool Equivalent( const CPath& right ) const { return path::Equivalent( m_filePath, right.m_filePath ); }
		bool Equals( const CPath& right ) const { return path::Equals( m_filePath, right.m_filePath ); }

		bool FileExist( AccessMode accessMode = Exist ) const { return fs::FileExist( m_filePath.c_str(), accessMode ); }

		bool LocateFile( CFileFind& rFindFile ) const;
		CPath ExtractExistingFilePath( void ) const;

		inline size_t GetHashValue( void ) const { return path::GetHashValue( m_filePath.c_str() ); }
	private:
		std::tstring m_filePath;
	};


	fs::CPath GetShortFilePath( const fs::CPath& filePath );
	fs::CPath GetLongFilePath( const fs::CPath& filePath );

	inline fs::CPath StripDirPrefix( const fs::CPath& filePath, const fs::CPath& dirPath ) { return path::StripCommonPrefix( filePath.GetPtr(), dirPath.GetPtr() ); }


	// pattern path utils (potentially with wildcards):

	fs::CPath StripWildcards( const fs::CPath& patternPath );

	enum PatternResult { ValidFile, ValidDirectory, InvalidPattern };

	PatternResult SplitPatternPath( fs::CPath* pPath, std::tstring* pWildSpec, const fs::CPath& patternPath );		// a valid file or valid directory path with a wildcards?
}


namespace fs
{
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
	inline size_t UniquifyPaths( ContainerT& rPaths )
	{
		stdext::hash_set< typename ContainerT::value_type > uniquePathIndex;
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

} // namespace fs


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


namespace stdext
{
	inline size_t hash_value( const fs::CPath& path ) { return path.GetHashValue(); }
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
	inline const std::tstring& StringOf( const fs::CPath& filePath ) { return filePath.Get(); }		// for uniform string algorithms
	inline const fs::CPath& PathOf( const fs::CPath& keyPath ) { return keyPath; }					// for uniform path algorithms


	struct ToNameExt
	{
		const TCHAR* operator()( const fs::CPath& fullPath ) const { return fullPath.GetFilenamePtr(); }
		const TCHAR* operator()( const TCHAR* pFullPath ) const { return path::FindFilename( pFullPath ); }
	};


	struct PrefixPath
	{
		PrefixPath( const fs::CPath& folderPath ) : m_folderPath( folderPath ) {}

		void operator()( fs::CPath& rPath ) const
		{
			rPath = m_folderPath / rPath;
		}
	private:
		fs::CPath m_folderPath;
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

		void operator()( fs::CPath& rPath ) { operator()( rPath.Ref() ); }
	};
}


namespace pred
{
	// path unary predicates

	template<>
	inline bool IsEmpty::operator()( const fs::CPath& object ) const
	{
		return object.IsEmpty();
	}


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


	struct IsEquivalentPathChar : public std::unary_function< bool, TCHAR >
	{
		bool operator()( TCHAR left, TCHAR right ) const
		{
			return path::ToEquivalentChar( left ) == path::ToEquivalentChar( right );
		}
	};


	struct IsEquivalentPathString : public std::unary_function< bool, std::tstring >
	{
		IsEquivalentPathString( const std::tstring& path ) : m_path( path ) {}

		bool operator()( const std::tstring& path ) const
		{
			return path::Equivalent( m_path, path );
		}
	private:
		const std::tstring& m_path;
	};


	struct IsEquivalentPath : public std::unary_function< bool, fs::CPath >
	{
		IsEquivalentPath( const fs::CPath& path ) : m_path( path ) {}

		bool operator()( const fs::CPath& path ) const
		{
			return m_path.Equivalent( path );
		}
	private:
		const fs::CPath& m_path;
	};


	struct IsPathEquivalent : public std::binary_function< bool, fs::CPath, fs::CPath >
	{
		bool operator()( const fs::CPath& left, const fs::CPath& right ) const
		{
			return left.Equivalent( right );
		}
	};
}


namespace fs { struct CFileState; }


namespace pred
{
	// path binary predicates

	struct CompareEquivPath			// equivalent
	{
		pred::CompareResult operator()( const std::tstring& leftPath, const std::tstring& rightPath ) const
		{
			return str::_CompareN( leftPath.c_str(), rightPath.c_str(), &path::ToEquivalentChar );
		}
	};


	struct CompareNaturalPath		// equivalent | natural-intuitive
	{
		pred::CompareResult operator()( const TCHAR* pLeftPath, const TCHAR* pRightPath ) const
		{
			if ( path::EquivalentPtr( pLeftPath, pRightPath ) )
				return pred::Equal;

			return path::CompareNaturalPtr( pLeftPath, pRightPath );
		}

		pred::CompareResult operator()( const std::tstring& leftPath, const std::tstring& rightPath ) const
		{
			return operator()( leftPath.c_str(), rightPath.c_str() );
		}

		pred::CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			if ( leftPath == rightPath )
				return pred::Equal;

			return path::CompareNaturalPtr( leftPath.GetPtr(), rightPath.GetPtr() );
		}

		pred::CompareResult operator()( const fs::CFileState& leftState, const fs::CFileState& rightState ) const;
	};


	typedef CompareAdapter< CompareNaturalPath, func::ToNameExt > _CompareNameExt;		// filename | fullpath
	typedef JoinCompare< _CompareNameExt, CompareNaturalPath > TCompareNameExt;

	typedef LessValue< CompareNaturalPath > TLess_NaturalPath;
}


namespace fs
{
	// forward declarations
	interface IEnumerator;


	// orders path keys using path equivalence (rather than CPath::operator< intuitive ordering)
	typedef std::map< fs::CPath, fs::CPath, pred::TLess_NaturalPath > TPathPairMap;
	typedef std::set< fs::CPath, pred::TLess_NaturalPath > TPathSet;


	// path sort for std::tstring, fs::CPath, fs::CFlexPath

	template< typename IteratorT >
	inline void SortPaths( IteratorT itFirst, IteratorT itLast, bool ascending = true )
	{
		std::sort( itFirst, itLast, pred::OrderByValue< pred::CompareNaturalPath >( ascending ) );
	}

	template< typename ContainerT >
	inline void SortPaths( ContainerT& rStrPaths, bool ascending = true )
	{
		SortPaths( rStrPaths.begin(), rStrPaths.end(), ascending );
	}

	void SortByPathDepth( std::vector< fs::CPath >& rFilePaths, bool ascending = true );
}


namespace path
{
	void QueryParentPaths( std::vector< fs::CPath >& rParentPaths, const std::vector< fs::CPath >& filePaths, bool uniqueOnly = true );


	template< typename ContainerT >
	bool HasMultipleDirPaths( const ContainerT& paths )			// uses func::PathOf() to extract path of element - for container versatility (map, vector, etc)
	{
		if ( !paths.empty() )
		{
			typename ContainerT::const_iterator it = paths.begin();
			const fs::CPath dirPath = func::PathOf( *it ).GetParentPath();

			for ( ; it != paths.end(); ++it )
				if ( func::PathOf( *it ).GetParentPath() != dirPath )
					return true;
		}

		return false;
	}

	template< typename ContainerT >
	fs::CPath ExtractCommonParentPath( const ContainerT& paths )	// uses func::PathOf() to extract path of element - for container versatility (map, vector, etc)
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
	inline size_t StripDirPrefix( ContainerT& rPaths, const TCHAR* pDirPrefix )
	{
		return std::for_each( rPaths.begin(), rPaths.end(), func::StripPathCommonPrefix( pDirPrefix ) ).m_count;		// count of stripped prefixes
	}

	template< typename ContainerT >
	inline size_t StripCommonParentPath( ContainerT& rPaths )
	{
		fs::CPath commonDirPath = ExtractCommonParentPath( rPaths );
		return !commonDirPath.IsEmpty() ? StripDirPrefix( rPaths, commonDirPath.GetPtr() ) : 0;
	}


	template< typename ContainerT >
	inline void StripRootPrefix( ContainerT& rPaths )		// cut the drive letter or UNC server/share
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
