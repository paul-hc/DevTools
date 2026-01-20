
#include "pch.h"
#include "FileService.h"
#include "FileMacroCommands.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "EditLinkItem.h"
#include "utl/PathFormatter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileService::CFileService( void )
{
}

std::auto_ptr<CMacroCommand> CFileService::MakeRenameCmds( const std::vector<CRenameItem*>& renameItems ) const
{
	std::auto_ptr<CMacroCommand> pBatchMacro;
	if ( !IsDistinctWorkingSet( renameItems ) )			// all SRC and DEST paths must be a distinct working set (pre-validated)?
		return pBatchMacro;

	pBatchMacro.reset( new cmd::CFileMacroCmd( cmd::RenameFile ) );

	std::vector<utl::ICommand*> finalCmds;
	std::set<fs::CPath> destToBeSet;

	for ( const CRenameItem* pItem: renameItems )
		if ( pItem->IsModified() )
		{
			const fs::CPath& srcPath = pItem->GetSrcPath();
			fs::CPath destPath = pItem->GetDestPath();

			if ( ( destPath.FileExist() && destPath != srcPath ) ||			// collision with an existing file (not of this pair)?
				 destToBeSet.find( destPath ) != destToBeSet.end() )		// collision with a future file?
			{
				// avoid rename collisions: rename in 2 steps using an unique intermediate file path
				fs::CPath intermPath = MakeUniqueIntermPath( destPath, destToBeSet );

				pBatchMacro->AddCmd( new CRenameFileCmd( srcPath, intermPath ) );					// SRC -> INTERM
				finalCmds.push_back( new CRenameFileCmd( intermPath, destPath ) );					// INTERM -> DEST (later)

				destToBeSet.insert( intermPath );
			}
			else
				pBatchMacro->AddCmd( new CRenameFileCmd( srcPath, destPath ) );

			destToBeSet.insert( destPath );
		}

	// add final commands: INTERM -> DEST
	for ( std::vector<utl::ICommand*>::const_iterator itLateCmd = finalCmds.begin(); itLateCmd != finalCmds.end(); ++itLateCmd )
		pBatchMacro->AddCmd( *itLateCmd );

	return pBatchMacro;
}

std::auto_ptr<CMacroCommand> CFileService::MakeTouchCmds( const std::vector<CTouchItem*>& touchItems ) const
{
	std::auto_ptr<CMacroCommand> pBatchMacro( new cmd::CFileMacroCmd( cmd::TouchFile ) );

	for ( const CTouchItem* pItem: touchItems )
		if ( pItem->IsModified() )
			pBatchMacro->AddCmd( new CTouchFileCmd( pItem->GetSrcState(), pItem->GetDestState() ) );

	return pBatchMacro;
}

std::auto_ptr<CMacroCommand> CFileService::MakeEditLinkCmds( const std::vector<CEditLinkItem*>& editLinkItems ) const
{
	std::auto_ptr<CMacroCommand> pBatchMacro( new cmd::CFileMacroCmd( cmd::EditShortcut ) );

	for ( const CEditLinkItem* pItem: editLinkItems )
		if ( pItem->IsModified() )
			pBatchMacro->AddCmd( new CEditLinkFileCmd( pItem->GetFilePath(), pItem->GetSrcShortcut(), pItem->GetDestShortcut() ) );

	return pBatchMacro;
}


bool CFileService::IsDistinctWorkingSet( const std::vector<CRenameItem*>& renameItems )
{
	std::set<fs::CPath> srcPaths, destPaths;

	for ( const CRenameItem* pItem: renameItems )
		if ( !srcPaths.insert( pItem->GetSrcPath() ).second || !destPaths.insert( pItem->GetDestPath() ).second )
			return false;		// SRC or DEST not unique in the working set

	ENSURE( srcPaths.size() == destPaths.size() );
	ENSURE( srcPaths.size() == renameItems.size() );
	return true;
}

fs::CPath CFileService::MakeUniqueIntermPath( const fs::CPath& destPath, const std::set<fs::CPath>& destToBeSet )
{
	static const CPathFormatter s_formatter( _T("*-[#]"), true );

	fs::CPath uniqueDestPath;
	for ( UINT seqCount = 2; ; ++seqCount )
	{
		uniqueDestPath = s_formatter.FormatPath( destPath.Get(), seqCount );
		if ( !uniqueDestPath.FileExist() && destToBeSet.find( destPath ) == destToBeSet.end() )		// unique existing or to-be file?
			break;
	}

	return uniqueDestPath;
}
