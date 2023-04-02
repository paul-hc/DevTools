
#include "pch.h"
#include "Path.h"
#include "FileState.h"
#include "FileSystem.h"
#include "Crc32.h"
#include "Algorithms.h"
#include "StringUtilities.h"
#include "StringIntuitiveCompare.h"
#include <io.h>
#include <unordered_map>
#include <shlwapi.h>				// for PathCombine


namespace func
{
	// Translates characters to generate a natural order, useful for sorting paths and filenames.
	// Natural order: intuitive (case insensitive, order numeric sequences by value) | punctuation first (shortest filename first in a tie).
	// Note: This is closer to Explorer.exe sort order (which varies with Windows version), yet different than ::StrCmpLogicalW from <shlwapi.h>
	//
	struct ToNaturalPathCharValue
	{
		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return static_cast<CharType>( Translate( ch ) );
		}
	private:
		static int Translate( wchar_t charCode );
	};
}


namespace path
{
	const std::tstring CDelims::s_dirDelims( _T("\\/") );
	const std::tstring CDelims::s_hugePrefix( _T("\\\\?\\") );		// "\\?\" without the C escape sequences
	const std::tstring CDelims::s_wildcards( _T("*?") );
	const std::tstring CDelims::s_multiSpecDelims( _T(";,") );
	const std::tstring CDelims::s_fmtNumSuffix( _T("_[%d]") );


	pred::CompareResult CompareIntuitive( const TCHAR* pLeft, const TCHAR* pRight )
	{
		return pred::MakeIntuitiveComparator( func::ToNaturalPathCharValue() ).Compare( pLeft, pRight );
	}


	size_t GetHashValuePtr( const TCHAR* pPath, size_t count /*= utl::npos*/ )
	{
		// compute hash value based on lower-case and normalized backslashes

		// inspired from template class instantiation 'template<> class hash<std::wstring>' from <functional> - hashing mechanism for std::unordered_map, std::unordered_set, etc.
		size_t hashValue = 2166136261u;
		size_t pos = 0;
		size_t lastPos = count != utl::npos ? count : str::GetLength( pPath );
		size_t stridePos = 1 + lastPos / 10;
		const func::ToEquivalentPathChar toEquivalentPathChar;

		if ( stridePos < lastPos )
			lastPos -= stridePos;

		for ( ; pos < lastPos; pos += stridePos )
			hashValue = 16777619u * hashValue ^ static_cast<size_t>( toEquivalentPathChar( pPath[ pos ] ) );

		return hashValue;
	}
}


namespace path
{
	// an efficient way (copy-on-write) of referring to the original path by pointer if it was originally normal, or a buffered normalized copy
	//
	class CWindowsPath
	{
	public:
		CWindowsPath( const TCHAR* pRawPath )
			: m_pRawPath( pRawPath )
			, m_isWindows( path::IsWindows( m_pRawPath ) )
		{
			if ( m_pRawPath != nullptr && !m_isWindows )
				EnsureBuffer();
		}

		const TCHAR* Get( void ) const { return m_normalizedPath.empty() ? m_pRawPath : m_normalizedPath.c_str(); }
		const TCHAR* GetRaw( void ) const { return m_pRawPath; }
		operator const TCHAR*( void ) const { return Get(); }

		bool IsWindows( void ) const { return m_isWindows; }

		TCHAR* Ref( void ) { return const_cast<TCHAR*>( EnsureBuffer().c_str() ); }

		std::tstring& EnsureBuffer( void )
		{
			if ( m_normalizedPath.empty() && !str::IsEmpty( m_pRawPath ) )
				str::Transform( m_pRawPath, std::back_inserter( m_normalizedPath ), func::ToWinPathChar() );

			return m_normalizedPath;
		}

		void NormalizeComplex( void ) { path::NormalizeComplexPath( EnsureBuffer() ); }
	private:
		const TCHAR* m_pRawPath;
		std::tstring m_normalizedPath;
		const bool m_isWindows;
	};


	struct CFnameExt
	{
		CFnameExt( const TCHAR* pFilename )
		{
			ASSERT_PTR( pFilename );
			_tcscpy( m_fname, pFilename );
			m_pExtCore = ::PathFindExtension( m_fname );
			if ( HasExt() )
			{
				*m_pExtCore = '\0';
				++m_pExtCore;
			}
		}

		bool HasFname( void ) const { return m_fname[ 0 ] != '\0'; }
		bool HasExt( void ) const { return *m_pExtCore != '\0'; }
		bool IsEmpty( void ) const { return !HasFname() && !HasExt(); }

