
#include "stdafx.h"
#include "PathInfo.h"
#include "PathSortOrder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



int revFindCharsPos( const TCHAR* string, const TCHAR* chars, int pos /*= -1*/ )
{
	if ( string == NULL || string[ 0 ] == _T('\0') || chars == NULL || chars[ 0 ] == _T('\0') )
		return -1;

	int len = str::Length( string );

	if ( pos == -1 )
		pos = len;
	ASSERT( pos >= 0 && pos <= len );

	while ( --pos >= 0 && _tcschr( chars, string[ pos ] ) == NULL )
		UNUSED_ALWAYS( 0 );

	return pos;
}


// path namespace implementation

namespace path
{
	void normalizePaths( std::vector< CString >& rOutFilepaths )
	{
		for ( std::vector< CString >::iterator itPath = rOutFilepaths.begin(); itPath != rOutFilepaths.end(); ++itPath )
			*itPath = path::MakeNormal( *itPath ).c_str();
	}
}


// PathInfo implementation

int PathInfo::s_extPredefinedOrder[] =
{
	12,		// ft::Unknown
	0,		// ft::Extless
	1,		// ft::H
	2,		// ft::CPP
	3,		// ft::C
	4,		// ft::HXX
	5,		// ft::CXX
	6,		// ft::IDL
	7,		// ft::TLB
	8,		// ft::RC
	9,		// ft::RES
	10,		// ft::DEF
	11,		// ft::DSP
	12,		// ft::DSW
	13,		// ft::DSM
	13,		// ft::BAS

	14,		// ft::SQL
	15,		// ft::TAB
	16,		// ft::PK,
	17,		// ft::PKG
	18,		// ft::PKB
	19,		// ft::PKS
	20,		// ft::PAC
	21,		// ft::OT
	22		// ft::OTB
};

PathInfo::PathInfo( void )
	: drive()
	, dir()
	, name()
	, ext()
	, dirName()
	, m_fileType( ft::Unknown )
{
}

PathInfo::PathInfo( const PathInfo& src )
	: drive( src.drive )
	, dir( src.dir )
	, name( src.name )
	, ext( src.ext )
	, dirName( src.dirName )
	, m_fileType( src.m_fileType )
{
}

PathInfo::PathInfo( const CString& _fullPath, bool doStdConvert /*= false*/ )
	: drive()
	, dir()
	, name()
	, ext()
	, dirName()
	, m_fileType( ft::Unknown )
{
	assign( _fullPath, doStdConvert );
}

PathInfo::PathInfo( const CString& _drive, const CString& _dir, const CString& _name, const CString& _ext )
	: drive( _drive )
	, dir( _dir )
	, name( _name )
	, ext( _ext )
{
	setupDirName();
	SetupFileType();
}

PathInfo::~PathInfo()
{
}

const std::vector< PathField >& PathInfo::GetDefaultOrder( void )
{
	return CPathSortOrder::GetDefaultOrder();
}

CString PathInfo::getField( PathField field, const TCHAR* defaultField /*= NULL*/ ) const
{
	switch ( field )
	{
		case pfDrive:		return drive;
		case pfDir:			return dir;
		case pfName:		return name;
		case pfExt:			return ext;
		case pfDirName:		return getDirName( defaultField );
		case pfFullPath:	return GetFullPath();
		case pfDirPath:		return getDirPath();
		case pfDirNameExt:	return getDirNameExt( defaultField );
		case pfNameExt:		return getNameExt();
		case pfCoreExt:		return getCoreExt();
	}
	VERIFY( false );
	return CString();
}

void PathInfo::Clear( void )
{
	drive.Empty();
	dir.Empty();
	name.Empty();
	ext.Empty();
	dirName.Empty();
	m_fileType = ft::Unknown;

	updateFullPath();	// Full-path update (derived types only).
}

PathInfo& PathInfo::operator=( const PathInfo& src )
{
	drive = src.drive;
	dir = src.dir;
	name = src.name;
	ext = src.ext;
	dirName = src.dirName;
	m_fileType = src.m_fileType;

	updateFullPath();	// Full-path update (derived types only).
	return *this;
}

