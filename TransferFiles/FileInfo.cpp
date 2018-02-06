
#include "stdafx.h"
#include "FileInfo.h"
#include "InputOutput.h"
#include "utl/FileSystem.h"


CFileInfo::CFileInfo( void )
	: m_attributes( INVALID_FILE_ATTRIBUTES )
	, m_lastModifiedTimestamp( 0 )
{
}

CFileInfo::CFileInfo( const CFileFind& foundFile )
	: m_fullPath( AdjustPath( foundFile.GetFilePath(), foundFile.IsDirectory() ? path::AddSlash : path::PreserveSlash ) )
	, m_attributes( ExtractFileAttributes( foundFile ) )
	, m_lastModifiedTimestamp( 0 )
{
	foundFile.GetLastWriteTime( m_lastModifiedTimestamp );
}

CFileInfo::CFileInfo( const std::tstring& fullPath )
	: m_fullPath( fullPath )
	, m_attributes( INVALID_FILE_ATTRIBUTES )
	, m_lastModifiedTimestamp( 0 )
{
	CFileStatus fileStatus;
	std::tstring fsPath = AdjustPath( fullPath.c_str(), path::RemoveSlash );		// [!] GetStatus() fails if a directory path with a trailing slash is passed

	if ( CFile::GetStatus( fsPath.c_str(), fileStatus ) )
	{
		m_attributes = fileStatus.m_attribute;
		m_lastModifiedTimestamp = fileStatus.m_mtime;
	}
}

std::tstring CFileInfo::AdjustPath( const TCHAR* pFullPath, path::TrailSlash trailSlash )
{
	std::tstring fullPath = pFullPath;
	path::SetBackslash( fullPath, trailSlash );
	return fullPath;
}

struct FriendlyFileFind : CFileFind { using CFileFind::m_pFoundInfo; };

DWORD CFileInfo::ExtractFileAttributes( const CFileFind& foundFile )
{
	const FriendlyFileFind* pFriendlyFileFinder = reinterpret_cast< const FriendlyFileFind* >( &foundFile );
	const WIN32_FIND_DATA* pFoundInfo = reinterpret_cast< const WIN32_FIND_DATA* >( pFriendlyFileFinder->m_pFoundInfo );

	ASSERT_PTR( pFoundInfo );				// must have already found have a file (current file)
	return pFoundInfo->dwFileAttributes;
}

void CFileInfo::Clear( void )
{
	m_fullPath.Clear();
	m_attributes = INVALID_FILE_ATTRIBUTES;
	m_lastModifiedTimestamp = CTime( 0 );
}

bool CFileInfo::Exist( void ) const
{
	ASSERT( m_attributes == INVALID_FILE_ATTRIBUTES || !m_fullPath.IsEmpty() );
	return m_attributes != INVALID_FILE_ATTRIBUTES;
}

bool CFileInfo::DirPathExist( void ) const
{
	return fs::IsValidDirectory( m_fullPath.GetParentPath().GetPtr() );
}
