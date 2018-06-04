
#include "stdafx.h"
#include "FileService.h"
#include "FileCommands.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "utl/PathFormatter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileService::CFileService( void )
{
}

CMacroCommand* CFileService::MakeRenameCmds( const std::vector< CRenameItem* >& renameItems ) const
{
	if ( !IsDistinctWorkingSet( renameItems ) )
	{
		REQUIRE( false );			// all SRC and DEST paths must be a distinct working set (pre-validated)
		return NULL;
	}

	std::auto_ptr< CMacroCommand > pBatchMacro( new cmd::CFileMacroCmd( cmd::RenameFile ) );
	std::vector< utl::ICommand* > laterCmds;
	std::set< fs::CPath > destToBeSet;

	for ( std::vector< CRenameItem* >::const_iterator itItem = renameItems.begin(); itItem != renameItems.end(); ++itItem )
		if ( ( *itItem )->IsModified() )
		{
			const fs::CPath& srcPath = ( *itItem )->GetSrcPath();
			fs::CPath destPath = ( *itItem )->GetDestPath();

			if ( destPath.FileExist() || destToBeSet.find( destPath ) != destToBeSet.end() )		// collision with an existing or future file (not from this rename pair)?
			{
				// avoid rename collisions: rename in 2 steps using an unique intermediate file path
				fs::CPath intermPath = MakeUniqueIntermPath( destPath, destToBeSet );

				pBatchMacro->AddCmd( new CRenameFileCmd( srcPath, intermPath ) );					// SRC -> INTERM
				laterCmds.push_back( new CRenameFileCmd( intermPath, destPath ) );					// INTERM -> DEST (later)

				destToBeSet.insert( intermPath );
			}
			else
				pBatchMacro->AddCmd( new CRenameFileCmd( srcPath, destPath ) );

			destToBeSet.insert( destPath );
		}

	for ( std::vector< utl::ICommand* >::const_iterator itLateCmd = laterCmds.begin(); itLateCmd != laterCmds.end(); ++itLateCmd )
		pBatchMacro->AddCmd( *itLateCmd );

	return !pBatchMacro->IsEmpty() ? pBatchMacro.release() : NULL;
}

CMacroCommand* CFileService::MakeTouchCmds( const std::vector< CTouchItem* >& touchItems ) const
{
	std::auto_ptr< CMacroCommand > pBatchMacro( new cmd::CFileMacroCmd( cmd::TouchFile ) );

	for ( std::vector< CTouchItem* >::const_iterator itItem = touchItems.begin(); itItem != touchItems.end(); ++itItem )
		if ( ( *itItem )->IsModified() )
			pBatchMacro->AddCmd( new CTouchFileCmd( ( *itItem )->GetSrcState(), ( *itItem )->GetDestState() ) );

	return !pBatchMacro->IsEmpty() ? pBatchMacro.release() : NULL;
}


bool CFileService::IsDistinctWorkingSet( const std::vector< CRenameItem* >& renameItems )
{
	std::set< fs::CPath > srcPaths, destPaths;

	for ( std::vector< CRenameItem* >::const_iterator itItem = renameItems.begin(); itItem != renameItems.end(); ++itItem )
		if ( !srcPaths.insert( ( *itItem )->GetSrcPath() ).second || !destPaths.insert( ( *itItem )->GetDestPath() ).second )
			return false;		// SRC or DEST not unique in the working set

	ENSURE( srcPaths.size() == destPaths.size() );
	ENSURE( srcPaths.size() == renameItems.size() );
	return true;
}

fs::CPath CFileService::MakeUniqueIntermPath( const fs::CPath& destPath, const std::set< fs::CPath >& destToBeSet )
{
	static const CPathFormatter s_formatter( _T("*-[#].*") );

	fs::CPath uniqueDestPath;
	for ( UINT seqCount = 2; ; ++seqCount )
	{
		uniqueDestPath = s_formatter.FormatPath( destPath.Get(), seqCount );
		if ( !uniqueDestPath.FileExist() && destToBeSet.find( destPath ) == destToBeSet.end() )		// unique existing or to-be file?
			break;
	}

	return uniqueDestPath;
}
