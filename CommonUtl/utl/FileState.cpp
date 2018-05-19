
#include "stdafx.h"
#include "FileState.h"
#include "RuntimeException.h"
#include "TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CFileState implementation

	CFileState::CFileState( const ::CFileStatus* pFileStatus )
		: m_fullPath( pFileStatus->m_szFullName )
		, m_attributes( pFileStatus->m_attribute )
		, m_creationTime( pFileStatus->m_ctime )
		, m_modifTime( pFileStatus->m_mtime )
		, m_accessTime( pFileStatus->m_atime )
	{
	}

	CFileState::CFileState( const fs::CPath& path )
	{
		CFileState source;
		if ( source.ReadFromFile( path ) )
			*this = source;			// assign valid state
		else
		{	// assign path with invalid state
			m_fullPath = path;
			m_attributes = s_invalidAttributes;
		}
	}

	bool CFileState::ReadFromFile( const fs::CPath& path )
	{
		::CFileStatus fileStatus;

		Clear();
		m_fullPath = path;
		if ( !CFile::GetStatus( path.GetPtr(), fileStatus ) )
			return false;

		*this = CFileState( &fileStatus );
		return true;
	}

	void CFileState::WriteToFile( void ) const throws_( CFileException, mfc::CRuntimeException )
	{
		REQUIRE( IsValid() );

		::CFileStatus fileStatus;
		if ( !CFile::GetStatus( m_fullPath.GetPtr(), fileStatus ) )
			throw new mfc::CRuntimeException( str::Format( _T("Cannot acces file status for file: %s"), m_fullPath.GetPtr() ) );

		fileStatus.m_attribute = m_attributes;
		fileStatus.m_ctime = m_creationTime;
		fileStatus.m_mtime = m_modifTime;
		fileStatus.m_atime = m_accessTime;

		CFile::SetStatus( m_fullPath.GetPtr(), fileStatus );
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
}
