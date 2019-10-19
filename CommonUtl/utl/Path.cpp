
#include "stdafx.h"
#include "Path.h"
#include "FileSystem.h"
#include "Crc32.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"
#include "StringIntuitiveCompare.h"
#include <xhash>
#include <hash_map>
#include <shlwapi.h>				// for PathCombine


namespace func
{
	int ToNaturalPathChar::Translate( int charCode )
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


// this has no effect in VC2013; workaround in utl_ui_vc12.vcxproj: Solution Explorer > C++ > Advanced - Disable Specific Warnings: 4996
#pragma warning( disable: 4996 )	// 'std::equal': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'


namespace path
{
	const TCHAR* DirDelims( void )
	{
		static const TCHAR dirDelims[] = _T("\\/");
		return dirDelims;
	}

	bool EquivalentPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		ASSERT( pLeftPath != NULL && pRightPath != NULL );
		if ( pLeftPath == pRightPath )
			return true;

		return
			str::GetLength( pLeftPath ) == str::GetLength( pRightPath ) &&
			std::equal( str::begin( pLeftPath ), str::end( pLeftPath ), str::begin( pRightPath ), pred::IsEquivalentPathChar() );
	}

	bool Equivalent( const std::tstring& leftPath, const std::tstring& rightPath )
	{
		if ( &leftPath == &rightPath )
			return true;

		return
			leftPath.length() == rightPath.length() &&
			std::equal( leftPath.begin(), leftPath.end(), rightPath.begin(), pred::IsEquivalentPathChar() );
	}

	bool EqualsPtr( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		return str::Equals< str::IgnoreCase >( pLeftPath, pRightPath );
	}

	pred::CompareResult CompareNaturalPtr( const TCHAR* pLeft, const TCHAR* pRight )
	{
		return pred::MakeIntuitiveComparator( func::ToNaturalPathChar() ).Compare( pLeft, pRight );
	}


	size_t GetHashValue( const TCHAR* pPath )
	{
		// compute hash value based on lower-case and normalized backslashes

		// inspired from template function in <xhash> for hash of range of elements:
		//	template< class InIt >
		//	size_t stdext::_Hash_value( InIt _Begin, InIt _End )

		size_t hashValue = 2166136261U;

		for ( const TCHAR* pCh = pPath; *pCh != _T('\0'); ++pCh )
			hashValue = 16777619U * hashValue ^ static_cast< size_t >( ToEquivalentChar( *pCh ) );

		return hashValue;
	}


/*	str::Match GetMatch( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		if ( pred::Equal == str::_CompareN( pLeftPath, pRightPath, &path::ToNormalChar ) )			// case sensitive
			return str::MatchEqual;
		if ( pred::Equal == str::_CompareN( pLeftPath, pRightPath, &path::ToEquivalentChar ) )		// case insensitive
			return str::MatchEqualDiffCase;
		return str::MatchNotEqual;
	}*/


	struct CFnameExt
	{
		CFnameExt( const TCHAR* pFilename )
		{
			ASSERT_PTR( pFilename );
			_tcscpy( m_fname, pFilename );
			m_pExtCore = ::PathFindExtension( m_fname );
			if ( HasExt() )
			{
				*m_pExtCore = _T('\0');
				++m_pExtCore;
			}
		}

		bool HasFname( void ) const { return m_fname[ 0 ] != _T('\0'); }
		bool HasExt( void ) const { return *m_pExtCore != _T('\0'); }
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


	const TCHAR* Wildcards( void )
	{
		return _T("*?");
	}

	const TCHAR* MultiSpecDelims( void )
	{
		return _T(";,");
	}

