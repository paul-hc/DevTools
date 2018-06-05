
#include "stdafx.h"
#include "FileState.h"
#include "FlagTags.h"
#include "RuntimeException.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	const CFlagTags& GetTags_FileAttributes( void )
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


	// CFileState implementation

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

		::CFileStatus fileStatus;
		if ( !CFile::GetStatus( m_fullPath.GetPtr(), fileStatus ) )
			throw new mfc::CRuntimeException( str::Format( _T("Cannot access file status for file: %s"), m_fullPath.GetPtr() ) );

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