		bool MatchWildcard( const CFnameExt& wild )
		{
			bool matchFname = wild.HasFname() ? ::PathMatchSpec( m_fname, wild.m_fname ) != FALSE : !HasFname();
			bool matchExt = wild.HasExt() ? ::PathMatchSpec( m_pExtCore, wild.m_pExtCore ) != FALSE : !HasExt();
			return matchFname && matchExt;
		}
	public:
		TCHAR m_fname[ MAX_PATH ];		// "fname.ext" -> "fname"
		TCHAR* m_pExtCore;				// "fname.ext" -> "ext"
	};
}


// this has no effect in VC2013; workaround in utl_ui_vc12.vcxproj: Solution Explorer > C++ > Advanced - Disable Specific Warnings: 4996
#pragma warning( disable: 4996 )	// 'std::equal': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'


namespace path
{
	const TCHAR* FindBreak( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );

		if ( '\0' == *pPath )
			return nullptr;

		while ( !IsBreakChar( *pPath ) )
			++pPath;

		return pPath;
	}

	const TCHAR* SkipBreak( const TCHAR* pPathBreak )
	{
		ASSERT( pPathBreak != nullptr && IsBreakChar( *pPathBreak ) );

		if ( '\0' == *pPathBreak )
			return nullptr;

		while ( *pPathBreak != '\0' && IsBreakChar( *pPathBreak ) )
			++pPathBreak;

		return pPathBreak;
	}

	bool MatchRootSegment( const TCHAR* pPath, const TCHAR* pSegment, size_t* pOutMatchLength = nullptr OUT )
	{
		// workaround for: PathIsSameRoot fails for comparing dots (".", "..")
		ASSERT_PTR( pPath );
		ASSERT_PTR( pSegment );

		if ( HasRoot( pPath ) || HasRoot( pSegment ) )		// must check for root match?
			if ( !HasSameRoot( pPath, pSegment ) )
				return false;

		utl::AssignPtr( pOutMatchLength, Range<const TCHAR*>( pPath, SkipRoot( pPath ) ).GetSpan<size_t>() );
		return true;
	}

	bool MatchSegment( const TCHAR* pPath, const TCHAR* pSegment, OUT size_t* pOutMatchLength /*= nullptr*/ )
	{
		ASSERT( pPath != nullptr && !IsBreakChar( *pPath ) );
		ASSERT( pSegment != nullptr && !IsBreakChar( *pSegment ) );

		const pred::TCharEqualEquivalentPath charEqual;
		size_t matchLength = 0;

		for ( ; !IsBreakChar( *pPath ); ++matchLength, ++pPath, ++pSegment )
			if ( !charEqual( *pPath, *pSegment ) )
				break;

		utl::AssignPtr( pOutMatchLength, matchLength );
		return IsBreakChar( *pPath ) && IsBreakChar( *pSegment );		// true if both break at end of match
	}


	bool DoMatchFilenameWildcard( const TCHAR* pFilename, const TCHAR* pWildSpec, const std::tstring& multiSpecDelims /*= CDelims::s_multiSpecDelims*/ )
	{
		if ( IsMultipleWildcard( pWildSpec, multiSpecDelims ) )
		{
			std::vector<TCHAR> wildcards;
			str::QuickTokenize( wildcards, pWildSpec, multiSpecDelims.c_str() );		// multiple zero-terminated items

			typedef const TCHAR* TConstIterator;

			for ( TConstIterator pWild = &wildcards.front(), pEnd = &wildcards.back() + 1; pWild != pEnd; ++pWild )
				if ( '\0' == *pWild )
					++pWild;						// skip empty specs, e.g. in ";x;;y"
				else if ( DoMatchFilenameWildcard( pFilename, pWild, multiSpecDelims ) )			// recursive call spec by spec
					return true;
				else
					pWild += str::GetLength( pWild );

			return false;
		}

		// ::PathMatchSpecEx( pFilename, pWildSpec, PMSF_MULTIPLE ) doesn't work well here: it matches partially as in Explorer search bar
		// for example it matches file _T("a") when wildcard is _T("a.txt")

		if ( ::PathMatchSpec( pFilename, pWildSpec ) )
			return true;

		if ( str::Equals<str::Case>( _T("*"), pWildSpec ) )
			return !str::IsEmpty( pFilename );

		// take care for special cases not covered by PathMatchSpecEx, such as "*." when must match only without extension
		CFnameExt path( pFilename ), wild( ::PathFindFileName( pWildSpec ) );
		return path.MatchWildcard( wild );
	}

	bool MatchWildcard( const TCHAR* pPath, const TCHAR* pWildSpec, const std::tstring& multiSpecDelims /*= CDelims::s_multiSpecDelims*/ )
	{
		return DoMatchFilenameWildcard( FindFilename( pPath ), pWildSpec, multiSpecDelims );
	}

	bool IsMultipleWildcard( const TCHAR* pWildSpec, const std::tstring& multiSpecDelims /*= CDelims::s_multiSpecDelims*/ )
	{
		const TCHAR* pWildEnd = str::end( pWildSpec );
		return std::find_first_of( pWildSpec, pWildEnd, multiSpecDelims.begin(), multiSpecDelims.end() ) != pWildEnd;
	}

	bool ContainsWildcards( const TCHAR* pWildSpec, const std::tstring& wildcards /*= CDelims::s_wildcards*/ )
	{
		const TCHAR* pWildEnd = str::end( pWildSpec );
		return std::find_first_of( pWildSpec, pWildEnd, wildcards.begin(), wildcards.end() ) != pWildEnd;
	}


	SpecMatch MatchesPrefix( const TCHAR* pFilePath, const TCHAR* pPrefixOrSpec )
	{
		if ( str::IsEmpty( pPrefixOrSpec ) )
			return Match_Any;		// no common prefix/spec filter

		fs::CPath filePath( pFilePath ), spec( pPrefixOrSpec );
		filePath.Normalize();
		spec.Normalize();

		if ( ::PathIsPrefix( spec.GetPtr(), filePath.GetPtr() ) )
			return Match_Prefix;

		if ( ::PathMatchSpec( filePath.GetPtr(), spec.GetPtr() ) )
			return Match_Spec;

		return NoMatch;
	}


	bool IsValid( const std::tstring& path )
	{
		return !path.empty() && std::tstring::npos == path.find_first_of( GetInvalidChars() );
	}

	const TCHAR* GetInvalidChars( void )
	{
		static const TCHAR s_invalidChars[] = _T("<>|?*\"");
		return s_invalidChars;
	}

	const TCHAR* GetReservedChars( void )
	{
		static const TCHAR s_reservedChars[] = _T(":/\\<>|?*\"");
		return s_reservedChars;
	}


	bool IsAbsolute( const TCHAR* pPath )
	{
		return !GetRootPath( pPath ).empty();
	}

	bool IsRelative( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsRelative( CWindowsPath( pPath ) ) != FALSE;
	}

	bool IsDirectory( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsDirectory( CWindowsPath( pPath ) ) != FALSE;
	}

	bool IsFilename( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsFileSpec( CWindowsPath( pPath ) ) != FALSE;
	}

	bool HasDirectory( const TCHAR* pPath )
	{
		return _tcspbrk( pPath, CDelims::s_dirDelims.c_str() ) != nullptr;
	}


	const TCHAR* Find( const TCHAR* pPath, const TCHAR* pSubString )
	{
		ASSERT_PTR( pPath );

		return std::search( str::begin( pPath ), str::end( pPath ),
							str::begin( pSubString ), str::end( pSubString ),
							pred::TCharEqualEquivalentPath() );
	}

	size_t FindPos( const TCHAR* pPath, const TCHAR* pSubString, size_t offset /*= 0*/ )
	{
		REQUIRE( offset < str::GetLength( pPath ) );
		const TCHAR* pFound = Find( pPath + offset, pSubString );
		ASSERT_PTR( pFound );
		return *pFound != '\0' ? std::distance( pPath, pFound ) : utl::npos;
	}

	const TCHAR* FindFilename( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathFindFileName( GetSubPath( pPath ) );
	}

	const TCHAR* FindExt( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathFindExtension( pPath );
	}

	const TCHAR* SkipRoot( const TCHAR* pPath )
	{
		CWindowsPath pathCopy( pPath );

		if ( const TCHAR* pRootEnd = ::PathSkipRoot( pathCopy ) )
			return pPath + std::distance( pathCopy.Get(), pRootEnd );

		return pPath;
	}

	bool MatchAnyExt( const TCHAR* pPath, const TCHAR* pMultiExt, TCHAR sepChr /*= '|'*/ )
	{
		const TCHAR sep[] = { sepChr, '\0' };
		std::vector<std::tstring> extensions;

		str::Split( extensions, pMultiExt, sep );
		return utl::Any( extensions, pred::HasExtension( pPath ) );
	}


	// huge prefix syntax: path prefixed with "\\?\"

	const TCHAR* GetStart( const TCHAR* pPath )
	{
		if ( HasHugePrefix( pPath ) )
			return pPath + CDelims::s_hugePrefix.length();

		return pPath;
	}

	bool AutoHugePrefix( IN OUT std::tstring& rPath )
	{
		bool isHuge = rPath.length() >= MAX_PATH;

		if ( isHuge == HasHugePrefix( rPath.c_str() ) )
			return false;		// no change

		if ( isHuge )
			rPath.insert( 0, CDelims::s_hugePrefix );
		else
			str::StripPrefix( rPath, STRING_SPAN( CDelims::s_hugePrefix ) );

		return true;
	}


	// complex path

	bool IsWellFormed( const TCHAR* pFilePath )
	{
		return std::count( str::begin( pFilePath ), str::end( pFilePath ), CDelims::s_complexPathSep ) <= 1;		// ensure at most single complex separator
	}

	std::tstring ExtractPhysical( const std::tstring& filePath )
	{
		size_t sepPos = FindComplexSepPos( filePath.c_str() );
		if ( std::tstring::npos == sepPos )
			return filePath;

		return filePath.substr( 0, sepPos );
	}

	const TCHAR* GetEmbedded( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		pPath = std::find( str::begin( pPath ), str::end( pPath ), CDelims::s_complexPathSep );
		return *pPath != '\0' ? ( pPath + 1 ) : pPath;
	}

	const TCHAR* GetSubPath( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		const TCHAR* pComplexSep = std::find( str::begin( pPath ), str::end( pPath ), CDelims::s_complexPathSep );
		return *pComplexSep != '\0' ? ( pComplexSep + 1 ) : pPath;
	}

	std::tstring MakeComplex( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath )
	{
		ASSERT( !physicalPath.empty() );
		ASSERT( !str::IsEmpty( pEmbeddedPath ) );
		ASSERT( utl::npos == FindComplexSepPos( physicalPath.c_str() ) );
		ASSERT( utl::npos == FindComplexSepPos( pEmbeddedPath ) );

		return physicalPath + CDelims::s_complexPathSep + pEmbeddedPath;
	}

	bool SplitComplex( OUT std::tstring& rPhysicalPath, OUT std::tstring& rEmbeddedPath, const std::tstring& filePath )
	{
		size_t sepPos = FindComplexSepPos( filePath.c_str() );
		if ( std::tstring::npos == sepPos )
		{
			rPhysicalPath = filePath;
			rEmbeddedPath.clear();
			return false;
		}

		rPhysicalPath = filePath.substr( 0, sepPos );
		rEmbeddedPath = filePath.substr( sepPos + 1 );
		return true;
	}

	bool NormalizeComplexPath( IN OUT std::tstring& rFlexPath, TCHAR chNormalSep /*= '\\'*/ )
	{	// treat storage document in rFlexPath as a directory
		size_t sepPos = FindComplexSepPos( rFlexPath.c_str() );
		if ( std::tstring::npos == sepPos )
			return false;

		rFlexPath[ sepPos ] = chNormalSep;
		return true;
	}


	std::tstring& SetBackslash( IN OUT std::tstring& rDirPath, TrailSlash trailSlash /*= AddSlash*/ )
	{
		if ( PreserveSlash == trailSlash )
			return rDirPath;

		std::vector<TCHAR> buffer;
		buffer.reserve( rDirPath.length() + 1 + 1 );				// make room for extra-slash and terminating zero
		buffer.assign( rDirPath.c_str(), rDirPath.c_str() + rDirPath.length() + 1 );

		if ( !rDirPath.empty() )
		{
			TCHAR& rTrailChar = buffer[ rDirPath.length() - 1 ];
			rTrailChar = func::ToWinPathChar()( rTrailChar );		// just normalize last slash to make it compatible with the shell API
		}

		if ( AddSlash == trailSlash ? ::PathAddBackslash( &buffer.front() ) : ::PathRemoveBackslash( &buffer.front() ) )
			if ( !Equivalent( rDirPath.c_str(), &buffer.front() ) )		// only if changed
				rDirPath = &buffer.front();

		return rDirPath;
	}

	bool HasTrailingSlash( const TCHAR* pPath )
	{	// true for non-root paths with a trailing slash
		return
			!IsRoot( pPath ) &&
			IsSlash( pPath[ str::GetLength( pPath ) - 1 ] );
	}

	std::tstring GetParentPath( const TCHAR* pPath, TrailSlash trailSlash /*= PreserveSlash*/ )
	{
		const TCHAR* pFilename = FindFilename( pPath );
		ASSERT_PTR( pFilename );

		std::tstring dirPath( pPath, pFilename - pPath );

		SetBackslash( dirPath, trailSlash );

		if ( RemoveSlash == trailSlash )
			str::StripSuffix( dirPath, &CDelims::s_complexPathSep, 1 );		// flex paths: treat trailing '>' like a slash - remove it

		return dirPath;
	}


	bool IsWindows( const TCHAR* pPath )
	{
		return !str::Contains( pPath, '/' );
	}

	std::tstring MakeWindows( const TCHAR* pPath )
	{
		std::tstring windowsPath;

		str::Transform( pPath, std::back_inserter( windowsPath ), func::ToWinPathChar() );
		str::Trim( windowsPath );
		return windowsPath;
	}

	std::tstring MakeCanonical( const TCHAR* pPath )
	{
		std::tstring absolutePath = MakeWindows( pPath );		// PathCanonicalize works only with backslashes
		TCHAR fullPath[ MAX_PATH ];
		if ( ::PathCanonicalize( fullPath, absolutePath.c_str() ) )
			absolutePath = fullPath;
		return absolutePath;
	}

	std::tstring Combine( const TCHAR* pDirPath, const TCHAR* pRightPath )
	{
		std::tstring dirPath = MakeWindows( pDirPath ), rightPath = MakeWindows( pRightPath );		// PathCombine works only with backslashes

		TCHAR fullPath[ MAX_PATH ];
		if ( nullptr == ::PathCombine( fullPath, dirPath.c_str(), rightPath.c_str() ) )
			return _T("<bad PathCombine>");
		return fullPath;
	}


	bool IsRoot( const TCHAR* pPath )
	{
		return ::PathIsRoot( CWindowsPath( pPath ).Get() ) != FALSE;
	}

	bool IsNetwork( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsNetworkPath( pPath ) != FALSE;
	}

	bool IsURL( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsURL( pPath ) != FALSE;
	}

	bool IsUNC( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsUNC( pPath ) != FALSE;
	}

	bool IsUNCServer( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsUNCServer( pPath ) != FALSE;
	}

	bool IsUNCServerShare( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsUNCServerShare( pPath ) != FALSE;
	}


	bool HasPrefix( const TCHAR* pPath, const TCHAR* pPrefix )
	{
		ASSERT_PTR( pPath );
		ASSERT_PTR( pPrefix );

		if ( str::IsEmpty( pPrefix ) )
			return false;							// no match of empty prefix

		if ( ::PathIsPrefix( pPrefix, pPath ) )		// this fails for dots matching
			return true;

		// handle case of non-normalized/complex paths
		size_t matchLen = FindCommonPrefixLength( pPath, pPrefix );
		size_t prefixLen = str::GetLength( pPrefix );

		if ( IsBreakChar( pPrefix[ prefixLen - 1 ] ) )
			--prefixLen;

		return prefixLen == matchLen;
	}

	bool MatchPrefix( const TCHAR* pPath, const TCHAR* pPrefix )
	{
		return !str::IsEmpty( pPrefix ) && pPath == Find( pPath, pPrefix );
	}

	bool HasRoot( const TCHAR* pPath )
	{
		return SkipRoot( pPath ) != pPath;
	}

	bool HasSameRoot( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		ASSERT( IsWindows( pLeftPath ) );
		ASSERT( IsWindows( pRightPath ) );

		return ::PathIsSameRoot( pLeftPath, pRightPath ) != FALSE;
	}

	std::tstring GetRootPath( const TCHAR* pPath )
	{
		CWindowsPath rootPath( pPath );

		::PathStripToRoot( rootPath.Ref() );
		return rootPath.Ref();
	}


	size_t FindCommonPrefixLength( const TCHAR* pPath, const TCHAR* pPrefix )
	{
		ASSERT_PTR( pPath );
		ASSERT_PTR( pPrefix );

		if ( str::IsEmpty( pPrefix ) )
			return 0;

		std::tstring path = MakeWindows( pPath );
		std::tstring prefix = MakeWindows( pPrefix );

		typedef const TCHAR* TConstIterator;
		TConstIterator itPath = path.c_str();
		TConstIterator itPrefix = prefix.c_str();

		Range<const TCHAR*> commonRange( itPath );

		size_t segmentLen;

		if ( MatchRootSegment( itPath, itPrefix, &segmentLen ) )
		{
			itPath += segmentLen;
			itPrefix += segmentLen;
			commonRange.m_end = itPath;			// include the root segment

			while ( !str::IsEmpty( itPath ) && !str::IsEmpty( itPrefix ) )
			{
				if ( !MatchSegment( itPath, itPrefix, &segmentLen ) )
					break;

				itPath += segmentLen;
				commonRange.m_end = itPath;

				itPath = SkipBreak( itPath );
				itPrefix = SkipBreak( itPrefix + segmentLen );
			}
		}

		return commonRange.GetSpan<size_t>();
	}

	std::tstring FindCommonPrefix( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		size_t commonLength = FindCommonPrefixLength( pLeftPath, pRightPath );
		std::tstring commonPrefix( pLeftPath, pLeftPath + commonLength );

		return commonPrefix;
	}

	std::tstring _FindCommonPrefix( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		size_t complSepPos = FindComplexSepPos( pLeftPath );
		std::tstring leftPath = MakeWindows( pLeftPath );		// backslashes only
		std::tstring rightPath = MakeWindows( pRightPath );

		// ensure it works seamlessly with deep embedded paths (stoage doc treated as directory)
		NormalizeComplexPath( leftPath );
		NormalizeComplexPath( rightPath );

		TCHAR commonPrefix[ MAX_PATH ] = _T("");
		::PathCommonPrefix( leftPath.c_str(), rightPath.c_str(), commonPrefix );

		// restore the complex path separator if it's still part of the resulting common prefix
		if ( complSepPos != std::tstring::npos && complSepPos < str::GetLength( commonPrefix ) )
			commonPrefix[ complSepPos ] = CDelims::s_complexPathSep;

		return commonPrefix;
	}

	std::tstring StripCommonPrefix( const TCHAR* pFullPath, const TCHAR* pDirPath )
	{
		std::tstring restPath = pFullPath;

		if ( !str::IsEmpty( pDirPath ) )
		{
			size_t commonLen = func::TCompareEquivalentPath().Compare( pFullPath, pDirPath ).second;
			if ( commonLen != 0 )
			{
				if ( IsSlash( pFullPath[ commonLen ] ) )
					++commonLen;								// exclude leading slash if pDirPath doesn't end in "\\"

				restPath = restPath.substr( commonLen );
			}
		}

		return restPath;
	}

	bool StripPrefix( std::tstring& rPath, const TCHAR* pPrefix )
	{
		if ( str::IsEmpty( pPrefix ) )
			return true;					// nothing to strip, not an error

		if ( size_t prefixLen = str::GetLength( pPrefix ) )
			if ( pred::Equal == CompareEquivalent( rPath.c_str(), pPrefix, prefixLen ) )
			{
				TCHAR chNext = rPath.c_str()[ prefixLen ];

				if ( IsSlash( chNext ) || CDelims::s_complexPathSep == chNext )
					++prefixLen;			// cut leading slash or '>'

				rPath.erase( 0, prefixLen );
				return true;				// changed
			}

		return false;
	}

} //namespace path


