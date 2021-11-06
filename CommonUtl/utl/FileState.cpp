
#include "stdafx.h"
#include "FileState.h"
#include "FileSystem.h"
#include "Crc32.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "RuntimeException.h"
#include "SerializeStdTypes.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define EDITABLE_ATTRIBUTE_MASK ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL )


namespace fs
{
	// CFileState implementation

	CFileState::CFileState( const ::CFileStatus* pFileStatus )
		: m_fullPath( pFileStatus->m_szFullName )
		, m_fileSize( static_cast<UINT64>( pFileStatus->m_size ) )
		, m_attributes( static_cast<BYTE>( pFileStatus->m_attribute ) )
		, m_creationTime( pFileStatus->m_ctime )
		, m_modifTime( pFileStatus->m_mtime )
		, m_accessTime( pFileStatus->m_atime )
		, m_crc32( 0 )
	{
	}

	CFileState::CFileState( const CFileFind& foundFile )
		: m_fullPath( foundFile.GetFilePath().GetString() )
		, m_fileSize( foundFile.GetLength() )
		, m_attributes( static_cast<BYTE>( fs::GetFileAttributes( foundFile ) ) )
		, m_crc32( 0 )
	{
		foundFile.GetCreationTime( m_creationTime );
		foundFile.GetLastWriteTime( m_modifTime );
		foundFile.GetLastAccessTime( m_accessTime );
	}

	CFileState CFileState::ReadFromFile( const fs::CPath& path )
	{
		::CFileStatus fileStatus;
		if ( !CFile::GetStatus( path.GetPtr(), fileStatus ) )
		{
			CFileState badFileState;
			badFileState.m_fullPath = path;
			return badFileState;
		}

		return CFileState( &fileStatus );
	}

	void CFileState::WriteToFile( void ) const throws_( CFileException, mfc::CRuntimeException )
	{
		REQUIRE( IsValid() );

		::CFileStatus newStatus;
		if ( !CFile::GetStatus( m_fullPath.GetPtr(), newStatus ) )
			throw new mfc::CRuntimeException( str::Format( _T("Cannot access file status for file: %s"), m_fullPath.GetPtr() ) );

		newStatus.m_attribute = m_attributes;
		newStatus.m_ctime = m_creationTime;
		newStatus.m_mtime = m_modifTime;
		newStatus.m_atime = m_accessTime;

		ModifyFileStatus( newStatus );		// was CFile::SetStatus( m_fullPath.GetPtr(), newStatus );
	}

	bool CFileState::operator==( const CFileState& right ) const
	{
		return
			m_fullPath == right.m_fullPath &&
			m_fileSize == right.m_fileSize &&
			m_attributes == right.m_attributes &&
			m_creationTime == right.m_creationTime &&
			m_modifTime == right.m_modifTime &&
			m_accessTime == right.m_accessTime;
	}

	const CTime& CFileState::GetTimeField( TimeField field ) const
	{
		switch ( field )
		{
			default: ASSERT( false );
			case ModifiedDate:	return m_modifTime;
			case CreatedDate:	return m_creationTime;
			case AccessedDate:	return m_accessTime;
		}
	}

	void CFileState::ModifyFileStatus( const ::CFileStatus& newStatus ) const throws_( CFileException )
	{
		// Verbatim from CFile::SetStatus( LPCTSTR lpszFileName, const CFileStatus& status ).
		//	works for file using FILE_ATTRIBUTE_NORMAL - like CFile::SetStatus()
		//	works for directory using FILE_FLAG_BACKUP_SEMANTICS - new

		DWORD currentAttr = ::GetFileAttributes( m_fullPath.GetPtr() );
		if ( INVALID_FILE_ATTRIBUTES == currentAttr )
			ThrowLastError();

		ModifyFileTimes( newStatus, HasFlag( currentAttr, FILE_ATTRIBUTE_DIRECTORY ) );

		DWORD newAttr = currentAttr;
		SetMaskedValue( newAttr, EDITABLE_ATTRIBUTE_MASK, static_cast<DWORD>( newStatus.m_attribute ) );

		if ( newAttr != currentAttr )
			if ( !::SetFileAttributes( m_fullPath.GetPtr(), newAttr ) )
				ThrowLastError();
	}

	void CFileState::ModifyFileTimes( const ::CFileStatus& newStatus, bool isDirectory ) const throws_( CFileException )
	{
		std::vector< std::pair< FILETIME, const FILETIME* > > times( _TimeFieldCount );		// pair.second: ptr not NULL when time is defined (!= 0)

		times[ ModifiedDate ].second = fs::thr::MakeFileTime( times[ ModifiedDate ].first, newStatus.m_mtime, newStatus.m_szFullName, fs::MfcExc );			// last modification time
		if ( times[ ModifiedDate ].second != NULL )
		{
			times[ AccessedDate ].second = fs::thr::MakeFileTime( times[ AccessedDate ].first, newStatus.m_atime, newStatus.m_szFullName, fs::MfcExc );		// last access time
			times[ CreatedDate ].second = fs::thr::MakeFileTime( times[ CreatedDate ].first, newStatus.m_ctime, newStatus.m_szFullName, fs::MfcExc );		// create time

			fs::CScopedWriteableFile scopedWriteable( m_fullPath.GetPtr() );
			fs::CHandle file( ::CreateFile( m_fullPath.GetPtr(), GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ, NULL, OPEN_EXISTING,
				isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,								// IMPORTANT: for access to directory vs file
				NULL ) );

			if ( !file.IsValid() ||
				 !::SetFileTime( file.Get(), times[ CreatedDate ].second, times[ AccessedDate ].second, times[ ModifiedDate ].second ) ||
				 !file.Close() )
				ThrowLastError();
		}
	}

