
#include "stdafx.h"
#include "FileState.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "RuntimeException.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	CFileState::CFileState( const ::CFileStatus* pFileStatus )
		: m_fullPath( pFileStatus->m_szFullName )
		, m_attributes( pFileStatus->m_attribute )
		, m_creationTime( pFileStatus->m_ctime )
		, m_modifTime( pFileStatus->m_mtime )
		, m_accessTime( pFileStatus->m_atime )
	{
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

		ModifyFileStatus( newStatus );
	}

	bool CFileState::operator==( const CFileState& right ) const
	{
		return
			m_fullPath == right.m_fullPath &&
			m_attributes == right.m_attributes &&
			m_creationTime == right.m_creationTime &&
			m_modifTime == right.m_modifTime &&
			m_accessTime == right.m_accessTime;
	}

	const CEnumTags& CFileState::GetTags_TimeField( void )
	{
		static const CEnumTags tags( _T("Created Date|Modified Date|Accessed Date"), _T("C|M|A") );
		return tags;
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

	const CFlagTags& CFileState::GetTags_FileAttributes( void )
	{
		static const CFlagTags::FlagDef flagDefs[] =
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
		static const std::tstring uiTags = _T("READ-ONLY|HIDDEN|SYSTEM|VOLUME|DIRECTORY|ARCHIVE|Device|NORMAL|Temporary|Sparse File|Reparse Point|Compressed|Offline|Not Content Indexed|Encrypted");
		static const CFlagTags tags( flagDefs, COUNT_OF( flagDefs ), uiTags );
		return tags;
	}


	void CFileState::ModifyFileStatus( const ::CFileStatus& newStatus ) const throws_( CFileException )
	{
		// Verbatim from CFile::SetStatus( LPCTSTR lpszFileName, const CFileStatus& status ).
		//	works for file using FILE_ATTRIBUTE_NORMAL - like CFile::SetStatus()
		//	works for directory using FILE_FLAG_BACKUP_SEMANTICS - new

		DWORD currentAttr = ::GetFileAttributes( m_fullPath.GetPtr() );

		if ( INVALID_FILE_ATTRIBUTES == currentAttr )
			ThrowLastError();

		DWORD newAttr = static_cast< DWORD >( newStatus.m_attribute );

		if ( newAttr != currentAttr && HasFlag( currentAttr, CFile::readOnly ) )		// currently readonly?
			if ( !::SetFileAttributes( m_fullPath.GetPtr(), newAttr ) )					// this way we will be able to modify the times assuming the caller changed the file for a read-only file
				ThrowLastError();

		ModifyFileTimes( newStatus, HasFlag( currentAttr, FILE_ATTRIBUTE_DIRECTORY ) );

		if ( newAttr != currentAttr && !HasFlag( currentAttr, CFile::readOnly ) )
			if ( !::SetFileAttributes( m_fullPath.GetPtr(), newAttr ) )
				ThrowLastError();
	}

	void CFileState::ModifyFileTimes( const ::CFileStatus& newStatus, bool isDirectory ) const throws_( CFileException )
	{
		std::vector< std::pair< FILETIME, const FILETIME* > > times( _TimeFieldCount );		// pair.second: ptr not NULL when time is defined (!= 0)

		times[ ModifiedDate ].second = MakeFileTime( times[ ModifiedDate ].first, newStatus.m_mtime );			// last modification time
		if ( times[ ModifiedDate ].second != NULL )
		{
			times[ AccessedDate ].second = MakeFileTime( times[ AccessedDate ].first, newStatus.m_atime );		// last access time
			times[ CreatedDate ].second = MakeFileTime( times[ CreatedDate ].first, newStatus.m_ctime );		// create time

			HANDLE hFile = ::CreateFile( m_fullPath.GetPtr(), GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ, NULL, OPEN_EXISTING,
				isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,								// IMPORTANT: for access to directory vs file
				NULL );

			if ( INVALID_HANDLE_VALUE == hFile )
				ThrowLastError();

			if ( !::SetFileTime( hFile, times[ CreatedDate ].second, times[ AccessedDate ].second, times[ ModifiedDate ].second ) )
			{
				const DWORD lastError = ::GetLastError();
				::CloseHandle( hFile );
				ThrowLastError( lastError );
			}

			if ( !::CloseHandle( hFile ) )
				ThrowLastError();
		}
	}

	FILETIME* CFileState::MakeFileTime( FILETIME& rOutFileTime, const CTime& time ) const throws_( CFileException )
	{
		if ( 0 == time.GetTime() )
			return NULL;

		SYSTEMTIME sysTime;
		sysTime.wYear = (WORD)time.GetYear();
		sysTime.wMonth = (WORD)time.GetMonth();
		sysTime.wDay = (WORD)time.GetDay();
		sysTime.wHour = (WORD)time.GetHour();
		sysTime.wMinute = (WORD)time.GetMinute();
		sysTime.wSecond = (WORD)time.GetSecond();
		sysTime.wMilliseconds = 0;

		// convert system time to local file time
		FILETIME localTime;
		if ( !::SystemTimeToFileTime( &sysTime, &localTime ) )
			CFileException::ThrowOsError( (LONG)::GetLastError() );

		// convert local file time to UTC file time
		if ( !::LocalFileTimeToFileTime( &localTime, &rOutFileTime ) )
			CFileException::ThrowOsError( (LONG)::GetLastError() );

		return &rOutFileTime;
	}
}