namespace fs
{
	bool FileExist( const TCHAR* pFilePath, AccessMode accessMode /*= Exist*/ )
	{
		return !str::IsEmpty( pFilePath ) && 0 == ::_taccess( pFilePath, accessMode );
	}


	// CPathParts implementation

	void CPathParts::Clear( void )
	{
		m_drive.clear();
		m_dir.clear();
		m_fname.clear();
		m_ext.clear();
	}

	void CPathParts::SplitPath( const std::tstring& filePath )
	{
		TCHAR drive[ _MAX_DRIVE ], dir[ _MAX_DIR ], fname[ _MAX_FNAME ], ext[ _MAX_EXT ];
		size_t sepPos = std::tstring::npos;

		if ( !path::IsComplex( filePath.c_str() ) )
			_tsplitpath( filePath.c_str(), drive, dir, fname, ext );
		else
		{
			ASSERT( path::IsWellFormed( filePath.c_str() ) );

			std::tstring tempPath = filePath;
			sepPos = path::FindComplexSepPos( tempPath.c_str() );
			tempPath[ sepPos ] = '/';			// temporary make it part of the dir path

			_tsplitpath( tempPath.c_str(), drive, dir, fname, ext );
		}

		m_drive = drive;
		m_dir = dir;
		m_fname = fname;
		m_ext = ext;

		if ( sepPos != std::tstring::npos )
		{
			sepPos -= m_drive.length();
			ASSERT( '/' == m_dir[ sepPos ] );	// restore to original
			m_dir[ sepPos ] = path::CDelims::s_complexPathSep;
		}
	}