bool PathInfo::operator==( const PathInfo& right ) const
{
	return
		path::EquivalentPtr( ext, right.ext ) &&
		path::EquivalentPtr( name, right.name ) &&
		path::EquivalentPtr( drive, right.drive ) &&
		path::EquivalentPtr( dir, right.dir );
}

pred::CompareResult PathInfo::Compare( const PathInfo& right,
									   const std::vector< PathField >& orderFields /*= GetDefaultOrder()*/,
									   const TCHAR* pDefaultDirName /*= NULL*/ ) const
{
	// make field comparisions as specified by orderFields while most signifiant fileds are equal
	pred::CompareResult result = pred::Equal;
	for ( size_t i = 0; result == 0 && i != orderFields.size(); ++i )
		result = CompareField( right, orderFields[ i ], pDefaultDirName );
	return result;
}

pred::CompareResult PathInfo::CompareField( const PathInfo& right, PathField field, const TCHAR* pDefaultDirName /*= NULL*/ ) const
{
	pred::CompareResult result = path::CompareNPtr( getField( field, pDefaultDirName ), right.getField( field, pDefaultDirName ) );

	if ( pfExt == field )
		if ( !( pred::Equal == result || ext.IsEmpty() || right.ext.IsEmpty() ) )		// different non-empty extensions
			if ( m_fileType != ft::Unknown || right.m_fileType != ft::Unknown )
				// compare extension custom order
				result = pred::ToCompareResult( s_extPredefinedOrder[ m_fileType ] - s_extPredefinedOrder[ right.m_fileType ] );

	return result;
}

bool PathInfo::smartNameExtEQ( const PathInfo& right ) const
{
	// compares filename+ext and only if any, directory path
	if ( !path::EquivalentPtr( getNameExt(), right.getNameExt() ) )
		return false;	// Filename+ext mismatch.

	bool thisAbs = isAbsolutePath(), cmpAbs = right.isAbsolutePath();

	if ( thisAbs != cmpAbs || !thisAbs )
		return true;	// Both or one of then not absolute path -> consider it a match !
	return path::EquivalentPtr( drive, right.drive ) && path::EquivalentPtr( dir, right.dir );
}

CString PathInfo::GetFullPath( void ) const
{
	CString fullPathDest;

	_tmakepath( fullPathDest.GetBuffer( MAX_PATH ), drive, dir, name, ext );
	fullPathDest.ReleaseBuffer();
	return fullPathDest;
}

void PathInfo::assign( const CString& _fullPath, bool doStdConvert /*= false*/ )
{
	_tsplitpath( doStdConvert ? path::MakeNormal( _fullPath ).c_str() : (LPCTSTR)_fullPath,
				 drive.GetBuffer( _MAX_DRIVE ),
				 dir.GetBuffer( _MAX_DIR ),
				 name.GetBuffer( _MAX_FNAME ),
				 ext.GetBuffer( _MAX_EXT ) );

	drive.ReleaseBuffer();
	dir.ReleaseBuffer();
	name.ReleaseBuffer();
	ext.ReleaseBuffer();

	setupDirName();
	SetupFileType();
}

bool PathInfo::isNetworkPath( void ) const
{
	return drive.IsEmpty() && dir.GetLength() > 2 && path::IsSlash( dir[ 0 ] ) && path::IsSlash( dir[ 1 ] );
}

bool PathInfo::hasWildcards( void ) const
{
	return _tcspbrk( name, _T("*?") ) != NULL || _tcspbrk( ext, _T("*?") ) != NULL;
}

bool PathInfo::exist( const TCHAR* pFilePath, bool allowDevices /*= false*/ )
{
	return
		!str::IsEmpty( pFilePath ) &&
		fs::FileExist( pFilePath, fs::Read ) &&
		( allowDevices || !isDeviceFile( pFilePath ) );
}

CString PathInfo::getDirPath( bool withTrailSlash /*= true*/ ) const
{
	// ex: for "E:\WINNT\system32\BROWSER.DLL", returns:
	//	withTrailSlash == true  -> "E:\WINNT\system32\"
	//	withTrailSlash == false -> "E:\WINNT\system32"
	if ( withTrailSlash || dir.IsEmpty() || isRootPath() )
		return drive + dir;
	else
		return drive + dir.Left( dir.GetLength() - 1 );
}

