#ifndef FileWorkingSet_h
#define FileWorkingSet_h
#pragma once

#include <vector>
#include <map>
#include <set>
#include <list>
#include "utl/FileState.h"
#include "utl/Path.h"
#include "Application_fwd.h"


enum PathType { FullPath, FilenameExt };
class CUndoChangeLog;


class CFileWorkingSet
{
public:
	CFileWorkingSet( void );
	~CFileWorkingSet();

	size_t GetFileCount( void ) const { return m_sourceFiles.size(); }
	bool IsEmpty( void ) const { return m_sourceFiles.empty(); }

	const std::vector< fs::CPath >& GetSourceFiles( void ) const { return m_sourceFiles; }
	const fs::TPathPairMap& GetRenamePairs( void ) const { return m_renamePairs; }
	fs::TFileStatePairMap& GetTouchPairs( void ) { return m_touchPairs; }

	size_t SetupFromDropInfo( HDROP hDropInfo );

	void ClearDestinations( void );

	void LoadUndoLog( void );
	void SaveUndoLog( void );

	bool CanUndo( app::Action action ) const;
	void SaveUndoInfo( app::Action action, const fs::TPathSet& keyPaths );
	void RetrieveUndoInfo( app::Action action );		// fills action pairs from undo stack
	void CommitUndoInfo( app::Action action );			// pops last from undo stack (when Undo-Rename OK is pressed)

	bool HasErrors( void ) const { return !m_errorIndexes.empty(); }
	bool IsErrorAt( unsigned int index ) const { return m_errorIndexes.find( index ) != m_errorIndexes.end(); }
	const std::set< size_t >& GetErrorIndexes( void ) const { return m_errorIndexes; }
	void ClearErrors( void ) { m_errorIndexes.clear(); }

	// RENAME
	bool CopyClipSourcePaths( PathType pathType, CWnd* pWnd ) const;
	void PasteClipDestinationPaths( CWnd* pWnd ) throws_( CRuntimeException );

	template< typename Func >
	void ForEachRenameDestination( const Func& func );

	bool GenerateDestPaths( const std::tstring& format, UINT* pSeqCount );
	UINT FindNextAvailSeqCount( const std::tstring& format ) const;
	void EnsureUniformNumPadding( void );

	bool CheckPathCollisions( std::vector< std::tstring >& rDestDuplicates );
	void CheckPathCollisions( void ) throws_( CRuntimeException );
	bool FileExistOutsideWorkingSet( const fs::CPath& filePath ) const;		// collision with an existing file/dir outside working set (selected files)
private:
	template< typename UndoMapType, typename DataMapType >
	static void _SaveUndoInfo( UndoMapType& rUndoPairs, DataMapType& rDataMemberPairs, const fs::TPathSet& keyPaths );

	template< typename DataMapType, typename UndoMapType >
	static void _RetrieveUndoInfo( DataMapType& rDataMemberPairs, const UndoMapType* pTopUndoPairs );
private:
	std::vector< fs::CPath > m_sourceFiles;
	fs::TPathPairMap m_renamePairs;
	fs::TFileStatePairMap m_touchPairs;
	std::auto_ptr< CUndoChangeLog > m_pUndoChangeLog;

	std::set< size_t > m_errorIndexes;
};


template< typename Func >
void CFileWorkingSet::ForEachRenameDestination( const Func& func )
{
	for ( fs::TPathPairMap::iterator itRename = m_renamePairs.begin(); itRename != m_renamePairs.end(); ++itRename )
	{
		fs::CPathParts destParts( !itRename->second.IsEmpty() ? itRename->second.Get() : itRename->first.Get() );		// use source if dest emty
		func( destParts );
		itRename->second.Set( destParts.MakePath() );
	}
}


#endif // FileWorkingSet_h
