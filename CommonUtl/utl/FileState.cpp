
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

	CFileState::CFileState( const CFileStatus& fileStatus )
		: m_fullPath( fileStatus.m_szFullName )
		, m_attributes( fileStatus.m_attribute )
		, m_creationTime( fileStatus.m_ctime )
		, m_modifTime( fileStatus.m_mtime )
		, m_accessTime( fileStatus.m_atime )
	{
	}

	bool CFileState::GetFileState( const TCHAR* pSrcFilePath )
	{
		CFileStatus fileStatus;

		Clear();
		m_fullPath.Set( pSrcFilePath );
		if ( !CFile::GetStatus( pSrcFilePath, fileStatus ) )
			return false;

		*this = CFileState( fileStatus );
		return true;
	}

	void CFileState::SetFileState( const TCHAR* pDestFilePath /*= NULL*/ ) const throws_( CFileException, mfc::CRuntimeException )
	{
		if ( NULL == pDestFilePath )
			pDestFilePath = m_fullPath.GetPtr();

		CFileStatus fileStatus;
		if ( !CFile::GetStatus( pDestFilePath, fileStatus ) )
			throw new mfc::CRuntimeException( str::Format( _T("Cannot acces file status for file: %s"), pDestFilePath ) );

		fileStatus.m_attribute = m_attributes;
		fileStatus.m_ctime = m_creationTime;
		fileStatus.m_mtime = m_modifTime;
		fileStatus.m_atime = m_accessTime;

		CFile::SetStatus( pDestFilePath, fileStatus );
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