	UINT CFileState::GetCrc32( ChecksumEvaluation evaluation /*= Compute*/ ) const
	{
		if ( evaluation != AsIs )
			if ( 0 == m_crc32 && m_fileSize != 0 )		// not computed?
				ComputeCrc32( evaluation );

		return m_crc32;
	}

	UINT CFileState::ComputeCrc32( ChecksumEvaluation evaluation ) const
	{
		switch ( evaluation )
		{
			case Compute:
				m_crc32 = crc32::ComputeFileChecksum( m_fullPath );
				break;
			case CacheCompute:
				m_crc32 = fs::CCrc32FileCache::Instance().AcquireCrc32( m_fullPath );
				break;
		}
		return m_crc32;
	}

	void CFileState::Stream( CArchive& archive )
	{
		static const BYTE s_newBinFormat = 0xFE;	// distinct from s_invalidAttributes

		if ( archive.IsStoring() )
			archive
				<< m_fullPath
				<< s_newBinFormat		// save with the new binary format (including m_crc32)
				<< m_attributes
				<< m_creationTime
				<< m_modifTime
				<< m_accessTime
				<< m_crc32;
		else
		{
			archive >> m_fullPath;
			archive >> m_attributes;

			const bool isNewBinFormat = s_newBinFormat == m_attributes;
			if ( isNewBinFormat )
				archive >> m_attributes;	// the real m_attributes comes next in the stream

			archive
				>> m_creationTime
				>> m_modifTime
				>> m_accessTime;

			if ( isNewBinFormat )
				archive >> m_crc32;
			else
				m_crc32 = 0;
		}
	}


	const CFlagTags& CFileState::GetTags_FileAttributes( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FILE_ATTRIBUTE_READONLY, _T("R") },		// CFile::readOnly
			{ FILE_ATTRIBUTE_HIDDEN, _T("H") },			// CFile::hidden
			{ FILE_ATTRIBUTE_SYSTEM, _T("S") },			// CFile::system
			{ CFile::volume, _T("V") },
			{ FILE_ATTRIBUTE_DIRECTORY, _T("D") },		// CFile::directory
			{ FILE_ATTRIBUTE_ARCHIVE, _T("A") },		// CFile::archive
			{ FILE_ATTRIBUTE_DEVICE, _T("d") },
			{ FILE_ATTRIBUTE_NORMAL, _T("N") },
			{ FILE_ATTRIBUTE_TEMPORARY, _T("t") },
			{ FILE_ATTRIBUTE_SPARSE_FILE, _T("s") },
			{ FILE_ATTRIBUTE_REPARSE_POINT, _T("r") },
			{ FILE_ATTRIBUTE_COMPRESSED, _T("c") },
			{ FILE_ATTRIBUTE_OFFLINE, _T("o") },
			{ FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, _T("n") },
			{ FILE_ATTRIBUTE_ENCRYPTED, _T("e") },
		};
		static const std::tstring s_uiTags = _T("READ-ONLY|HIDDEN|SYSTEM|VOLUME|DIRECTORY|ARCHIVE|Device|NORMAL|Temporary|Sparse File|Reparse Point|Compressed|Offline|Not Content Indexed|Encrypted");

		static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ), s_uiTags );

		return s_tags;
	}


	// CSecurityDescriptor implementation

	PSECURITY_DESCRIPTOR CSecurityDescriptor::ReadFromFile( const fs::CPath& filePath )
	{
		DWORD dwLength = 0;

		if ( ::GetFileSecurity( filePath.GetPtr(), DACL_SECURITY_INFORMATION, NULL, dwLength, &dwLength ) )
		{
			m_securityDescriptor.resize( dwLength );

			PSECURITY_DESCRIPTOR pSecurityDescriptor = GetDescriptor();

			if ( ::GetFileSecurity( filePath.GetPtr(), DACL_SECURITY_INFORMATION, pSecurityDescriptor, dwLength, &dwLength ) )
				return pSecurityDescriptor;
		}

		m_securityDescriptor.clear();
		return NULL;
	}

	bool CSecurityDescriptor::WriteToFile( const fs::CPath& destFilePath ) const
	{
		REQUIRE( IsValid() );
		return ::SetFileSecurity( destFilePath.GetPtr(), DACL_SECURITY_INFORMATION, GetDescriptor() ) != FALSE;
	}

	bool CSecurityDescriptor::CopyToFile( const fs::CPath& srcFilePath, const fs::CPath& destFilePath )
	{
		CSecurityDescriptor securityDescriptor( srcFilePath );

		return securityDescriptor.IsValid() && securityDescriptor.WriteToFile( destFilePath );
	}
}


#include "FmtUtils.h"

std::ostream& operator<<( std::ostream& os, const fs::CFileState& fileState )
{
	return os << fmt::FormatClipFileState( fileState );
}

std::wostream& operator<<( std::wostream& os, const fs::CFileState& fileState )
{
	return os << fmt::FormatClipFileState( fileState );
}
