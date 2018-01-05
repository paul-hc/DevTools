#ifndef PathInfo_h
#define PathInfo_h
#pragma once

#include <io.h>
#include <algorithm>
#include "utl/Path.h"
#include "FileType.h"
#include "PublicEnums.h"


int revFindCharsPos( const TCHAR* string, const TCHAR* chars, int pos = -1 );

inline const TCHAR* revFindChars( const TCHAR* string, const TCHAR* chars )
{
	int pos = revFindCharsPos( string, chars );
	return pos == -1 ? NULL : ( string + pos );
}


namespace path
{
	void normalizePaths( std::vector< CString >& rOutFilepaths );
}


struct PathInfo
{
public:
	PathInfo( void );
	PathInfo( const PathInfo& src );
	PathInfo( const CString& _fullPath, bool doStdConvert = false );
	PathInfo( const CString& _drive, const CString& _dir, const CString& _name, const CString& _ext );
	~PathInfo();

	// assignment
	PathInfo& operator=( const PathInfo& src );

	virtual void assign( const CString& _fullPath, bool doStdConvert = false );

	void assignDirPath( CString pDirPath, bool doStdConvert = false );
	void assignNameExt( const CString& pNameExt, bool doStdConvert = false );
	void assignComponents( const TCHAR* pDirPath, const TCHAR* pNameExt ) { assign( path::Combine( pDirPath, pNameExt ).c_str() ); }

	static CString fromComponents( const TCHAR* pDirPath, const TCHAR* pNameExt ) { return path::Combine( pDirPath, pNameExt ).c_str(); }

	// comparision
	bool operator==( const PathInfo& right ) const;
	bool operator!=( const PathInfo& right ) const { return !operator==( right ); }
	bool operator<( const PathInfo& right ) const { return path::CompareNPtr( GetFullPath(), right.GetFullPath() ) < 0; }
	pred::CompareResult Compare( const PathInfo& right, const std::vector< PathField >& orderArray = GetDefaultOrder(),
								 const TCHAR* pDefaultDirName = NULL ) const;
	bool smartNameExtEQ( const PathInfo& right ) const;

	// path conversion
	virtual CString GetFullPath( void ) const;
	void SetupFileType( void ) { m_fileType = ft::FindTypeOfExtension( ext ); }

	CString getDirPath( bool withTrailSlash = true ) const;				// "C:\WINNT\system32\" or "C:\WINNT\system32"
	CString getDirName( const TCHAR* pDefaultDirName = NULL ) const;	// "system32"
	CString getDirNameExt( const TCHAR* pDefaultDirName = NULL ) const;	// "system32\BROWSER.DLL"
	CString getNameExt( void ) const { return name + ext; }				// "BROWSER.DLL"

	const TCHAR* getCoreExt( void ) const { return (const TCHAR*)ext + !ext.IsEmpty(); }	// "DLL"

	// attributes
	bool isEmpty( void ) const { return GetFullPath().IsEmpty() != FALSE; }
	bool isAbsolutePath( void ) const { return !drive.IsEmpty() || path::IsSlash( *(LPCTSTR)dir ); }
	bool isRelativePath( void ) const { return drive.IsEmpty() && !path::IsSlash( *(LPCTSTR)dir ); }
	bool isNetworkPath( void ) const;
	bool isRootPath( void ) const { return dir.GetLength() == 1 && path::IsSlash( *(LPCTSTR)dir ); }
	bool isFileNameOnly( void ) const { return drive.IsEmpty() && dir.IsEmpty(); }
	bool hasWildcards( void ) const;

	bool isDeviceFile( void ) const { return isDeviceFile( GetFullPath() ); }
	static bool isDeviceFile( const TCHAR* pFilePath ) { return UINT_MAX == ::GetFileAttributes( pFilePath ); }

	bool exist( bool allowDevices = false ) const { return PathInfo::exist( GetFullPath(), allowDevices ); }
	static bool exist( const TCHAR* filePath, bool allowDevices = false );

	void makeAbsolute( void );
	static CString makeAbsolute( const TCHAR* pathToConvert );

	// operations
	void Clear( void );
	virtual void updateFields( void );
	virtual void updateFullPath( void );
public:
	static const TCHAR* revFindSlash( const TCHAR* path ) { return ::revFindChars( path, _T("\\/") ); }
	static int revFindSlashPos( const TCHAR* path, int pos = -1 ) { return ::revFindCharsPos( path, _T("\\/"), pos ); }

	static TCHAR* findSubString( const TCHAR* pathString, const TCHAR* subString );
	static int find( const TCHAR* pathString, const TCHAR* subString, int startPos = 0 );

	static const std::vector< PathField >& GetDefaultOrder( void );
protected:
	void setupDirName( void );

	CString getField( PathField field, const TCHAR* defaultField = NULL ) const;
	pred::CompareResult CompareField( const PathInfo& right, PathField field, const TCHAR* pDefaultDirName = NULL ) const;
public:									// "C:\WINNT\system32\BROWSER.DLL":
	CString drive;					// "C:"
	CString dir;					// "\WINNT\system32\"
	CString name;					// "BROWSER"
	CString ext;					// ".DLL"
	CString dirName;				// "system32"
	ft::FileType m_fileType;		// ft::Unknown

	static int extPredefinedOrder[];
};


struct PathInfoEx : public PathInfo
{
	PathInfoEx( void );
	PathInfoEx( const PathInfoEx& src );
	PathInfoEx( const CString& _fullPath, bool doStdConvert = false );
	~PathInfoEx();

	// path conversion
	const CString& Get( void ) const { return fullPath; }
	virtual CString GetFullPath( void ) const;
	virtual void assign( const CString& _fullPath, bool doStdConvert = false );

	// operations
	virtual void updateFields( void );
	virtual void updateFullPath( void );

	// comparision
	bool operator==( const PathInfoEx& right ) const { ASSERT( isConsistent() && right.isConsistent() ); return path::EquivalentPtr( fullPath, right.fullPath ); }
	bool operator!=( const PathInfoEx& right ) const { return !operator==( right ); }
	bool operator==( const CString& rightFullPath ) const { ASSERT( isConsistent() ); return path::EquivalentPtr( fullPath, rightFullPath ); }
	bool operator!=( const CString& rightFullPath ) const { return !operator==( rightFullPath ); }

	// attributes
	bool isConsistent( void ) const;
public:
	CString fullPath;
};


struct CPathOrder
{
	CPathOrder( void ) {}

	bool IsEmpty( void ) const { return m_fields.empty(); }

	void Clear( void ) { m_fields.clear(); }
	void Add( PathField field );
	bool Toggle( PathField field );

	std::tstring GetAsString( void ) const;
	void SetFromString( const std::tstring& source );
public:
	std::vector< PathField > m_fields;
};


struct LessPathPred
{
	template< typename PathStringType >
	bool operator()( const PathStringType& left, const PathStringType& right ) const
	{
		return pred::Less == path::CompareNPtr( left, right );
	}
};


struct EqualPathPred
{
	template< typename PathStringType >
	bool operator()( const PathStringType& left, const PathStringType& right ) const
	{
		return path::EquivalentPtr( left, right );
	}
};


class FileFindEx : public CFileFind
{
public:
	FileFindEx( void ) {}

	bool findDir( const TCHAR* dirPathFilter = NULL );
};


#endif // PathInfo_h
