
#include "pch.h"
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

// ULARGE_INTEGER conversion
#define MAKE_UINT64( low, hi )	( static_cast<UINT64>( low ) | static_cast<UINT64>( hi ) << 32 )
#define LO_DWORD( largeUInt )	static_cast<DWORD>( largeUInt & 0xFFFFFFFF )
#define HI_DWORD( largeUInt )   static_cast<DWORD>( ( largeUInt >> 32 ) & 0xFFFFFFFF )


namespace fs
{
	CTime FromFileTime( FILETIME fileTime )
	{
		if ( !CTime::IsValidFILETIME( fileTime ) )
			return CTime();

		return CTime( fileTime );
	}


	// CFileState implementation

	CFileState::CFileState( void )
		: m_fileSize( 0 )
		, m_attributes( s_invalidAttributes )
		, m_crc32( 0 )
	{
	}

	CFileState::CFileState( const fs::CPath& fullPath )
		: m_fileSize( 0 )
		, m_attributes( s_invalidAttributes )
		, m_crc32( 0 )
	{
		Retrieve( fullPath );
	}

	CFileState::CFileState( const WIN32_FILE_ATTRIBUTE_DATA& fileAttrData, const TCHAR* pFullPath )
		: m_fullPath( pFullPath )
	{	// built from ::GetFileAttributesEx()
		SetFrom( fileAttrData );
	}

	CFileState::CFileState( const WIN32_FIND_DATA& findData )
	{
		m_fullPath.Set( findData.cFileName );
		SetFrom( findData );
	}

	CFileState::CFileState( const ::CFileStatus* pMfcFileStatus )
		: m_fullPath( pMfcFileStatus->m_szFullName )
		, m_fileSize( static_cast<UINT64>( pMfcFileStatus->m_size ) )
		, m_attributes( static_cast<BYTE>( pMfcFileStatus->m_attribute ) )
		, m_creationTime( pMfcFileStatus->m_ctime )
		, m_modifTime( pMfcFileStatus->m_mtime )
		, m_accessTime( pMfcFileStatus->m_atime )
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

	void CFileState::Set( const WIN32_FILE_ATTRIBUTE_DATA& fileAttrData, const TCHAR* pFullPath )
	{
		m_fullPath.Set( pFullPath );
		SetFrom( fileAttrData );
	}

	void CFileState::Set( const WIN32_FIND_DATA& findData )
	{
		m_fullPath.Set( findData.cFileName );
		SetFrom( findData );
		m_crc32 = 0;
	}

	bool CFileState::Retrieve( const fs::CPath& fullPath )
	{
		WIN32_FILE_ATTRIBUTE_DATA fileAttrData;

		*this = CFileState();
		m_fullPath = fullPath;

		if ( 0 == ::GetFileAttributesEx( m_fullPath.GetPtr(), GetFileExInfoStandard, &fileAttrData ) )
			return false;

		SetFrom( fileAttrData );
		return true;
	}

	template< typename Win32AttrDataT >
	void CFileState::SetFrom( const Win32AttrDataT& src )
	{
		m_fileSize = MAKE_UINT64( src.nFileSizeLow, src.nFileSizeHigh );
		m_attributes = static_cast<BYTE>( src.dwFileAttributes );		// store only the CFile::Attribute enum values (low-byte), and discard higher ones
		m_creationTime = FromFileTime( src.ftCreationTime );
		m_modifTime = FromFileTime( src.ftLastWriteTime );
		m_accessTime = FromFileTime( src.ftLastAccessTime );
		m_crc32 = 0;
	}

	CFileState CFileState::ReadFromFile( const fs::CPath& path )
	{
		::CFileStatus mfcFileStatus;
		if ( !CFile::GetStatus( path.GetPtr(), mfcFileStatus ) )
		{
			CFileState badFileState;
			badFileState.m_fullPath = path;
			return badFileState;
		}

		return CFileState( &mfcFileStatus );
	}

