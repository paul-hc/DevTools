#ifndef FileState_h
#define FileState_h
#pragma once

#include "Path.h"


namespace fs
{
	struct CFileState
	{
		CFileState( void ) : m_attributes( s_invalidAttributes ) {}
		CFileState( const ::CFileStatus* pFileStatus );
		explicit CFileState( const fs::CPath& fullPath );

		void Clear( void ) { *this = CFileState(); }

		bool IsEmpty( void ) const { return m_fullPath.IsEmpty(); }
		bool IsValid( void ) const { return !IsEmpty() && m_attributes != s_invalidAttributes; }
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
	private:
		static const BYTE s_invalidAttributes = 0xFF;
	};


	typedef std::map< fs::CFileState, fs::CFileState > TFileStatePairMap;
	typedef std::set< fs::CFileState > TFileStateSet;
}


namespace func
{
	inline const fs::CPath& PathOf( const fs::CFileState& keyFileState ) { return keyFileState.m_fullPath; }		// for uniform path algorithms
}


#endif // FileState_h
