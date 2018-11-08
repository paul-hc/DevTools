#ifndef FileInfo_h
#define FileInfo_h

#include "utl/Path.h"


struct CFileInfo
{
	CFileInfo( void );
	CFileInfo( const CFileFind& foundFile );
	CFileInfo( const std::tstring& fullPath );

	void Clear( void );

	bool Exist( void ) const;

	bool IsRegularFile( void ) const { return Exist() && !IsDirectory(); }
	bool IsDirectory( void ) const { return Exist() && HasFlag( m_attributes, FILE_ATTRIBUTE_DIRECTORY ); }
	bool IsReadOnly( void ) const { return Exist() && HasFlag( m_attributes, FILE_ATTRIBUTE_READONLY ); }
	bool IsProtected( void ) const { return Exist() && HasFlag( m_attributes, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ); }

	bool DirPathExist( void ) const;
private:
	static std::tstring AdjustPath( const TCHAR* pFullPath, path::TrailSlash trailSlash );
public:
	fs::CPath m_fullPath;
	DWORD m_attributes;
	CTime m_lastModifiedTimestamp;
};


#endif // FileInfo_h
