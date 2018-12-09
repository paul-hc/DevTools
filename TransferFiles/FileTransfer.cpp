
#include "stdafx.h"
#include "FileTransfer.h"
#include "XferOptions.h"
#include "utl/ContainerUtilities.h"
#include "utl/FlagTags.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include <iostream>
#include <algorithm>


CFileTransfer::CFileTransfer( const CXferOptions* pOptions )
	: m_pOptions( pOptions )
	, m_fileCount( 0 )
	, m_createdDirCount( 0 )

	, m_uqOverrideReadOnly( m_pOptions->m_userPrompt != PromptNever && !m_pOptions->m_overrideReadOnlyFiles )
	, m_uqOverrideFiles( m_pOptions->m_userPrompt != PromptNever )
	, m_uqCreateDirs( _T("Create target directory"), m_pOptions->m_userPrompt != PromptNever, io::YesNoAll )
{
	ASSERT_PTR( m_pOptions );
}

CFileTransfer::~CFileTransfer()
{
	utl::ClearOwningContainer( m_transferItems, func::DeleteSecond() );
}

int CFileTransfer::Run( void )
{
	SearchSourceFiles( m_pOptions->m_sourceDirPath );
	return Transfer();
}

int CFileTransfer::Transfer( void )
{
	m_fileCount = 0;

	std::auto_ptr< CBackupInfo > pBackupInfo;
	if ( m_pOptions->m_pBackupDirPath.get() != NULL )
		pBackupInfo.reset( new CBackupInfo( m_pOptions ) );

	for ( TransferItemMap::const_iterator itItem = m_transferItems.begin(); itItem != m_transferItems.end(); ++itItem )
	{
		CTransferItem* pItem = itItem->second;

		if ( ExecuteTransfer == m_pOptions->m_transferMode )
		{
			// do the actual file transfer
			if ( CanAlterTargetFile( *pItem ) )
				if ( !m_pOptions->m_justCreateTargetDirs )
					if ( pItem->Transfer( m_pOptions->m_fileAction, pBackupInfo.get() ) )
					{
						++m_fileCount;
						pItem->Print( std::cout, m_pOptions->m_fileAction, m_pOptions->m_filterByTimestamp ) << std::endl;
					}
		}
		else
		{	// just display SOURCE or TARGET
			if ( JustDisplaySourceFile == m_pOptions->m_transferMode )
				std::cout << pItem->m_sourceFileInfo.m_filePath.Get();
			else
				std::cout << pItem->m_targetFileInfo.m_filePath.Get();

			if ( pItem->m_sourceFileInfo.IsDirectory() )
				++m_createdDirCount;
			else
				++m_fileCount;

			std::cout << std::endl;
		}
	}

	return m_fileCount;
}

void CFileTransfer::SearchSourceFiles( const fs::CPath& dirPath )
{
	ASSERT( fs::IsValidDirectory( dirPath.GetPtr() ) );

	if ( m_pOptions->m_justCreateTargetDirs )
		AddTransferItem( new CTransferItem( dirPath, m_pOptions->m_sourceDirPath, m_pOptions->m_targetDirPath ) );
	else
		fs::EnumFiles( this, dirPath.GetPtr(), m_pOptions->m_searchSpecs.c_str(), m_pOptions->m_recurseSubDirectories ? Deep : Shallow );
}

void CFileTransfer::AddFoundSubDir( const TCHAR* pSubDirPath )
{
	pSubDirPath;
}

void CFileTransfer::AddFile( const CFileFind& foundFile )
{
	AddTransferItem( new CTransferItem( foundFile, m_pOptions->m_sourceDirPath, m_pOptions->m_targetDirPath ) );
}