	void CFileState::WriteToFile( void ) const throws_( CFileException, mfc::CRuntimeException )
	{
		REQUIRE( IsValid() );

		::CFileStatus mfcFileStatus;
		if ( !CFile::GetStatus( m_fullPath.GetPtr(), mfcFileStatus ) )
			throw new mfc::CRuntimeException( str::Format( _T("Cannot access file status for file: %s"), m_fullPath.GetPtr() ) );

		mfcFileStatus.m_attribute = m_attributes;
		mfcFileStatus.m_ctime = m_creationTime;
		mfcFileStatus.m_mtime = m_modifTime;
		mfcFileStatus.m_atime = m_accessTime;

		ModifyFileStatus( mfcFileStatus );		// was CFile::SetStatus( m_fullPath.GetPtr(), mfcFileStatus );
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

	CTime& CFileState::RefTimeField( TimeField field )
	{
		switch ( field )
		{
			default: ASSERT( false );
			case ModifiedDate:	return m_modifTime;
			case CreatedDate:	return m_creationTime;
			case AccessedDate:	return m_accessTime;
		}
	}

	void CFileState::ModifyFileStatus( const ::CFileStatus& mfcFileStatus ) const throws_( CFileException )
	{
		// Verbatim from CFile::SetStatus( LPCTSTR lpszFileName, const CFileStatus& status ).
		//	works for file using FILE_ATTRIBUTE_NORMAL - like CFile::SetStatus()
		//	works for directory using FILE_FLAG_BACKUP_SEMANTICS - new

		DWORD currentAttr = ::GetFileAttributes( m_fullPath.GetPtr() );
		if ( INVALID_FILE_ATTRIBUTES == currentAttr )
			ThrowLastError();

		ModifyFileTimes( mfcFileStatus, HasFlag( currentAttr, FILE_ATTRIBUTE_DIRECTORY ) );

		DWORD newAttr = currentAttr;
		SetMaskedValue( newAttr, EDITABLE_ATTRIBUTE_MASK, static_cast<DWORD>( mfcFileStatus.m_attribute ) );

		if ( newAttr != currentAttr )
			if ( !::SetFileAttributes( m_fullPath.GetPtr(), newAttr ) )
				ThrowLastError();
	}

	void CFileState::ModifyFileTimes( const ::CFileStatus& mfcFileStatus, bool isDirectory ) const throws_( CFileException )
	{
		std::vector< std::pair<FILETIME, const FILETIME*> > times( _TimeFieldCount );		// pair.second: ptr not NULL when time is defined (!= 0)

		times[ ModifiedDate ].second = fs::thr::MakeFileTime( times[ ModifiedDate ].first, mfcFileStatus.m_mtime, mfcFileStatus.m_szFullName, fs::MfcExc );			// last modification time
		if ( times[ ModifiedDate ].second != nullptr )
		{
			times[ AccessedDate ].second = fs::thr::MakeFileTime( times[ AccessedDate ].first, mfcFileStatus.m_atime, mfcFileStatus.m_szFullName, fs::MfcExc );		// last access time
			times[ CreatedDate ].second = fs::thr::MakeFileTime( times[ CreatedDate ].first, mfcFileStatus.m_ctime, mfcFileStatus.m_szFullName, fs::MfcExc );		// create time

			fs::CScopedWriteableFile scopedWriteable( m_fullPath.GetPtr() );
			fs::CHandle file( ::CreateFile( m_fullPath.GetPtr(), GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ, nullptr, OPEN_EXISTING,
				isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,								// IMPORTANT: for access to directory vs file
				nullptr ) );

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
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_READONLY, "R", "READ-ONLY" ),		// CFile::readOnly
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_HIDDEN, "H", "HIDDEN" ),			// CFile::hidden
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_SYSTEM, "S", "SYSTEM" ),			// CFile::system
			FLAG_TAG_KEY_UI( CFile::volume, "V", "VOLUME" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_DIRECTORY, "D", "DIRECTORY" ),		// CFile::directory
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_ARCHIVE, "A", "ARCHIVE" ),			// CFile::archive
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_DEVICE, "d", "Device" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_NORMAL, "N", "NORMAL" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_TEMPORARY, "t", "Temporary" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_SPARSE_FILE, "s", "Sparse File" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_REPARSE_POINT, "r", "Reparse Point" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_COMPRESSED, "c", "Compressed" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_OFFLINE, "o", "Offline" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "n", "Not Content Indexed" ),
			FLAG_TAG_KEY_UI( FILE_ATTRIBUTE_ENCRYPTED, "e", "Encrypted" )
		};
		static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );

		return s_tags;
	}


	// CSecurityDescriptor implementation

	PSECURITY_DESCRIPTOR CSecurityDescriptor::ReadFromFile( const fs::CPath& filePath )
	{
		DWORD dwLength = 0;

		if ( ::GetFileSecurity( filePath.GetPtr(), DACL_SECURITY_INFORMATION, nullptr, dwLength, &dwLength ) )
		{
			m_securityDescriptor.resize( dwLength );

			PSECURITY_DESCRIPTOR pSecurityDescriptor = GetDescriptor();

			if ( ::GetFileSecurity( filePath.GetPtr(), DACL_SECURITY_INFORMATION, pSecurityDescriptor, dwLength, &dwLength ) )
				return pSecurityDescriptor;
		}

		m_securityDescriptor.clear();
		return nullptr;
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
