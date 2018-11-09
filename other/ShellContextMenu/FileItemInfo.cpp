
#include "stdafx.h"
#include "FileItemInfo.h"
#include "utl/FileSystem.h"
#include "utl/ShellTypes.h"
#include <shlwapi.h>			// for StrFormatByteSize64()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileItemInfo::CFileItemInfo( const std::tstring& fullPath, const CFileFind* pFoundFile, const TCHAR* pFilename /*= NULL*/ )
	: m_fullPath( fullPath )
	, m_fileAttributes( UINT_MAX )
	, m_fileSize( 0 )
{
	StoreFilename( pFilename, pFoundFile );

	if ( pFoundFile != NULL )
	{
		m_fileAttributes = fs::GetFindData( *pFoundFile )->dwFileAttributes;
		m_fileSize = pFoundFile->GetLength();
		pFoundFile->GetLastAccessTime( m_modifiedTime );
	}
}

void CFileItemInfo::StoreFilename( const TCHAR* pFilename, const CFileFind* pFoundFile )
{
	if ( !str::IsEmpty( pFilename ) )
		m_filename = pFilename;
	else if ( pFoundFile != NULL )
		m_filename = pFoundFile->GetFileName().GetString();
	else
		m_filename = path::FindFilename( m_fullPath.c_str() );
}

CFileItemInfo* CFileItemInfo::MakeItem( const std::tstring& filePath )
{
	if ( path::IsRoot( filePath.c_str() ) )
	{	// CFileFind doesn't work for root directories!
		CFileItemInfo* pRootItem = new CFileItemInfo( filePath, NULL );
		pRootItem->m_fileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		pRootItem->m_filename = _T(".");
		return pRootItem;
	}

	CFileFind finder;
	if ( !finder.FindFile( filePath.c_str() ) )
		return NULL;				// error accessing the file

	finder.FindNextFile();
	return MakeItem( finder );
}

CFileItemInfo* CFileItemInfo::MakeItem( const CFileFind& foundFile )
{
	return new CFileItemInfo( foundFile.GetFilePath().GetString(), &foundFile );
}

CFileItemInfo* CFileItemInfo::MakeParentDirItem( const std::tstring& dirPath )
{
	if ( path::IsRoot( dirPath.c_str() ) )
		return NULL;			// no parent directory

	std::tstring parentDirPath = path::GetParentPath( dirPath.c_str(), path::RemoveSlash );
	CFileItemInfo* pParentDirItem = MakeItem( parentDirPath );

	pParentDirItem->m_filename = _T("..");
	return pParentDirItem;
}


// CFileItemInfo::CDetails implementation

void CFileItemInfo::CDetails::LazyInit( const CFileItemInfo* pItem )
{
	ASSERT_PTR( pItem );
	ASSERT( pItem->IsValid() );

	if ( IsValid() )
		return;

	SHFILEINFO fileInfo;
	::SHGetFileInfo( pItem->m_fullPath.c_str(), NULL, &fileInfo, sizeof( fileInfo ), SHGFI_SYSICONINDEX | SHGFI_TYPENAME );

	m_iconIndex = fileInfo.iIcon;
	m_typeName = fileInfo.szTypeName;

	m_fileAttributesText = FormatAttributes( pItem->m_fileAttributes );

	if ( !pItem->IsDirectory() )
		m_fileSizeText = shell::FormatByteSize( pItem->m_fileSize );

	m_modifiedDateText = pItem->m_modifiedTime.Format( _T("%x %X") );
}

std::tstring CFileItemInfo::CDetails::FormatAttributes( DWORD fileAttributes )
{
	static const struct { DWORD m_attribute; const TCHAR m_char; } s_attrTags[] =
	{
		{ FILE_ATTRIBUTE_ARCHIVE, _T('A') },
		{ FILE_ATTRIBUTE_HIDDEN, _T('H') },
		{ FILE_ATTRIBUTE_READONLY, _T('R') },
		{ FILE_ATTRIBUTE_SYSTEM, _T('S') },
		{ FILE_ATTRIBUTE_COMPRESSED, _T('C') },
		{ FILE_ATTRIBUTE_ENCRYPTED, _T('E') },
		{ FILE_ATTRIBUTE_DIRECTORY, _T('D') },
	};

	std::tstring text; text.reserve( 10 );

	for ( unsigned int i = 0; i != _countof( s_attrTags ); ++i )
		if ( HasFlag( fileAttributes, s_attrTags[ i ].m_attribute ) )
			text += s_attrTags[ i ].m_char;

	return text;
}