CString PathInfo::getDirName( const TCHAR* pDefaultDirName /*= NULL*/ ) const
{
	// ex: for "E:\WINNT\system32\BROWSER.DLL" -> returns "system32"
	// if pDefaultDirName="system32" -> returns "" !
	if ( pDefaultDirName != NULL && path::EquivalentPtr( dirName, pDefaultDirName ) )
		return CString();
	return dirName;
}

// ex: for "E:\WINNT\system32\BROWSER.DLL" -> returns "system32"

void PathInfo::setupDirName( void )
{
	dirName.Empty();
	if ( dir.IsEmpty() )
		return;
	if ( isRootPath() )
		dirName = _T('\\');
	else
	{
		int pos = revFindSlashPos( dirName = getDirPath( false ) );
		if ( pos != -1 )
			dirName = dirName.Mid( pos + 1 );
	}
}

void PathInfo::assignDirPath( CString dirPath, bool doStdConvert /*= false*/ )
// Assigns 'drive' and 'dir' members, alters 'dirName' and preserves 'name' and 'ext' members
{
	if ( doStdConvert )
		dirPath = path::MakeNormal( dirPath ).c_str();
	// Add trailing slash (if not already):
	if ( !dirPath.IsEmpty() && !path::IsSlash( dirPath[ dirPath.GetLength() - 1 ] ) )
		dirPath += _T('\\');

	_tsplitpath( dirPath,
				 drive.GetBuffer( _MAX_DRIVE ),
				 dir.GetBuffer( _MAX_DIR ),
				 NULL,
				 NULL );
	drive.ReleaseBuffer();
	dir.ReleaseBuffer();

	setupDirName();
	updateFullPath();	// Full-path update (derived types only).
}

void PathInfo::assignNameExt( const CString& nameExt, bool doStdConvert /*= false*/ )
// Assigns 'name' and 'ext' members and preserves 'drive' and 'dir' members
{
	_tsplitpath( doStdConvert ? path::MakeNormal( nameExt ).c_str() : (LPCTSTR)nameExt,
				 NULL,
				 NULL,
				 name.GetBuffer( _MAX_FNAME ),
				 ext.GetBuffer( _MAX_EXT ) );
	name.ReleaseBuffer();
	ext.ReleaseBuffer();

	SetupFileType();
	updateFullPath();	// Full-path update (derived types only).
}

void PathInfo::updateFields( void )
// Does nothing, but allows derivates to update path-fields from full-path info.
{
}

// does nothing, but allows derivates to update full-path info from path-fields
void PathInfo::updateFullPath( void )
{
}

CString PathInfo::getDirNameExt( const TCHAR* pDefaultDirName /*= NULL*/ ) const
{
	CString dirNameFriendly = getDirName( pDefaultDirName );

	if ( dirNameFriendly.IsEmpty() )
		return getNameExt();
	else
		return dirNameFriendly + _T('\\') + getNameExt();
}

void PathInfo::makeAbsolute( void )
{
	assign( PathInfo::makeAbsolute( GetFullPath() ) );
}

CString PathInfo::makeAbsolute( const TCHAR* pathToConvert )
{
	CString absolutePath;
	bool success = _tfullpath( absolutePath.GetBuffer( _MAX_PATH ), pathToConvert, _MAX_PATH ) != NULL;

	absolutePath.ReleaseBuffer();
	if ( !success )
		absolutePath = pathToConvert;		// on failure, assign the original path!

	return absolutePath;
}

TCHAR* PathInfo::findSubString( const TCHAR* pathString, const TCHAR* subString )
{
	if ( *subString == _T('\0') )
		return const_cast< TCHAR* >( pathString );

	for ( TCHAR *cmp = const_cast< TCHAR* >( pathString ); *cmp; cmp++ )
	{
		TCHAR* pStr = cmp, *subStr = const_cast< TCHAR* >( subString );

		while ( *pStr && *subStr && !( path::ToEquivalentChar( *pStr ) - path::ToEquivalentChar( *subStr ) ) )
			++pStr, ++subStr;

		if ( *subStr == _T('\0') )
			return cmp;
	}

	return NULL;
}

