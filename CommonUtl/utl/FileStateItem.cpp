
#include "stdafx.h"
#include "FileStateItem.h"
#include "SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileStateItem implementation

CFileStateItem::CFileStateItem( const fs::CFileState& fileState )
	: CPathItemBase( fileState.m_fullPath )
	, m_fileState( fileState )
{
}

CFileStateItem::CFileStateItem( const CFileFind& foundFile )
	: CPathItemBase( fs::CPath( foundFile.GetFilePath().GetString() ) )
	, m_fileState( foundFile )
{
}

void CFileStateItem::SetFilePath( const fs::CPath& filePath )
{
	__super::SetFilePath( filePath );

	if ( filePath.FileExist() )
		m_fileState = fs::CFileState::ReadFromFile( filePath );
	else
		m_fileState.Clear();
}

void CFileStateItem::Stream( CArchive& archive )
{
	// skip calling __super::Stream() since the file path is serialized with the m_fileState
	if ( archive.IsStoring() )
		archive << m_fileState;
	else
	{
		archive >> m_fileState;
		ResetFilePath( m_fileState.m_fullPath );
	}
}