	std::tstring CPathParts::MakeFullPath( const TCHAR* pDrive, const TCHAR* pDir, const TCHAR* pFname, const TCHAR* pExt )
	{
		TCHAR path[ MAX_PATH * 2 ];
		_tmakepath( path, pDrive, pDir, pFname, pExt );
		return path;
	}

	fs::CPath CPathParts::GetDirPath( void ) const
	{
		fs::CPath dirPath = m_drive;
		dirPath /= m_dir;
		dirPath.SetBackslash( false );
		return dirPath;
	}

	CPathParts& CPathParts::SetFilename( const std::tstring& filename )
	{
		CPathParts parts( filename );
		m_fname = parts.m_fname;
		m_ext = parts.m_ext;
		return *this;
	}

	CPathParts& CPathParts::SetDirPath( const std::tstring& dirPath )
	{
		std::tstring newDirPath = dirPath;
		CPathParts parts( path::SetBackslash( newDirPath, true ) );
		m_drive = parts.m_drive;
		m_dir = parts.m_dir;
		return *this;
	}

	fs::CPath CPathParts::MakePath( void ) const
	{
		TCHAR path[ MAX_PATH * 2 ];
		size_t sepPos = std::tstring::npos;

		if ( !path::IsComplex( m_dir.c_str() ) )
			_tmakepath( path, m_drive.c_str(), m_dir.c_str(), m_fname.c_str(), m_ext.c_str() );
		else
		{
			ASSERT( path::IsWellFormed( m_dir.c_str() ) );

			std::tstring tempDir = m_dir;
			sepPos = path::FindComplexSepPos( tempDir.c_str() );
			tempDir[ sepPos ] = '/';			// temporary make it look like a real dir

			_tmakepath( path, m_drive.c_str(), tempDir.c_str(), m_fname.c_str(), m_ext.c_str() );

			sepPos += m_drive.length();
			ASSERT( '/' == path[ sepPos ] );	// restore to original
			path[ sepPos ] = path::CDelims::s_complexPathSep;
		}

		return fs::CPath( path );
	}


