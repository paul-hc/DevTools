// Copyleft 2004 Paul H. Cocoveanu
//
#include "stdafx.h"
#include "TransferItem.h"
#include <iostream>


CTransferItem::CTransferItem( const CFileFind& sourceFinder, const std::tstring& rootSourceDirPath, const std::tstring& rootTargetDirPath )
	: m_sourceFileInfo( sourceFinder )
	, m_targetFileInfo( MakeTargetFilePath( m_sourceFileInfo.m_fullPath.Get(), rootSourceDirPath, rootTargetDirPath ) )
{
}

CTransferItem::CTransferItem( const std::tstring& sourceFilePath, const std::tstring& rootSourceDirPath, const std::tstring& rootTargetDirPath )
	: m_sourceFileInfo( sourceFilePath )
	, m_targetFileInfo( MakeTargetFilePath( m_sourceFileInfo.m_fullPath.Get(), rootSourceDirPath, rootTargetDirPath ) )
{
}

CTransferItem::~CTransferItem()
{
}

std::ostream& CTransferItem::Print( std::ostream& os, FileAction fileAction, bool showTimestamp /*= false*/ ) const
{
	static const char timestampFormat[] = "%b %d, %Y";
	static const char linePrefix[] = "  ";

	if ( FileCopy == fileAction || FileMove == fileAction )
	{
		os << linePrefix;
		if ( fileAction == FileMove )
			os << "[-] ";
		os << m_sourceFileInfo.m_fullPath.Get();
		if ( showTimestamp && m_sourceFileInfo.Exist() )
			os << " (" << m_sourceFileInfo.m_lastModifiedTimestamp.Format( timestampFormat ) << ")";
		os << " ->" << std::endl;
	}

	os << linePrefix;
	switch ( fileAction )
	{
		case FileMove: os << "[+] "; break;
		case TargetFileRemove: os << "[-] "; break;
	}
	os << m_targetFileInfo.m_fullPath.Get();
	if ( showTimestamp && m_targetFileInfo.Exist() )
		os << " (" << m_targetFileInfo.m_lastModifiedTimestamp.Format( timestampFormat ) << ")";
	os << std::endl;
	return os;
}

bool CTransferItem::Transfer( FileAction fileAction )
{
	enum { NotExecuted, Executed, Error } result = NotExecuted;

	if ( m_sourceFileInfo.IsRegularFile() )
	{
		if ( m_targetFileInfo.IsReadOnly() )
		{
			m_targetFileInfo.m_attributes &= ~FILE_ATTRIBUTE_READONLY;
			result = SetFileAttributes( m_targetFileInfo.m_fullPath.GetPtr(), m_targetFileInfo.m_attributes ) ? Executed : Error;

			ASSERT( Error == result || m_targetFileInfo.m_attributes == GetFileAttributes( m_targetFileInfo.m_fullPath.GetPtr() ) );
		}

		if ( result != Error )
			switch ( fileAction )
			{
				case FileCopy:
					ASSERT( m_sourceFileInfo.Exist() );
					result = ::CopyFile( m_sourceFileInfo.m_fullPath.GetPtr(), m_targetFileInfo.m_fullPath.GetPtr(), FALSE ) ? Executed : Error;
					break;
				case FileMove:
					ASSERT( m_sourceFileInfo.Exist() );
					result = ::MoveFileEx( m_sourceFileInfo.m_fullPath.GetPtr(), m_targetFileInfo.m_fullPath.GetPtr(),
										   MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) ? Executed : Error;
					break;
				case TargetFileRemove:
					if ( m_targetFileInfo.Exist() )
						result = ::DeleteFile( m_targetFileInfo.m_fullPath.GetPtr() ) ? Executed : Error;
					break;
			}
	}

	if ( Error == result )
	{
		static const char* pFileActionLabel[] = { "copy", "move", "remove" };			// indexed by FileAction

		std::cerr << " * Couldn't " << pFileActionLabel[ fileAction ] << " '" << m_sourceFileInfo.m_fullPath.Get() << "' !" << std::endl;
	}

	return Executed == result;
}

std::tstring CTransferItem::MakeTargetFilePath( const std::tstring& sourceFullPath, const std::tstring& rootSourceDirPath,
											    const std::tstring& rootTargetDirPath )
{
	fs::CPathParts srcParts( sourceFullPath );
	std::tstring sourceDirPath = srcParts.GetDirPath();

	std::tstring targetRelPath = path::StripCommonPrefix( sourceDirPath.c_str(), rootSourceDirPath.c_str() );
	std::tstring targetDirPath( path::Combine( rootTargetDirPath.c_str(), targetRelPath.c_str() ) );

	std::tstring targetFullPath( path::Combine( targetDirPath.c_str(), path::FindFilename( sourceFullPath.c_str() ) ) );
	return targetFullPath;
}
