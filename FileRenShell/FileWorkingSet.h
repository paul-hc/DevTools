#ifndef FileWorkingSet_h
#define FileWorkingSet_h
#pragma once

#include <vector>
#include <map>
#include <set>
#include <list>
#include "utl/Path.h"


enum PathType { FullPath, FilenameExt };


class CFileWorkingSet
{
public:
	CFileWorkingSet( void );
	~CFileWorkingSet();

	size_t GetFileCount( void ) const { return m_sourceFiles.size(); }
	bool IsEmpty( void ) const { return m_sourceFiles.empty(); }

	const std::vector< fs::CPath >& GetSourceFiles( void ) const { return m_sourceFiles; }
	const fs::PathPairMap& GetRenamePairs( void ) const { return m_renamePairs; }

	size_t SetupFromDropInfo( HDROP hDropInfo );

	void ClearDestinationFiles( void );

	bool CopyClipSourcePaths( PathType pathType, CWnd* pWnd ) const;
	void PasteClipDestinationPaths( CWnd* pWnd ) throws_( CRuntimeException );

	template< typename Func >
	void ForEachDestination( const Func& func );

	bool GenerateDestPaths( const std::tstring& format, UINT* pSeqCount );
	UINT FindNextAvailSeqCount( const std::tstring& format ) const;
	void EnsureUniformNumPadding( void );

	bool CanUndo( void ) const { return !m_undoStack.empty(); }
	void SaveUndoInfo( const fs::PathSet& renamedKeys );
	void RetrieveUndoInfo( void );		// fills rename pairs from undo stack
	void CommitUndoInfo( void );		// pops last from undo stack (when Undo-Rename OK is pressed)

	void LoadUndoLog( void );
	void SaveUndoLog( void );

	bool CheckPathCollisions( std::vector< std::tstring >& rDestDuplicates );
	void CheckPathCollisions( void ) throws_( CRuntimeException );
	bool FileExistOutsideWorkingSet( const fs::CPath& filePath ) const;		// collision with an existing file/dir outside working set (selected files)

	bool HasErrors( void ) const { return !m_errorIndexes.empty(); }
	bool IsErrorAt( unsigned int index ) const { return m_errorIndexes.find( index ) != m_errorIndexes.end(); }
	const std::set< size_t >& GetErrorIndexes( void ) const { return m_errorIndexes; }
	void ClearErrors( void ) { m_errorIndexes.clear(); }
private:
	enum { MaxUndoSize = 20 };
private:
	std::vector< fs::CPath > m_sourceFiles;
	fs::PathPairMap m_renamePairs;
	std::list< fs::PathPairMap > m_undoStack;

	std::set< size_t > m_errorIndexes;
};


template< typename Func >
void CFileWorkingSet::ForEachDestination( const Func& func )
{
	for ( fs::PathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it )
	{
		fs::CPathParts destParts( !it->second.IsEmpty() ? it->second.Get() : it->first.Get() );		// use source if dest emty
		func( destParts );
		it->second.Set( destParts.MakePath() );
	}
}


#endif // FileWorkingSet_h