	bool DoMatchFilenameWildcard( const TCHAR* pFilename, const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims )
	{
		if ( IsMultipleWildcard( pWildSpec, pMultiSpecDelims ) )
		{
			std::vector< TCHAR > wildcards;
			str::QuickTokenize( wildcards, pWildSpec, pMultiSpecDelims );		// multiple zero-terminated items

			for ( str::const_iterator pWild = &wildcards.front(), pEnd = &wildcards.back() + 1; pWild != pEnd; ++pWild )
				if ( _T('\0') == *pWild )
					++pWild;						// skip empty specs, e.g. in ";x;;y"
				else if ( DoMatchFilenameWildcard( pFilename, pWild, pMultiSpecDelims ) )			// recursive call spec by spec
					return true;
				else
					pWild += str::GetLength( pWild );

			return false;
		}

		// ::PathMatchSpecEx( pFilename, pWildSpec, PMSF_MULTIPLE ) doesn't work well here: it matches partially as in Explorer search bar
		// for example it matches file _T("a") when wildcard is _T("a.txt")

		if ( ::PathMatchSpec( pFilename, pWildSpec ) )
			return true;

		if ( str::Equals< str::Case >( _T("*"), pWildSpec ) )
			return !str::IsEmpty( pFilename );

		// take care for special cases not covered by PathMatchSpecEx, such as "*." when must match only without extension
		CFnameExt path( pFilename ), wild( ::PathFindFileName( pWildSpec ) );
		return path.MatchWildcard( wild );
	}

	bool MatchWildcard( const TCHAR* pPath, const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims /*= MultiSpecDelims()*/ )
	{
		return DoMatchFilenameWildcard( FindFilename( pPath ), pWildSpec, pMultiSpecDelims );
	}

	bool IsMultipleWildcard( const TCHAR* pWildSpec, const TCHAR* pMultiSpecDelims /*= MultiSpecDelims()*/ )
	{
		const TCHAR* pWildEnd = str::end( pWildSpec );
		return std::find_first_of( pWildSpec, pWildEnd, pMultiSpecDelims, str::end( pMultiSpecDelims ) ) != pWildEnd;
	}

	bool ContainsWildcards( const TCHAR* pWildSpec, const TCHAR* pWildcards /*= Wildcards()*/ )
	{
		const TCHAR* pWildEnd = str::end( pWildSpec );
		return std::find_first_of( pWildSpec, pWildEnd, pWildcards, str::end( pWildcards ) ) != pWildEnd;
	}


	SpecMatch MatchesPrefix( const TCHAR* pFilePath, const TCHAR* pPrefixOrSpec )
	{
		if ( str::IsEmpty( pPrefixOrSpec ) )
			return Match_Any;		// no common prefix/spec filter

		fs::CPath filePath = pFilePath, spec = pPrefixOrSpec;
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
		static const TCHAR invalidChars[] = _T("<>|?*\"");
		return invalidChars;
	}

	const TCHAR* GetReservedChars( void )
	{
		static const TCHAR reservedChars[] = _T(":/\\<>|?*\"");
		return reservedChars;
	}


	bool IsAbsolute( const TCHAR* pPath )
	{
		return !GetRootPath( pPath ).empty();
	}

	bool IsRelative( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsRelative( pPath ) != FALSE;
	}

	bool IsDirectory( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsDirectory( pPath ) != FALSE;
	}

	bool IsNameExt( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsFileSpec( pPath ) != FALSE;
	}