bool CFileTransfer::AddTransferItem( CTransferItem* pTransferItem )
{
	ASSERT_PTR( pTransferItem );
	std::auto_ptr< CTransferItem > itemPtr( pTransferItem );			// take ownership of the new item

	if ( !m_pOptions->PassFilter( *pTransferItem ) )
		return false;

	TransferItemMap::const_iterator itFoundItem = m_transferItems.find( pTransferItem->m_sourceFileInfo.m_filePath );
	if ( itFoundItem != m_transferItems.end() )
		return false;			// reject duplicates

	m_transferItems.insert( std::make_pair( pTransferItem->m_sourceFileInfo.m_filePath, itemPtr.release() ) );
	return true;
}

bool CFileTransfer::CanAlterTargetFile( const CTransferItem& item )
{
	if ( item.m_targetFileInfo.Exist() )
	{
		static const char* s_promptFileAction[] = { "Overwrite", "Overwrite", "Remove" };		// indexed by FileAction

		if ( item.m_targetFileInfo.IsRegularFile() )
			if ( m_uqOverrideReadOnly.MustAsk() && item.m_targetFileInfo.IsProtected() )
			{
				std::cout << s_promptFileAction[ m_pOptions->m_fileAction ] << FormatProtectedFileAttr( item.m_targetFileInfo.m_attributes ) << _T(" ");

				return m_uqOverrideReadOnly.Ask( arg::Enquote( item.m_targetFileInfo.m_filePath ) );
			}
			else if ( m_uqOverrideFiles.MustAsk() )
			{
				std::cout << s_promptFileAction[ m_pOptions->m_fileAction ] << _T(" ");

				return m_uqOverrideFiles.Ask( arg::Enquote( item.m_targetFileInfo.m_filePath ) );
			}
	}
	else
		switch ( m_pOptions->m_fileAction )
		{
			case FileCopy:
			case FileMove:
				return CreateTargetDirectory( item );
			case TargetFileDelete:		// (!) avoid creating target directory when deleting target
				break;
		}

	return true;
}

bool CFileTransfer::CreateTargetDirectory( const CTransferItem& item )
{
	fs::CPath targetDirPath = item.m_targetFileInfo.m_filePath;

	if ( item.m_sourceFileInfo.IsRegularFile() )			// SRC (physically present) is a file?
		targetDirPath = targetDirPath.GetParentPath();		// take the target parent directory

	switch ( m_uqCreateDirs.Acquire( targetDirPath ) )
	{
		case fs::FoundExisting:
			break;
		case fs::Created:
			++m_createdDirCount;
			if ( m_uqCreateDirs.IsYesAll() )
				std::cout << "Created directory: " << targetDirPath << std::endl;
			break;
		case fs::CreationError:
			return false;			// creation already rejected by user -> cannot transfer file
	}

	return true;
}

std::tstring CFileTransfer::FormatProtectedFileAttr( DWORD fileAttr )
{
	static const CFlagTags::FlagDef flagDefs[] =
	{
		{ FILE_ATTRIBUTE_READONLY, _T("READ-ONLY") },
		{ FILE_ATTRIBUTE_HIDDEN, _T("HIDDEN") },
		{ FILE_ATTRIBUTE_SYSTEM, _T("SYSTEM") }
	};
	static const CFlagTags protectedFileAttrTags( flagDefs, COUNT_OF( flagDefs ) );

	std::tstring text;
	stream::Tag( text, protectedFileAttrTags.FormatUi( fileAttr, _T("/") ), _T(" ") );
	return text;
}

std::ostream& CFileTransfer::PrintStatistics( std::ostream& os ) const
{
	static const char* pFileActionLabel[] = { " copied", " moved", " removed" };			// indexed by FileAction
	const char* pActionPrefix = m_pOptions->m_transferMode != ExecuteTransfer ? " would be" : "";

	os << m_fileCount << " file(s)" << pActionPrefix << pFileActionLabel[ m_pOptions->m_fileAction ];

	if ( m_createdDirCount != 0 )
		os
			<< _T(", ") << m_createdDirCount
			<< ( 1 == m_createdDirCount ? " directory" : " directories" ) << pActionPrefix << " created";

	os << "." << std::endl;
	return os;
}
