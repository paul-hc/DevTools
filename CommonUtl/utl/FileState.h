#ifndef FileState_h
#define FileState_h
#pragma once

#include "FileSystem_fwd.h"


class CFlagTags;


namespace fs
{
	struct CFileState
	{
		CFileState( void ) : m_fileSize( 0 ), m_attributes( s_invalidAttributes ) {}
		CFileState( const ::CFileStatus* pFileStatus );

		void Clear( void ) { *this = CFileState(); }

		bool IsEmpty( void ) const { return m_fullPath.IsEmpty(); }
		bool IsValid( void ) const { return !IsEmpty() && m_attributes != s_invalidAttributes; }
		bool FileExist( AccessMode accessMode = Exist ) const { return m_fullPath.FileExist( accessMode ); }

		bool operator<( const CFileState& right ) const { return pred::Less == pred::CompareNaturalPath()( m_fullPath, right.m_fullPath ); }		// order by path
		bool operator==( const CFileState& right ) const;
		bool operator!=( const CFileState& right ) const { return !operator==( right ); }

		static CFileState ReadFromFile( const fs::CPath& path );		// could be relative path, will be stored as absolute
		void WriteToFile( void ) const throws_( CFileException, mfc::CRuntimeException );

		// time fields
		const CTime& GetTimeField( TimeField field ) const;
		void SetTimeField( const CTime& time, TimeField field ) { const_cast< CTime& >( GetTimeField( field ) ) = time; }

		static const CFlagTags& GetTags_FileAttributes( void );
	private:
		void ModifyFileStatus( const ::CFileStatus& newStatus ) const throws_( CFileException );
		void ModifyFileTimes( const ::CFileStatus& newStatus, bool isDirectory ) const throws_( CFileException );

		void ThrowLastError( DWORD osLastError = ::GetLastError() ) const throws_( CFileException ) { CFileException::ThrowOsError( osLastError, m_fullPath.GetPtr() ); }
	public:
		CPath m_fullPath;
		UINT64 m_fileSize;
		BYTE m_attributes;			// CFile::Attribute enum values (low-byte)
		CTime m_creationTime;		// creation time of file
		CTime m_modifTime;			// last modification time of file
		CTime m_accessTime;			// last access time of file
	private:
		static const BYTE s_invalidAttributes = 0xFF;
	};
}


namespace func
{
	inline const fs::CPath& PathOf( const fs::CFileState& keyFileState ) { return keyFileState.m_fullPath; }		// for uniform path algorithms
}


#include <iosfwd>

std::ostream& operator<<( std::ostream& os, const fs::CFileState& fileState );
std::wostream& operator<<( std::wostream& os, const fs::CFileState& fileState );


#endif // FileState_h