	const TCHAR* Find( const TCHAR* pPath, const TCHAR* pSubString )
	{
		ASSERT_PTR( pPath );

		return std::search( str::begin( pPath ), str::end( pPath ),
							str::begin( pSubString ), str::end( pSubString ),
							pred::IsEquivalentPathChar() );
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


	// complex path

	const TCHAR s_complexPathSep = _T('>');

	bool IsWellFormed( const TCHAR* pFilePath )
	{
		return std::count( str::begin( pFilePath ), str::end( pFilePath ), s_complexPathSep ) <= 1;		// ensure at most single complex separator
	}

	size_t FindComplexSepPos( const TCHAR* pPath )
	{
		return utl::FindPos( str::begin( pPath ), str::end( pPath ), s_complexPathSep );
	}

	std::tstring GetPhysical( const std::tstring& filePath )
	{
		size_t sepPos = FindComplexSepPos( filePath.c_str() );
		if ( std::tstring::npos == sepPos )
			return filePath;

		return filePath.substr( 0, sepPos );
	}

	const TCHAR* GetEmbedded( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		pPath = std::find( str::begin( pPath ), str::end( pPath ), s_complexPathSep );
		return *pPath != _T('\0') ? ( pPath + 1 ) : pPath;
	}

	const TCHAR* GetSubPath( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		const TCHAR* pComplexSep = std::find( str::begin( pPath ), str::end( pPath ), s_complexPathSep );
		return *pComplexSep != _T('\0') ? ( pComplexSep + 1 ) : pPath;
	}

	std::tstring MakeComplex( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath )
	{
		ASSERT( !physicalPath.empty() );
		ASSERT( !str::IsEmpty( pEmbeddedPath ) );
		ASSERT( utl::npos == FindComplexSepPos( physicalPath.c_str() ) );
		ASSERT( utl::npos == FindComplexSepPos( pEmbeddedPath ) );

		return physicalPath + s_complexPathSep + pEmbeddedPath;
	}

	bool SplitComplex( std::tstring& rPhysicalPath, std::tstring& rEmbeddedPath, const std::tstring& filePath )
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


	std::tstring& SetBackslash( std::tstring& rDirPath, TrailSlash trailSlash /*= AddSlash*/ )
	{
		if ( PreserveSlash == trailSlash /*|| IsComplex( rDirPath.c_str() )*/ )		// if complex path leave it alone
			return rDirPath;

		TCHAR dirPath[ _MAX_PATH ];
		str::Copy( dirPath, rDirPath );
		if ( !rDirPath.empty() )
		{	// just normalize last slash to make it compatible with the API
			TCHAR* pLastCh = dirPath + rDirPath.length() - 1;
			*pLastCh = ToNormalChar( *pLastCh );
		}

		if ( AddSlash == trailSlash ? ::PathAddBackslash( dirPath ) : ::PathRemoveBackslash( dirPath ) )
			if ( !Equivalent( rDirPath, dirPath ) )		// only if changed
				rDirPath = dirPath;

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
		return dirPath;
	}


	std::tstring MakeNormal( const TCHAR* pPath )
	{
		std::tstring normalPath = pPath;
		str::Trim( normalPath );
		std::replace( normalPath.begin(), normalPath.end(), _T('/'), _T('\\') );
		return normalPath;
	}

	std::tstring MakeCanonical( const TCHAR* pPath )
	{
		std::tstring absolutePath = MakeNormal( pPath );		// PathCanonicalize works only with backslashes
		TCHAR fullPath[ _MAX_PATH ];
		if ( ::PathCanonicalize( fullPath, absolutePath.c_str() ) )
			absolutePath = fullPath;
		return absolutePath;
	}

	std::tstring Combine( const TCHAR* pDirPath, const TCHAR* pRightPath )
	{
		std::tstring dirPath = MakeNormal( pDirPath ), rightPath = MakeNormal( pRightPath );		// PathCombine works only with backslashes

		TCHAR fullPath[ _MAX_PATH ];
		if ( NULL == ::PathCombine( fullPath, dirPath.c_str(), rightPath.c_str() ) )
			return _T("<bad PathCombine>");
		return fullPath;
	}


	bool IsRoot( const TCHAR* pPath )
	{
		ASSERT_PTR( pPath );
		return ::PathIsRoot( pPath ) != FALSE;
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
		ASSERT_PTR( pPath ); ASSERT_PTR( pPrefix );
		return ::PathIsPrefix( pPrefix, pPath ) != FALSE;
	}

	bool MatchPrefix( const TCHAR* pPath, const TCHAR* pPrefix )
	{
		return !str::IsEmpty( pPrefix ) && pPath == Find( pPath, pPrefix );
	}

	bool HasSameRoot( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		ASSERT_PTR( pLeftPath ); ASSERT_PTR( pRightPath );
		return ::PathIsSameRoot( pLeftPath, pRightPath ) != FALSE;
	}

	std::tstring GetRootPath( const TCHAR* pPath )
	{
		TCHAR rootPath[ _MAX_PATH ];
		_tcscpy( rootPath, pPath );
		::PathStripToRoot( rootPath );
		return rootPath;
	}

	std::tstring FindCommonPrefix( const TCHAR* pLeftPath, const TCHAR* pRightPath )
	{
		std::tstring leftPath = MakeNormal( pLeftPath );		// backslashes only
		std::tstring rightPath = MakeNormal( pRightPath );

		TCHAR commonPrefix[ _MAX_PATH ] = _T("");
		::PathCommonPrefix( leftPath.c_str(), rightPath.c_str(), commonPrefix );
		return commonPrefix;
	}

	std::tstring StripCommonPrefix( const TCHAR* pFullPath, const TCHAR* pDirPath )
	{
		std::tstring restPath = pFullPath;

		if ( !str::IsEmpty( pDirPath ) )
		{
			size_t commonLen = str::_BaseCompare( pFullPath, pDirPath, &path::ToEquivalentChar ).second;
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
			if ( pred::Equal == CompareNPtr( rPath.c_str(), pPrefix, prefixLen ) )
			{
				if ( IsSlash( rPath.c_str()[ prefixLen ] ) )
					++prefixLen;			// cut leading slash

				rPath.erase( 0, prefixLen );
				return true;				// changed
			}

		return false;
	}

} //namespace path


namespace fs
{
	// CPathParts implementation

	void CPathParts::Clear( void )
	{
		m_drive.clear();
		m_dir.clear();
		m_fname.clear();
		m_ext.clear();
	}

	fs::CPath CPathParts::GetDirPath( void ) const
	{
		fs::CPath dirPath = m_drive;
		dirPath /= m_dir;
		dirPath.SetBackslash( false );
		return dirPath;
	}

	CPathParts& CPathParts::SetNameExt( const std::tstring& nameExt )
	{
		CPathParts parts( nameExt );
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
		TCHAR path[ _MAX_PATH * 2 ];
		size_t sepPos = std::tstring::npos;

		if ( !path::IsComplex( m_dir.c_str() ) )
			_tmakepath( path, m_drive.c_str(), m_dir.c_str(), m_fname.c_str(), m_ext.c_str() );
		else
		{
			ASSERT( path::IsWellFormed( m_dir.c_str() ) );

			std::tstring tempDir = m_dir;
			sepPos = path::FindComplexSepPos( tempDir.c_str() );
			tempDir[ sepPos ] = _T('/');		// temporary make it look like a real dir

			_tmakepath( path, m_drive.c_str(), tempDir.c_str(), m_fname.c_str(), m_ext.c_str() );

			sepPos += m_drive.length();
			ASSERT( _T('/') == path[ sepPos ] );	// restore to original
			path[ sepPos ] = path::s_complexPathSep;
		}

		return fs::CPath( path );
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
			tempPath[ sepPos ] = _T('/');		// temporary make it part of the dir path

			_tsplitpath( tempPath.c_str(), drive, dir, fname, ext );
		}

		m_drive = drive;
		m_dir = dir;
		m_fname = fname;
		m_ext = ext;

		if ( sepPos != std::tstring::npos )
		{
			sepPos -= m_drive.length();
			ASSERT( _T('/') == m_dir[ sepPos ] );	// restore to original
			m_dir[ sepPos ] = path::s_complexPathSep;
		}
	}

	std::tstring CPathParts::MakeFullPath( const TCHAR* pDrive, const TCHAR* pDir, const TCHAR* pFname, const TCHAR* pExt )
	{
		TCHAR path[ _MAX_PATH * 2 ];
		_tmakepath( path, pDrive, pDir, pFname, pExt );
		return path;
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
	}

	size_t CPath::GetDepth( void ) const
	{
		size_t depth = 0;

		fs::CPath parentPath = *this;
		parentPath.Normalize();

		while ( !parentPath.IsEmpty() )
		{
			++depth;

			fs::CPath newParentPath = parentPath.GetParentPath();

			if ( newParentPath != parentPath )
				parentPath = newParentPath;
			else
				break;
		}

		return depth;
	}

	CPath CPath::GetParentPath( bool trailSlash /*= false*/ ) const
	{
		ASSERT( !IsEmpty() );
		return path::GetParentPath( GetPtr(), trailSlash ? path::AddSlash : path::RemoveSlash );
	}

	CPath& CPath::SetBackslash( bool trailSlash /*= true*/ )
	{
		path::SetBackslash( m_filePath, trailSlash ? path::AddSlash : path::RemoveSlash );
		return *this;
	}

	void CPath::SetNameExt( const std::tstring& nameExt )
	{
		CPathParts parts( m_filePath );			// split into parts rather than use '/' operator in order to preserve fwd slashes
		parts.SetNameExt( nameExt );
		m_filePath = parts.MakePath().Get();
	}

	std::tstring CPath::GetFname( void ) const
	{
		const TCHAR* pNameExt = GetNameExt();
		const TCHAR* pExt = GetExt();

		return std::tstring( pNameExt, std::distance( pNameExt, pExt ) );
	}

	void CPath::SplitFilename( std::tstring& rFname, std::tstring& rExt ) const
	{
		rFname = GetFname();
		rExt = GetExt();
	}

	namespace impl
	{
		size_t GetDotExtCount( const fs::CPath& filePath )
		{
			const TCHAR* pNameExt = filePath.GetNameExt();
			return std::count( pNameExt, pNameExt + str::GetLength( pNameExt ), _T('.') );
		}
	}

	ExtensionMatch CPath::GetExtensionMatch( const fs::CPath& right ) const
	{
		if ( impl::GetDotExtCount( *this ) != impl::GetDotExtCount( right ) )
			return MismatchDotsExt;
		else if ( !HasExt( right.GetExt() ) )		// ignore case changes
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

	CPath CPath::ExtractExistingFilePath( void ) const
	{
		ASSERT( FileExist() );

		CFileFind finder;
		VERIFY( finder.FindFile( m_filePath.c_str() ) );
		finder.FindNextFile();

		return CPath( (const TCHAR*)finder.GetFilePath() );
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
		::GetLongPathName( filePath.GetPtr(), longPath, COUNT_OF( longPath ) );						// convert to long path

		return fs::CPath( longPath );
	}
}


namespace pred
{
	struct ComparPathDepth
	{
		ComparPathDepth( const stdext::hash_map< fs::CPath, size_t >& rFilePathsToDepth ) : m_rFilePathsToDepth( rFilePathsToDepth ) {}

		CompareResult operator()( const fs::CPath& leftPath, const fs::CPath& rightPath ) const
		{
			size_t leftDepth = utl::FindValue( m_rFilePathsToDepth, leftPath ), rightDepth = utl::FindValue( m_rFilePathsToDepth, rightPath );

			CompareResult result = Compare_Scalar( leftDepth, rightDepth );
			if ( Equal == result )
				result = m_comparePath( leftPath, rightPath );

			return result;
		}
	private:
		const stdext::hash_map< fs::CPath, size_t >& m_rFilePathsToDepth;
		pred::CompareNaturalPath m_comparePath;
	};
}


namespace fs
{
	void SortByPathDepth( std::vector< fs::CPath >& rFilePaths, bool ascending /*= true*/ )
	{
		stdext::hash_map< fs::CPath, size_t > filePathsToDepth;

		for ( std::vector< fs::CPath >::const_iterator itFilePath = rFilePaths.begin(); itFilePath != rFilePaths.end(); ++itFilePath )
			filePathsToDepth[ *itFilePath ] = itFilePath->GetDepth();

		std::sort( rFilePaths.begin(), rFilePaths.end(), pred::MakeOrderByValue( pred::ComparPathDepth( filePathsToDepth ), ascending ) );
	}
}


namespace path
{
	void QueryParentPaths( std::vector< fs::CPath >& rParentPaths, const std::vector< fs::CPath >& filePaths, bool uniqueOnly /*= true*/ )
	{
		for ( std::vector< fs::CPath >::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
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
