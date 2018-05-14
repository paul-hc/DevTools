#ifndef FileInfo_h
#define FileInfo_h
#pragma once

#include "Path.h"


namespace fs
{
	struct CFileInfo
	{
		CFileInfo( void ) : m_attributes( 0 ) {}
		CFileInfo( const CFileStatus& fileStatus );

		void Clear( void ) { *this = CFileInfo(); }

		bool IsEmpty( void ) const { return m_fullPath.IsEmpty(); }
		bool FileExist( AccessMode accessMode = Exist ) { return m_fullPath.FileExist( accessMode ); }

		bool operator<( const CFileInfo& right ) const { return pred::Less == pred::CompareEquivalentPath()( m_fullPath, right.m_fullPath ); }		// order by path

		bool GetFileInfo( const TCHAR* pSrcFilePath );
		void SetFileInfo( const TCHAR* pDestFilePath = NULL ) const throws_( CFileException, mfc::CRuntimeException );

		std::tstring Format( void ) const;
		bool Parse( const std::tstring& text );
	public:
		CPath m_fullPath;
		BYTE m_attributes;			// CFile::Attribute enum values (low-byte)
		CTime m_creationTime;		// creation time of file
		CTime m_modifTime;			// last modification time of file
		CTime m_accessTime;			// last access time of file
	private:
		enum { FullPath, Attributes, CreationTime, ModifTime, AccessTime, _FieldCount };
		static const TCHAR s_sep[];
	};


	typedef std::set< fs::CFileInfo > TFileInfoSet;
}


#endif // FileInfo_h
