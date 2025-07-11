
#include "pch.h"
#include "RenameItem.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ren
{
	bool MakePairsFromItems( CPathRenamePairs* pOutRenamePairs, const std::vector<CRenameItem*>& renameItems )
	{
		pOutRenamePairs->Clear();

		for ( std::vector<CRenameItem*>::const_iterator itItem = renameItems.begin(); itItem != renameItems.end(); ++itItem )
			pOutRenamePairs->AddPair( (*itItem)->GetSrcPath(), (*itItem)->GetDestPath() );

		return pOutRenamePairs->GetPairs().size() == renameItems.size();			// all SRC keys unique?
	}

	void MakePairsToItems( std::vector<CRenameItem*>& rOutRenameItems, const CPathRenamePairs& renamePairs )
	{
		REQUIRE( rOutRenameItems.empty() );

		for ( CPathRenamePairs::const_iterator itPair = renamePairs.Begin(); itPair != renamePairs.End(); ++itPair )
		{
			CRenameItem* pItem = new CRenameItem( itPair->first );

			pItem->RefDestPath() = itPair->second;
			rOutRenameItems.push_back( pItem );
		}
	}

	void AssignPairsToItems( const std::vector<CRenameItem*>& items, const CPathRenamePairs& renamePairs )
	{
		REQUIRE( items.size() == renamePairs.GetPairs().size() );

		size_t pos = 0;
		for ( CPathRenamePairs::const_iterator itPair = renamePairs.Begin(); itPair != renamePairs.End(); ++itPair, ++pos )
		{
			CRenameItem* pItem = items[ pos ];

			if ( pItem->GetSrcPath() == itPair->first )			// both containers must share the same order
				pItem->RefDestPath() = itPair->second;
			else
			{
				ASSERT( false );
				break;
			}
		}
	}

	void QueryDestFnames( std::vector<std::tstring>& rDestFnames, const std::vector<CRenameItem*>& items )
	{
		rDestFnames.clear();
		rDestFnames.reserve( items.size() );

		for ( std::vector<CRenameItem*>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			fs::CPathParts destParts;
			( *itItem )->SplitSafeDestPath( &destParts );
			rDestFnames.push_back( destParts.m_fname );
		}
	}


	bool AdjustDirectoryFilename( fs::CPathParts* pOutParts, const fs::CPath* pSrcFilePath )
	{
		ASSERT_PTR( pOutParts );
		ASSERT_PTR( pSrcFilePath );

		if ( fs::IsValidDirectory( pSrcFilePath->GetPtr() ) )
		{
			// for directories treat extension as being part of the fname (dirs have no file type)
			pOutParts->m_fname += pOutParts->m_ext;
			pOutParts->m_ext.clear();
			return true;
		}

		return false;
	}

	bool SplitPath( fs::CPathParts* pOutParts, const fs::CPath* pSrcFilePath, const fs::CPath& filePath )
	{
		ASSERT_PTR( pOutParts );
		pOutParts->SplitPath( filePath.Get() );
		return AdjustDirectoryFilename( pOutParts, pSrcFilePath );
	}
}


// CRenameItem implementation

CRenameItem::CRenameItem( const fs::CPath& srcPath )
	: CFileStateItem( fs::CFileState::ReadFromFile( srcPath ) )
{
}

CRenameItem::~CRenameItem()
{
}


// CDisplayFilenameAdapter implementation

std::tstring CDisplayFilenameAdapter::FormatCode( const utl::ISubject* pSubject ) const
{
	ASSERT_PTR( pSubject );
	return FormatFilename( pSubject->GetDisplayCode() );
}

std::tstring CDisplayFilenameAdapter::FormatFilename( const fs::CPath& filePath ) const
{
	std::tstring filename = filePath.GetFilename();

	if ( m_ignoreExtension )
		return StripExtension( filename.c_str() );

	return filename;
}

fs::CPath CDisplayFilenameAdapter::ParseFilename( const std::tstring& displayFilename, const fs::CPath& referencePath ) const
{
	if ( m_ignoreExtension )
		return referencePath.GetParentPath() / ( displayFilename + referencePath.GetExt() );		// use the reference extension

	return referencePath.GetParentPath() / displayFilename;			// use the input extension
}

std::tstring CDisplayFilenameAdapter::FormatPath( fmt::PathFormat format, const fs::CPath& filePath ) const
{
	std::tstring displayPath = fmt::FormatPath( filePath, format );

	if ( m_ignoreExtension )
		displayPath = StripExtension( displayPath.c_str() );

	return displayPath;
}

fs::CPath CDisplayFilenameAdapter::ParsePath( const std::tstring& inputPath, const fs::CPath& referencePath ) const
{
	fs::CPath filePath( inputPath );

	if ( m_ignoreExtension && !filePath.ExtEquals( referencePath.GetExt() ) )		// avoid doubling the same extension
		filePath.Set( filePath.Get() + referencePath.GetExt() );

	if ( !filePath.HasParentPath() )
		filePath.SetDirPath( referencePath.GetParentPath().Get() );		// qualify with reference dir path

	return filePath;
}

bool CDisplayFilenameAdapter::IsExtensionChange( const fs::CPath& referencePath, const fs::CPath& destPath )
{
	switch ( referencePath.GetExtensionMatch( destPath ) )
	{
		//case fs::MismatchDotsExt:
		case fs::MismatchExt:
			return true;
	}

	return false;		// case change is not a major extension change
}

std::tstring CDisplayFilenameAdapter::StripExtension( const TCHAR* pFilePath )
{
	const TCHAR* pExt = path::FindExt( pFilePath );

	return std::tstring( pFilePath, std::distance( pFilePath, pExt ) );		// strip the extension
}
