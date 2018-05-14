
#include "stdafx.h"
#include "FileInfo.h"
#include "RuntimeException.h"
#include "TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CFileInfo implementation

	const TCHAR CFileInfo::s_sep[] = _T("|");

	CFileInfo::CFileInfo( const CFileStatus& fileStatus )
		: m_fullPath( fileStatus.m_szFullName )
		, m_attributes( fileStatus.m_attribute )
		, m_creationTime( fileStatus.m_ctime )
		, m_modifTime( fileStatus.m_mtime )
		, m_accessTime( fileStatus.m_atime )
	{
	}

	bool CFileInfo::GetFileInfo( const TCHAR* pSrcFilePath )
	{
		CFileStatus fileStatus;

		Clear();
		m_fullPath.Set( pSrcFilePath );
		if ( !CFile::GetStatus( pSrcFilePath, fileStatus ) )
			return false;

		*this = CFileInfo( fileStatus );
		return true;
	}

	void CFileInfo::SetFileInfo( const TCHAR* pDestFilePath /*= NULL*/ ) const throws_( CFileException, mfc::CRuntimeException )
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

	std::tstring CFileInfo::Format( void ) const
	{
		std::vector< std::tstring > parts;
		if ( !IsEmpty() )
		{
			parts.push_back( m_fullPath.Get() );
			parts.push_back( str::Format( _T("0x%08X"), m_attributes ) );
			parts.push_back( time_utl::FormatTimestamp( m_creationTime ) );
			parts.push_back( time_utl::FormatTimestamp( m_modifTime ) );
			parts.push_back( time_utl::FormatTimestamp( m_accessTime ) );
		}
		return str::Join( parts, s_sep );
	}

	bool CFileInfo::Parse( const std::tstring& text )
	{
		std::vector< std::tstring > parts;
		str::Split( parts, text.c_str(), s_sep );
		if ( parts.size() != _FieldCount )
		{
			Clear();
			return false;
		}

		m_fullPath.Set( parts[ FullPath ] );

		unsigned int attributes;
		if ( 1 == _tscanf( parts[ Attributes ].c_str(), _T("0x%08X"), &attributes ) )
			m_attributes = static_cast< BYTE >( attributes );

		m_creationTime = time_utl::ParseTimestamp( parts[ CreationTime ] );
		m_modifTime = time_utl::ParseTimestamp( parts[ ModifTime ] );
		m_accessTime = time_utl::ParseTimestamp( parts[ AccessTime ] );
		return true;
	}
}
