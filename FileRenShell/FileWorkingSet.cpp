
#include "stdafx.h"
#include "FileWorkingSet.h"
#include "FileSetUi.h"
#include "UndoChangeLog.h"
#include "utl/Clipboard.h"
#include "utl/FmtUtils.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_clipSep[] = _T("\t");

	std::tstring FormatTouchSrc( const TFileStatePair& fileStatePair, fmt::PathFormat pathFormat )
	{
		return std::tstring( FormatPath( fileStatePair.first.m_fullPath, pathFormat ) ) + s_clipSep + fmt::FormatFileState( fileStatePair.first );
	}

	void ParseTouchDest( TFileStatePair& rFileStatePair, const str::TStringRange& textRange ) throws_( CRuntimeException )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_clipSep ) )
		{
			fs::CFileState destFileState;
			destFileState.m_fullPath = textRange.ExtractLead( sepPos.m_start );

			if ( path::IsNameExt( destFileState.m_fullPath.GetPtr() ) )
				destFileState.m_fullPath = rFileStatePair.first.m_fullPath.GetParentPath() / destFileState.m_fullPath;		// convert to full path

			if ( destFileState.m_fullPath != rFileStatePair.first.m_fullPath )
				throw CRuntimeException( str::Format( _T("Pasted destination file name is inconsistent with source path.\n\nSource: %s\nDestination: %s"),
					rFileStatePair.first.m_fullPath.GetPtr(), destFileState.m_fullPath.GetPtr() ) );

			str::TStringRange stateRange = textRange.MakeTrail( sepPos.m_end );
			if ( ParseFileState( destFileState, stateRange ) )
			{
				rFileStatePair.second = destFileState;
				return;
			}
		}

		throw CRuntimeException( str::Format( _T("Pasted destination file status is not valid: %s"), textRange.GetText().c_str() ) );
	}
}


// CFileWorkingSet implementation

const TCHAR CFileWorkingSet::s_lineEnd[] = _T("\r\n");

CFileWorkingSet::CFileWorkingSet( void )
	: m_pUndoChangeLog( new CUndoChangeLog() )
	, m_mixedDirPaths( false )
{
}

CFileWorkingSet::~CFileWorkingSet()
{
}

void CFileWorkingSet::ResetDestinations( void )
{
	ClearErrors();

	for ( fs::TPathPairMap::iterator itPair = m_renamePairs.begin(); itPair != m_renamePairs.end(); ++itPair )
		itPair->second.Clear();

	for ( fs::TFileStatePairMap::iterator itPair = m_touchPairs.begin(); itPair != m_touchPairs.end(); ++itPair )
		itPair->second = itPair->first;
}

size_t CFileWorkingSet::SetupFromDropInfo( HDROP hDropInfo )
{
	m_sourceFiles.clear();
	m_renamePairs.clear();
	m_touchPairs.clear();
	m_mixedDirPaths = false;

	if ( hDropInfo != NULL )
	{
		for ( int i = 0, fileCount = ::DragQueryFile( hDropInfo, UINT_MAX, NULL, 0 ); i < fileCount; ++i )
		{
			TCHAR pathBuffer[ _MAX_PATH ];
			::DragQueryFile( hDropInfo, i, pathBuffer, COUNT_OF( pathBuffer ) );

			fs::CPath fullPath( pathBuffer );

			m_sourceFiles.push_back( fullPath );
			m_renamePairs[ fullPath ] = fs::CPath();

			fs::CFileState fileState = fs::CFileState::ReadFromFile( fullPath );
			m_touchPairs[ fileState ] = fileState;
		}

		std::sort( m_sourceFiles.begin(), m_sourceFiles.end() );
	}
	m_mixedDirPaths = utl::HasMultipleDirPaths( m_renamePairs ) || utl::HasMultipleDirPaths( m_touchPairs );

	return m_sourceFiles.size();
}

void CFileWorkingSet::LoadUndoLog( void )
{
	m_pUndoChangeLog->Load();
}

void CFileWorkingSet::SaveUndoLog( void )
{
	m_pUndoChangeLog->Save();
}

bool CFileWorkingSet::CanUndo( app::Action action ) const
{
	switch ( action )
	{
		default: ASSERT( false );
		case app::RenameFiles:	return m_pUndoChangeLog->GetRenameUndoStack().CanUndo();
		case app::TouchFiles:	return m_pUndoChangeLog->GetTouchUndoStack().CanUndo();
	}
}

void CFileWorkingSet::SaveUndoInfo( app::Action action, const fs::TPathSet& keyPaths )
{
	switch ( action )
	{
		case app::RenameFiles:	_SaveUndoInfo( m_pUndoChangeLog->GetRenameUndoStack().Push(), m_renamePairs, keyPaths ); break;
		case app::TouchFiles:	_SaveUndoInfo( m_pUndoChangeLog->GetTouchUndoStack().Push(), m_touchPairs, keyPaths ); break;
		default: ASSERT( false );
	}
}

template< typename UndoMapType, typename DataMapType >
void CFileWorkingSet::_SaveUndoInfo( UndoMapType& rUndoPairs, DataMapType& rDataMemberPairs, const fs::TPathSet& keyPaths )
{
	// push in the undo stack only the pairs that were successfully committed
	for ( typename DataMapType::const_iterator itDataPair = rDataMemberPairs.begin(); itDataPair != rDataMemberPairs.end(); ++itDataPair )
		if ( keyPaths.find( func::PathOf( itDataPair->first ) ) != keyPaths.end() )		// successfully renamed
			rUndoPairs[ itDataPair->first ] = itDataPair->second;

	rDataMemberPairs.clear();		// this action was committed, we're done
	m_mixedDirPaths = false;
}

