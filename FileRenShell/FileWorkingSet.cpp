
#include "stdafx.h"
#include "FileWorkingSet.h"
#include "FileSetUi.h"
#include "UndoChangeLog.h"
#include "utl/Clipboard.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"


CFileWorkingSet::CFileWorkingSet( void )
{
}

CFileWorkingSet::~CFileWorkingSet()
{
}

void CFileWorkingSet::ClearDestinationFiles( void )
{
	ClearErrors();

	for ( fs::PathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it )
		it->second = fs::CPath();
}

void CFileWorkingSet::LoadUndoLog( void )
{
	CUndoChangeLog::Load( m_undoStack );
}

void CFileWorkingSet::SaveUndoLog( void )
{
	CUndoChangeLog::Save( m_undoStack );
}

void CFileWorkingSet::SaveUndoInfo( const fs::PathSet& renamedKeys )
{
	m_undoStack.push_back( fs::PathPairMap() );

	// push in the undo stack only pairs that were successfully renamed
	fs::PathPairMap& rRenamePairs = m_undoStack.back();

	for ( fs::PathPairMap::const_iterator itRenamePair = m_renamePairs.begin();
		  itRenamePair != m_renamePairs.end(); ++itRenamePair )
		if ( renamedKeys.find( itRenamePair->first ) != renamedKeys.end() )		// successfully renamed
			rRenamePairs[ itRenamePair->first ] = itRenamePair->second;

	m_renamePairs.clear();

	if ( m_undoStack.size() > MaxUndoSize )
		m_undoStack.pop_front();
}

void CFileWorkingSet::RetrieveUndoInfo( void )
{
	ASSERT( CanUndo() );

	const fs::PathPairMap& topRenamePairs = m_undoStack.back();	// stack top is last

	m_renamePairs.clear();
	for ( fs::PathPairMap::const_iterator it = topRenamePairs.begin(); it != topRenamePairs.end(); ++it )
		m_renamePairs[ it->second ] = it->first;	// for undo swap source and destination
}

void CFileWorkingSet::CommitUndoInfo( void )
{
	ASSERT( CanUndo() );
	m_undoStack.pop_back();
}

size_t CFileWorkingSet::SetupFromDropInfo( HDROP hDropInfo )
{
	m_sourceFiles.clear();
	m_renamePairs.clear();

	if ( hDropInfo != NULL )
	{
		fs::CPath selFile;

		for ( int i = 0, fileCount = ::DragQueryFile( hDropInfo, UINT_MAX, NULL, 0 ); i < fileCount; ++i )
		{
			TCHAR filePath[ _MAX_PATH ];
			::DragQueryFile( hDropInfo, i, filePath, COUNT_OF( filePath ) );
			selFile.Set( filePath );

			m_sourceFiles.push_back( selFile );
			m_renamePairs[ selFile ] = fs::CPath();
		}

		std::sort( m_sourceFiles.begin(), m_sourceFiles.end() );
	}

	return GetFileCount();
}

bool CFileWorkingSet::CopyClipSourcePaths( PathType pathType, CWnd* pWnd ) const
{
	std::vector< std::tstring > sourcePaths;
	for ( std::vector< fs::CPath >::const_iterator itFile = m_sourceFiles.begin(); itFile != m_sourceFiles.end(); ++itFile )
		sourcePaths.push_back( FilenameExt == pathType ? itFile->GetNameExt() : itFile->Get() );

	std::tstring multiPath = str::Unsplit( sourcePaths, _T("\r\n") );
	return CClipboard::CopyText( multiPath, pWnd );
}

void CFileWorkingSet::PasteClipDestinationPaths( CWnd* pWnd ) throws_( CRuntimeException )
{
	ClearErrors();

	std::tstring text;
	if ( !CClipboard::CanPasteText() || !CClipboard::PasteText( text, pWnd ) )
		throw CRuntimeException( _T("No destination file paths available to paste.") );

	std::vector< std::tstring > destPaths;
	str::Split( destPaths, text.c_str(), _T("\r\n") );

	for ( std::vector< std::tstring >::iterator itPath = destPaths.begin(); itPath != destPaths.end(); )
	{
		str::Trim( *itPath );
		if ( itPath->empty() )
			itPath = destPaths.erase( itPath );
		else
			++itPath;
	}

	if ( destPaths.empty() )
		throw CRuntimeException( _T("No destination file paths available to paste.") );
	else if ( m_renamePairs.size() != destPaths.size() )
		throw CRuntimeException( str::Format( _T("Destination file count of %d doesn't match source file count of %d."),
											  destPaths.size(), m_renamePairs.size() ).c_str() );

	int pos = 0;

	for ( fs::PathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
	{
		fs::CPath newFilePath( destPaths[ pos ] );

		it->first.QualifyWithSameDirPathIfEmpty( newFilePath );
		it->second = newFilePath;
	}
}

bool CFileWorkingSet::CheckPathCollisions( std::vector< std::tstring >& rDestDuplicates )
{
	fs::PathSet destPaths;
	size_t pos = 0, emptyCount = 0;

	for ( fs::PathPairMap::const_iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
		if ( it->second.IsEmpty() )									// ignore empty dest paths
			++emptyCount;
		else if ( !destPaths.insert( it->second ).second ||			// not unique in the working set
				  FileExistOutsideWorkingSet( it->second ) )		// collides with an existing file/dir outside of the working set
		{
			rDestDuplicates.push_back( it->second.Get() );
			m_errorIndexes.insert( pos );
		}

	return rDestDuplicates.empty();
}

void CFileWorkingSet::CheckPathCollisions( void ) throws_( CRuntimeException )
{
	std::vector< std::tstring > dupDestPaths;
	if ( !CheckPathCollisions( dupDestPaths ) )
		throw CRuntimeException( str::Format( _T("Detected duplicate file name collisions:\r\n%s"), str::Unsplit( dupDestPaths, _T("\r\n") ).c_str() ) );
}

bool CFileWorkingSet::FileExistOutsideWorkingSet( const fs::CPath& filePath ) const
{
	return
		m_renamePairs.find( filePath ) == m_renamePairs.end() &&
		filePath.FileExist();
}


// returns the new incremented counter

bool CFileWorkingSet::GenerateDestPaths( const std::tstring& format, UINT* pSeqCount )
{
	ASSERT_PTR( pSeqCount );
	ClearDestinationFiles();

	CPathGenerator generator( m_renamePairs, format, *pSeqCount );
	if ( !generator.GeneratePairs() )
		return false;

	*pSeqCount = generator.GetSeqCount();
	return true;
}

UINT CFileWorkingSet::FindNextAvailSeqCount( const std::tstring& format ) const
{
	CPathGenerator generator( m_renamePairs, format );
	return generator.FindNextAvailSeqCount();
}

void CFileWorkingSet::EnsureUniformNumPadding( void )
{
	std::vector< std::tstring > fnames; fnames.reserve( fnames.size() );

	for ( fs::PathPairMap::const_iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it )
		fnames.push_back( CFileSetUi::GetDestFname( it ) );

	num::EnsureUniformZeroPadding( fnames );

	size_t pos = 0;
	for ( fs::PathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
	{
		fs::CPathParts destParts( CFileSetUi::GetDestPath( it ) );
		destParts.m_fname = fnames[ pos ];
		it->second = destParts.MakePath();
	}
}