	// CPath implementation

	bool CPath::operator<( const CPath& right ) const
	{
		return pred::TLess_NaturalPath()( *this, right );
	}

	void CPath::Set( const std::tstring& filePath )
	{
		m_filePath = filePath;
		str::Trim( m_filePath );
		path::AutoHugePrefix( m_filePath );
	}

	size_t CPath::GetDepth( void ) const
	{
		size_t depth = 0;

		fs::TDirPath parentPath = *this;
		parentPath.Normalize();

		while ( !parentPath.IsEmpty() )
		{
			++depth;

			fs::TDirPath newParentPath = parentPath.GetParentPath();

			if ( newParentPath != parentPath )
				parentPath = newParentPath;
			else
				break;
		}

		return depth;
	}


	TDirPath CPath::GetParentPath( bool trailSlash /*= false*/ ) const
	{
		ASSERT( !IsEmpty() );
		return path::GetParentPath( GetPtr(), trailSlash ? path::AddSlash : path::RemoveSlash );
	}

	CPath& CPath::SetBackslash( bool trailSlash /*= true*/ )
	{
		path::SetBackslash( m_filePath, trailSlash ? path::AddSlash : path::RemoveSlash );
		return *this;
	}

	void CPath::SetFilename( const std::tstring& filename )
	{
		CPathParts parts( m_filePath );			// split into parts rather than use '/' operator in order to preserve fwd slashes
		parts.SetFilename( filename );
		m_filePath = parts.MakePath().Get();
	}

