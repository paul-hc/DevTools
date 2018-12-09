
#include "stdafx.h"
#include "FileInfo.h"
#include "utl/ConsoleInputOutput.h"
#include "utl/FileSystem.h"


CFileInfo::CFileInfo( void )
	: m_attributes( INVALID_FILE_ATTRIBUTES )
	, m_lastModifyTime( 0 )
{
}

CFileInfo::CFileInfo( const CFileFind& foundFile )
	: m_filePath( foundFile.GetFilePath().GetString() )
	, m_attributes( fs::GetFileAttributes( foundFile ) )
	, m_lastModifyTime( 0 )
{
	foundFile.GetLastWriteTime( m_lastModifyTime );

	ENSURE( !path::HasTrailingSlash( m_filePath.GetPtr() ) );
}

CFileInfo::CFileInfo( const fs::CPath& filePath )
	: m_filePath( filePath )
	, m_attributes( INVALID_FILE_ATTRIBUTES )
	, m_lastModifyTime( 0 )
{
	if ( path::HasTrailingSlash( m_filePath.GetPtr() ) )
	{
		ASSERT( false );			// new style: not expected, calling code must not use unnecessary trailing backslashes
		m_filePath.SetBackslash( false );
	}

	CFileStatus fileStatus;
	if ( CFile::GetStatus( m_filePath.GetPtr(), fileStatus ) )		// [!] GetStatus() fails if a directory path with a trailing slash is passed
	{
		m_attributes = fileStatus.m_attribute;
		m_lastModifyTime = fileStatus.m_mtime;
	}
}

fs::CPath CFileInfo::AdjustPath( const TCHAR* pFullPath, path::TrailSlash trailSlash )
{
	fs::CPath fullPath = pFullPath;
	path::SetBackslash( fullPath.Ref(), trailSlash );
	return fullPath;
}

void CFileInfo::Clear( void )
{
	m_filePath.Clear();
	m_attributes = INVALID_FILE_ATTRIBUTES;
	m_lastModifyTime = CTime( 0 );
}

bool CFileInfo::Exist( void ) const
{
	ASSERT( m_attributes == INVALID_FILE_ATTRIBUTES || !m_filePath.IsEmpty() );
	return m_attributes != INVALID_FILE_ATTRIBUTES;
}

bool CFileInfo::DirPathExist( void ) const
{
	return fs::IsValidDirectory( m_filePath.GetParentPath().GetPtr() );
}
