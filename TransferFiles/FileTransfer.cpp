// Copyleft 2004-2016 Paul H. Cocoveanu

#include "stdafx.h"
#include "FileTransfer.h"
#include "XferOptions.h"
#include "InputOutput.h"
#include "utl/ContainerUtilities.h"
#include "utl/FlagTags.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include <iostream>
#include <algorithm>


CFileTransfer::CFileTransfer( const CXferOptions& options )
	: m_options( options )
	, m_fileCount( 0 )
	, m_createdDirCount( 0 )
{
}

CFileTransfer::~CFileTransfer()
{
	utl::ClearOwningContainer( m_xferNodesMap, func::DeleteSecond() );
}

int CFileTransfer::Run( void )
{
	SearchSourceFiles( m_options.m_sourceDirPath );
	return Transfer();
}

int CFileTransfer::Transfer( void )
{
	m_fileCount = 0;

	for ( TransferItemMap::const_iterator itXfer = m_xferNodesMap.begin(); itXfer != m_xferNodesMap.end(); ++itXfer )
		if ( ExecuteTransfer == m_options.m_transferMode )
		{	// do the actual file transfer
			if ( CanAlterTargetFile( *itXfer->second ) )
				if ( !m_options.m_justCreateTargetDirs )
					if ( itXfer->second->Transfer( m_options.m_fileAction ) )
					{
						++m_fileCount;
						itXfer->second->Print( std::cout, m_options.m_fileAction, m_options.m_filterByTimestamp ) << std::endl;
					}
		}
		else
		{	// just display SOURCE or TARGET
			if ( JustDisplaySourceFile == m_options.m_transferMode )
				std::cout << itXfer->second->m_sourceFileInfo.m_fullPath.Get();
			else
				std::cout << itXfer->second->m_targetFileInfo.m_fullPath.Get();

			if ( itXfer->second->m_sourceFileInfo.IsDirectory() )
				++m_createdDirCount;
			else
				++m_fileCount;

			std::cout << std::endl;
		}

	return m_fileCount;
}

void CFileTransfer::SearchSourceFiles( const std::tstring& dirPath )
{
	ASSERT( fs::IsValidDirectory( dirPath.c_str() ) );

	if ( m_options.m_justCreateTargetDirs )
		AddTransferItem( new CTransferItem( dirPath, m_options.m_sourceDirPath, m_options.m_targetDirPath ) );
	else
		fs::EnumFiles( this, dirPath.c_str(), m_options.m_searchSpecs.c_str(), m_options.m_recurseSubDirectories ? Deep : Shallow );
}

void CFileTransfer::AddFoundSubDir( const TCHAR* pSubDirPath )
{
	pSubDirPath;
}

void CFileTransfer::AddFile( const CFileFind& foundFile )
{
	AddTransferItem( new CTransferItem( foundFile, m_options.m_sourceDirPath, m_options.m_targetDirPath ) );
}

bool CFileTransfer::AddTransferItem( CTransferItem* pTransferItem )
{
	ASSERT_PTR( pTransferItem );
	std::auto_ptr< CTransferItem > itemPtr( pTransferItem );			// take ownership of the new node

	if ( !m_options.PassFilter( *pTransferItem ) )
		return false;

	TransferItemMap::const_iterator itFoundNode = m_xferNodesMap.find( pTransferItem->m_sourceFileInfo.m_fullPath );
	if ( itFoundNode != m_xferNodesMap.end() )
		return false;			// reject duplicates

	m_xferNodesMap.insert( std::make_pair( pTransferItem->m_sourceFileInfo.m_fullPath, itemPtr.release() ) );
	return true;
}

bool CFileTransfer::CanAlterTargetFile( const CTransferItem& node )
{
	if ( node.m_targetFileInfo.Exist() )
	{
		static bool queryOverrideReadOnly = !m_options.m_overrideReadOnlyFiles && m_options.m_userPrompt != PromptNever;
		static bool queryOverrideFiles = m_options.m_userPrompt != PromptNever;

		static const char* fileActionPrompt[] = { "Overwrite", "Overwrite", "Remove" }; // indexed by FileAction

		if ( node.m_targetFileInfo.IsRegularFile() )
			if ( queryOverrideReadOnly && node.m_targetFileInfo.IsProtected() )
			{
				for ( ;; )
				{
					std::tstring attributeText;

					std::cout <<
						fileActionPrompt[ m_options.m_fileAction ] <<
						FormatProtectedFileAttr( node.m_targetFileInfo.m_attributes ) <<
						" '" << node.m_targetFileInfo.m_fullPath.Get() << "' ? (Yes/No/All): ";
					switch ( io::InputUserKey() )
					{
						case 'Y': return true;
						case 'N': return false;
						case 'A': queryOverrideReadOnly = false; return true;
					}
				}
			}
			else if ( queryOverrideFiles )
			{
				for ( ;; )
				{
					std::cout <<
						fileActionPrompt[ m_options.m_fileAction ] <<
						" '" << node.m_targetFileInfo.m_fullPath.Get() << "' ? (Yes/No/All): ";
					switch ( io::InputUserKey() )
					{
						case 'Y': return true;
						case 'N': return false;
						case 'A': queryOverrideFiles = false; return true;
					}
				}
			}
	}
	else if ( FileCopy == m_options.m_fileAction || FileMove == m_options.m_fileAction )
	{	// (!) avoid creating target directory for target deletion
		std::tstring targetDirPath = node.m_targetFileInfo.m_fullPath.Get();

		if ( node.m_sourceFileInfo.IsRegularFile() )
			targetDirPath = path::GetParentPath( targetDirPath.c_str(), path::RemoveSlash );

		if ( m_failedTargetDirs.find( targetDirPath ) == m_failedTargetDirs.end() )
		{
			static bool queryCreateDirs = m_options.m_userPrompt != PromptNever;

			if ( !fs::FileExist( targetDirPath.c_str() ) )
			{
				for ( bool goodKeyStroke = false; queryCreateDirs && !goodKeyStroke; )
				{
					std::cout << "Create directory '" << targetDirPath << "' ? (Yes/No/All): ";
					switch ( io::InputUserKey() )
					{
						case 'A':
							queryCreateDirs = false;
						case 'Y':
							goodKeyStroke = true;
							break;
						case 'N':
							m_failedTargetDirs.insert( targetDirPath );
							return false;
					}
				}

				try
				{
					if ( !fs::IsValidDirectory( targetDirPath.c_str() ) )
						if ( fs::CreateDirPath( targetDirPath.c_str() ) )		// create target directory
						{
							++m_createdDirCount;
							if ( !queryCreateDirs )
								std::cout << "Creating directory: " << targetDirPath << std::endl;
						}
						else
							throw CRuntimeException( str::Format( _T("Error: could not create directory: '%s'"), targetDirPath.c_str() ) );
				}
				catch ( const CRuntimeException& exc )
				{
					std::cerr << exc.what() << std::endl;
					m_failedTargetDirs.insert( targetDirPath );
					return false;
				}
			}
		}
		else
			return false;			// no target directory created -> cannot transfer file
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
	const char* pActionPrefix = m_options.m_transferMode != ExecuteTransfer ? " would be" : "";

	os << m_fileCount << " file(s)" << pActionPrefix << pFileActionLabel[ m_options.m_fileAction ];

	if ( m_createdDirCount != 0 )
		os
			<< _T(", ") << m_createdDirCount
			<< ( 1 == m_createdDirCount ? " directory" : " directories" ) << pActionPrefix << " created";

	os << "." << std::endl;
	return os;
}