	void CPath::SplitFilename( std::tstring& rFname, std::tstring& rExt ) const
	{
		rFname = GetFname();
		rExt = GetExt();
	}

	std::tstring CPath::GetFname( void ) const
	{
		const TCHAR* pNameExt = GetFilenamePtr();
		const TCHAR* pExt = GetExt();

		return std::tstring( pNameExt, std::distance( pNameExt, pExt ) );
	}


	namespace impl
	{
		size_t GetDotExtCount( const fs::CPath& filePath )
		{
			const TCHAR* pNameExt = filePath.GetFilenamePtr();
			return std::count( pNameExt, pNameExt + str::GetLength( pNameExt ), '.' );
		}
	}

	ExtensionMatch CPath::GetExtensionMatch( const fs::CPath& right ) const
	{
		if ( impl::GetDotExtCount( *this ) != impl::GetDotExtCount( right ) )
			return MismatchDotsExt;
		else if ( !ExtEquals( right.GetExt() ) )		// ignore case changes
			return MismatchExt;
		else if ( std::tstring( GetExt() ) != std::tstring( right.GetExt() ) )
			return MatchDiffCaseExt;

		return MatchExt;
	}

	CPath CPath::GetRemoveExt( void ) const
	{
		size_t extPos = std::distance( GetPtr(), GetExt() );
		return CPath( m_filePath.substr( 0, extPos ) );
	}