int PathInfo::find( const TCHAR* pathString, const TCHAR* subString, int startPos /*= 0*/ )
{
	ASSERT( pathString != NULL && subString != NULL && startPos >= startPos );
	ASSERT( startPos <= _tcslen( pathString ) );

	TCHAR* pMatch = findSubString( pathString + startPos, subString );

	return pMatch != NULL ? ( int( pMatch - pathString ) + startPos ) : ( -1 );
}


// PathInfoEx implementation

PathInfoEx::PathInfoEx( void )
	: PathInfo()
	, fullPath()
{
}

PathInfoEx::PathInfoEx( const PathInfoEx& src )
	: PathInfo( src )
	, fullPath( src.fullPath )
{
}

PathInfoEx::PathInfoEx( const CString& _fullPath, bool doStdConvert /*= false*/ )
	: PathInfo()
	, fullPath()
{
	assign( _fullPath, doStdConvert );
}

PathInfoEx::~PathInfoEx()
{
}

CString PathInfoEx::GetFullPath( void ) const
{
	ASSERT( isConsistent() );
	return fullPath;
}

void PathInfoEx::assign( const CString& _fullPath, bool doStdConvert /*= false*/ )
{
	if ( doStdConvert )
		fullPath = path::MakeNormal( _fullPath ).c_str();
	else
		fullPath = _fullPath;
	PathInfo::assign( fullPath, false /*already converted*/ );
	ASSERT( isConsistent() );
}

void PathInfoEx::updateFields( void )
{
	PathInfo::assign( fullPath, false );	// fullPath -> path components
}

void PathInfoEx::updateFullPath( void )
{
	fullPath = PathInfo::GetFullPath();		// path components -> fullPath
}

bool PathInfoEx::isConsistent( void ) const
{
	return path::EquivalentPtr( fullPath, PathInfo::GetFullPath() );
}


// FileFindEx implementation

bool FileFindEx::findDir( const TCHAR* dirPathFilter /*= NULL*/ )
{
#ifdef _WIN32_WINNT
	Close();

	m_pNextInfo = new WIN32_FIND_DATA;

	if ( dirPathFilter == NULL )
		dirPathFilter = _T("*.*");
	lstrcpy( ( (WIN32_FIND_DATA*)m_pNextInfo )->cFileName, dirPathFilter );

	m_hContext = ::FindFirstFileEx( dirPathFilter, FindExInfoStandard,
									(WIN32_FIND_DATA*)m_pNextInfo,
									FindExSearchLimitToDirectories, NULL, 0 );

	if ( m_hContext == INVALID_HANDLE_VALUE )
	{
		DWORD error =::GetLastError();

		Close();
		::SetLastError( error );
		return false;
	}

	TCHAR* pstrRoot = m_strRoot.GetBufferSetLength( _MAX_PATH );
	const TCHAR* pstr = _tfullpath( pstrRoot, dirPathFilter, _MAX_PATH );

	// Passed name isn't a valid path but was found by the API
	ASSERT( pstr != NULL );
	if ( pstr == NULL )
	{
		m_strRoot.ReleaseBuffer();
		Close();
		::SetLastError( ERROR_INVALID_NAME );
		return false;
	}
	else
	{
		// Find the last forward or backward whack
		TCHAR* pstrBack = _tcsrchr( pstrRoot, _T('\\') );
		TCHAR* pstrFront = _tcsrchr( pstrRoot, _T('/') );

		if ( pstrFront != NULL || pstrBack != NULL )
		{
			if ( pstrFront == NULL )
				pstrFront = pstrRoot;
			if ( pstrBack == NULL )
				pstrBack = pstrRoot;
			// From the start to the last whack is the root
			if ( pstrFront >= pstrBack )
				*pstrFront = '\0';
			else
				*pstrBack = '\0';
		}
		m_strRoot.ReleaseBuffer();
	}
	return true;
#else  // !_WIN32_WINNT
	return !!FindFile( dirPathFilter );
#endif  // _WIN32_WINNT
}