void CFileWorkingSet::RetrieveUndoInfo( app::Action action )
{
	ASSERT( CanUndo( action ) );

	switch ( action )
	{
		case app::RenameFiles:	_RetrieveUndoInfo( m_renamePairs, m_pUndoChangeLog->GetRenameUndoStack().GetTop() ); break;
		case app::TouchFiles:	_RetrieveUndoInfo( m_touchPairs, m_pUndoChangeLog->GetTouchUndoStack().GetTop() ); break;
		default: ASSERT( false );
	}
}

template< typename DataMapType, typename UndoMapType >
void CFileWorkingSet::_RetrieveUndoInfo( DataMapType& rDataMemberPairs, const UndoMapType* pTopUndoPairs )
{
	ASSERT_PTR( pTopUndoPairs );

	rDataMemberPairs.clear();
	for ( typename UndoMapType::const_iterator itUndo = pTopUndoPairs->begin(); itUndo != pTopUndoPairs->end(); ++itUndo )
		rDataMemberPairs[ itUndo->second ] = itUndo->first;		// for undo swap source and destination

	m_mixedDirPaths = utl::HasMultipleDirPaths( rDataMemberPairs );
}

void CFileWorkingSet::CommitUndoInfo( app::Action action )
{
	ASSERT( CanUndo( action ) );

	switch ( action )
	{
		case app::RenameFiles:	m_pUndoChangeLog->GetRenameUndoStack().Pop(); break;
		case app::TouchFiles:	m_pUndoChangeLog->GetTouchUndoStack().Pop(); break;
		default: ASSERT( false );
	}
}

bool CFileWorkingSet::CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd ) const
{
	std::vector< std::tstring > sourcePaths;
	for ( std::vector< fs::CPath >::const_iterator itFile = m_sourceFiles.begin(); itFile != m_sourceFiles.end(); ++itFile )
		sourcePaths.push_back( fmt::FilenameExt == format ? itFile->GetNameExt() : itFile->Get() );

	std::tstring multiPath = str::Join( sourcePaths, _T("\r\n") );
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
											  destPaths.size(), m_renamePairs.size() ) );

	int pos = 0;

	for ( fs::TPathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
	{
		fs::CPath newFilePath( destPaths[ pos ] );

		it->first.QualifyWithSameDirPathIfEmpty( newFilePath );
		it->second = newFilePath;
	}
}

bool CFileWorkingSet::CheckPathCollisions( std::vector< std::tstring >& rDestDuplicates )
{
	fs::TPathSet destPaths;
	size_t pos = 0, emptyCount = 0;

	for ( fs::TPathPairMap::const_iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
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
		throw CRuntimeException( str::Format( _T("Detected duplicate file name collisions:\r\n%s"), str::Join( dupDestPaths, _T("\r\n") ).c_str() ) );
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
	ResetDestinations();

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

	for ( fs::TPathPairMap::const_iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it )
		fnames.push_back( CFileSetUi::GetDestFname( it ) );

	num::EnsureUniformZeroPadding( fnames );

	size_t pos = 0;
	for ( fs::TPathPairMap::iterator it = m_renamePairs.begin(); it != m_renamePairs.end(); ++it, ++pos )
	{
		fs::CPathParts destParts( CFileSetUi::GetDestPath( it ) );
		destParts.m_fname = fnames[ pos ];
		it->second = destParts.MakePath();
	}
}


// TOUCH

bool CFileWorkingSet::CopyClipSourceFileStates( CWnd* pWnd ) const
{
	std::vector< std::tstring > sourcePaths; sourcePaths.reserve( m_touchPairs.size() );

	for ( fs::TFileStatePairMap::const_iterator itTouchPair = m_touchPairs.begin(); itTouchPair != m_touchPairs.end(); ++itTouchPair )
		sourcePaths.push_back( fmt::FormatTouchSrc( *itTouchPair, m_mixedDirPaths ? fmt::FullPath : fmt::FilenameExt ) );

	std::tstring text = str::Join( sourcePaths, _T("\r\n") );
	return CClipboard::CopyText( text, pWnd );
}

void CFileWorkingSet::PasteClipDestinationFileStates( CWnd* pWnd ) throws_( CRuntimeException )
{
	ClearErrors();

	std::vector< std::tstring > lines;
	std::tstring text;
	if ( CClipboard::CanPasteText() && CClipboard::PasteText( text, pWnd ) )
		str::Split( lines, text.c_str(), _T("\r\n") );

	for ( std::vector< std::tstring >::iterator itLine = lines.begin(); itLine != lines.end(); )
	{
		str::Trim( *itLine );
		if ( itLine->empty() )
			itLine = lines.erase( itLine );
		else
			++itLine;
	}

	if ( lines.empty() )
		throw CRuntimeException( _T("No destination file states available to paste.") );
	else if ( m_touchPairs.size() != lines.size() )
		throw CRuntimeException( str::Format( _T("Destination file state count of %d doesn't match source file count of %d."),
											  lines.size(), m_touchPairs.size() ) );

	int pos = 0;
	for ( fs::TFileStatePairMap::iterator itTouchPair = m_touchPairs.begin(); itTouchPair != m_touchPairs.end(); ++itTouchPair, ++pos )
		fmt::ParseTouchDest( *itTouchPair, str::TStringRange( lines[ pos ] ) );
}