	void CPath::SetDirPath( const std::tstring& dirPath )
	{
		CPathParts parts( m_filePath );
		parts.SetDirPath( dirPath );
		m_filePath = parts.MakePath().Get();
	}

	CPath& CPath::operator/=( const CPath& right )
	{
		if ( &right != this )
			Set( path::Combine( GetPtr(), right.GetPtr() ) );
		return *this;
	}

	bool CPath::LocateFile( CFileFind& rFindFile ) const
	{
		rFindFile.Close();

		return
			rFindFile.FindFile( GetPtr() ) != FALSE &&
			rFindFile.FindNextFile() != FALSE;
	}

	CPath CPath::LocateExistingFile( void ) const
	{
		ASSERT( FileExist() );

		CFileFind finder;
		VERIFY( finder.FindFile( m_filePath.c_str() ) );
		finder.FindNextFile();

		return CPath( finder.GetFilePath().GetString() );
	}

} //namespace fs


namespace fs
{
	fs::CPath GetShortFilePath( const fs::CPath& filePath )
	{
		TCHAR shortPath[ MAX_PATH ];
		::GetShortPathName( filePath.GetPtr(), shortPath, COUNT_OF( shortPath ) );			// convert to short path

		return fs::CPath( shortPath );
	}

	fs::CPath GetLongFilePath( const fs::CPath& filePath )
	{
		TCHAR longPath[ MAX_PATH ];
		::GetLongPathName( filePath.GetPtr(), longPath, COUNT_OF( longPath ) );				// convert to long path

		return fs::CPath( longPath );
	}


