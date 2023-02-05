#ifndef FileState_h
#define FileState_h
#pragma once

#include "FileSystem_fwd.h"


class CFlagTags;


namespace fs
{
	struct CFileState
	{
		CFileState( void ) : m_fileSize( 0 ), m_attributes( s_invalidAttributes ), m_crc32( 0 ) {}
		CFileState( const ::CFileStatus* pFileStatus );
		CFileState( const CFileFind& foundFile );

		void Clear( void ) { *this = CFileState(); }

		bool IsEmpty( void ) const { return m_fullPath.IsEmpty(); }
		bool IsValid( void ) const { return !IsEmpty() && m_attributes != s_invalidAttributes; }

		bool IsReadOnly( void ) const { return IsValid() && HasFlag( m_attributes, CFile::readOnly ); }
		bool IsHidden( void ) const { return IsValid() && HasFlag( m_attributes, CFile::hidden ); }
		bool IsSystem( void ) const { return IsValid() && HasFlag( m_attributes, CFile::system ); }
		bool IsDirectory( void ) const { return IsValid() && HasFlag( m_attributes, CFile::directory ); }
		bool IsRegularFile( void ) const { return IsValid() && !HasFlag( m_attributes, CFile::directory ); }
		bool IsProtected( void ) const { return IsValid() && HasFlag( m_attributes, CFile::readOnly | CFile::hidden | CFile::system ); }

		bool FileExist( AccessMode accessMode = Exist ) const { return m_fullPath.FileExist( accessMode ); }

		bool operator<( const CFileState& right ) const { return pred::Less == pred::CompareNaturalPath()( m_fullPath, right.m_fullPath ); }		// order by path
		bool operator==( const CFileState& right ) const;
		bool operator!=( const CFileState& right ) const { return !operator==( right ); }

		static CFileState ReadFromFile( const fs::CPath& path );		// could be relative path, will be stored as absolute
		void WriteToFile( void ) const throws_( CFileException, mfc::CRuntimeException );

		// time fields
		const CTime& GetTimeField( TimeField field ) const;
		void SetTimeField( const CTime& time, TimeField field ) { const_cast<CTime&>( GetTimeField( field ) ) = time; }

		enum ChecksumEvaluation { Compute, CacheCompute, AsIs };

		UINT GetCrc32( ChecksumEvaluation evaluation = Compute ) const;
		UINT ComputeCrc32( ChecksumEvaluation evaluation ) const;
		void ResetCrc32( void ) { m_crc32 = 0; }

		// serialization
		void Stream( CArchive& archive );

		friend inline CArchive& operator>>( CArchive& archive, fs::CFileState& rFileState ) { rFileState.Stream( archive ); return archive; }
		friend inline CArchive& operator<<( CArchive& archive, const fs::CFileState& fileState ) { const_cast<fs::CFileState&>( fileState ).Stream( archive ); return archive; }

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
		mutable UINT m_crc32;		// lazy computation of CRC32 checksum

		static const BYTE s_invalidAttributes = 0xFF;
	};


	class CSecurityDescriptor
	{
	public:
		CSecurityDescriptor( void ) {}
		CSecurityDescriptor( const fs::CPath& filePath ) { ReadFromFile( filePath ); }

		PSECURITY_DESCRIPTOR ReadFromFile( const fs::CPath& filePath );
		bool WriteToFile( const fs::CPath& destFilePath ) const;

		static bool CopyToFile( const fs::CPath& srcFilePath, const fs::CPath& destFilePath );

		bool IsValid( void ) const { return !m_securityDescriptor.empty(); }
		PSECURITY_DESCRIPTOR GetDescriptor( void ) const { return IsValid() ? (PSECURITY_DESCRIPTOR)&m_securityDescriptor.front() : NULL; }
	private:
		std::vector<BYTE> m_securityDescriptor;
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
