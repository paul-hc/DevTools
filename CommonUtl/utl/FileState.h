#ifndef FileState_h
#define FileState_h
#pragma once

#include "Path.h"


namespace fs
{
	struct CFileState
	{
		CFileState( void ) : m_attributes( 0 ) {}
		CFileState( const CFileStatus& fileStatus );

		void Clear( void ) { *this = CFileState(); }

		bool IsEmpty( void ) const { return m_fullPath.IsEmpty(); }
		bool FileExist( AccessMode accessMode = Exist ) { return m_fullPath.FileExist( accessMode ); }

		bool operator==( const CFileState& right ) const;
		bool operator<( const CFileState& right ) const { return pred::Less == pred::CompareEquivalentPath()( m_fullPath, right.m_fullPath ); }		// order by path

		bool GetFileState( const TCHAR* pSrcFilePath );
		void SetFileState( const TCHAR* pDestFilePath = NULL ) const throws_( CFileException, mfc::CRuntimeException );
	public:
		CPath m_fullPath;
		BYTE m_attributes;			// CFile::Attribute enum values (low-byte)
		CTime m_creationTime;		// creation time of file
		CTime m_modifTime;			// last modification time of file
		CTime m_accessTime;			// last access time of file
	};


	typedef std::map< fs::CFileState, fs::CFileState > TFileStatePairMap;
	typedef std::set< fs::CFileState > TFileStateSet;
}


#endif // FileState_h