	fs::PatternResult SplitPatternPath( fs::CPath* pPath, std::tstring* pWildSpec, const fs::TPatternPath& patternPath )
	{
		REQUIRE( pPath != nullptr && pWildSpec != nullptr );

		if ( path::ContainsWildcards( patternPath.GetFilenamePtr() ) )
		{
			*pPath = patternPath.GetParentPath();
			*pWildSpec = patternPath.GetFilename();
		}
		else
		{
			*pPath = patternPath;
			pWildSpec->clear();
		}

		if ( fs::IsValidFile( pPath->GetPtr() ) )
			return fs::ValidFile;
		else if ( fs::IsValidDirectory( pPath->GetPtr() ) )
			return fs::ValidDirectory;

		return fs::InvalidPattern;
	}
}


namespace path
{
	fs::CPath StripWildcards( const fs::CPath& patternPath )
	{
		if ( path::ContainsWildcards( patternPath.GetFilenamePtr() ) )
			return patternPath.GetParentPath();

		return patternPath;
	}
}


namespace pred
{
	struct ComparePathDepth
	{
		ComparePathDepth( const std::unordered_map<fs::CPath, size_t>& rFilePathsToDepth ) : m_rFilePathsToDepth( rFilePathsToDepth ) {}

		CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			size_t leftDepth = utl::FindValue( m_rFilePathsToDepth, leftPath ), rightDepth = utl::FindValue( m_rFilePathsToDepth, rightPath );

			CompareResult result = Compare_Scalar( leftDepth, rightDepth );
			if ( Equal == result )
				result = m_comparePath( leftPath, rightPath );

			return result;
		}
	private:
		const std::unordered_map<fs::CPath, size_t>& m_rFilePathsToDepth;
		pred::CompareNaturalPath m_comparePath;
	};


	pred::CompareResult CompareNaturalPath::operator()( const fs::CFileState& leftState, const fs::CFileState& rightState ) const
	{
		return operator()( leftState.m_fullPath, rightState.m_fullPath );
	}
}


namespace fs
{
	void SortByPathDepth( std::vector<fs::CPath>& rFilePaths, bool ascending /*= true*/ )
	{
		std::unordered_map<fs::CPath, size_t> filePathsToDepth;

		for ( std::vector<fs::CPath>::const_iterator itFilePath = rFilePaths.begin(); itFilePath != rFilePaths.end(); ++itFilePath )
			filePathsToDepth[ *itFilePath ] = itFilePath->GetDepth();

		std::sort( rFilePaths.begin(), rFilePaths.end(), pred::MakeOrderByValue( pred::ComparePathDepth( filePathsToDepth ), ascending ) );
	}
}


namespace path
{
	void QueryParentPaths( std::vector<fs::CPath>& rParentPaths, const std::vector<fs::CPath>& filePaths, bool uniqueOnly /*= true*/ )
	{
		for ( std::vector<fs::CPath>::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
		{
			fs::CPath parentPath = itFilePath->GetParentPath();
			if ( !parentPath.IsEmpty() )
				if ( uniqueOnly )
					utl::AddUnique( rParentPaths, parentPath );
				else
					rParentPaths.push_back( parentPath );
		}
	}
}


namespace func
{
	int ToNaturalPathCharValue::Translate( wchar_t charCode )
	{
		enum TranslatedCode		// natural order for punctuation characters
		{
			Dot = 1, Colon, SemiColon, Comma, Dash, Plus, Underbar,
			CurvedBraceB, CurvedBraceE,
			SquareBraceB, SquareBraceE,
			CurlyBraceB, CurlyBraceE,
			AngularBraceB, AngularBraceE
		};

		switch ( charCode )
		{
			case '.':	return Dot;
			case ':':	return Colon;
			case ';':	return SemiColon;
			case ',':	return Comma;
			case '-':	return Dash;
			case '+':	return Plus;
			case '_':	return Underbar;
			case '(':	return CurvedBraceB;
			case ')':	return CurvedBraceE;
			case '[':	return SquareBraceB;
			case ']':	return SquareBraceE;
			case '{':	return CurlyBraceB;
			case '}':	return CurlyBraceE;
			case '<':	return AngularBraceB;
			case '>':	return AngularBraceE;
		}
		return charCode;
	}
}
