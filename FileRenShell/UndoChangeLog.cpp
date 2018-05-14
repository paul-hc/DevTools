
#include "stdafx.h"
#include "UndoChangeLog.h"
#include "utl/EnumTags.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/TimeUtl.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CUndoChangeLog implementation

static const TCHAR s_undoFnSuffix[] = _T("_undo");
static const TCHAR s_actionTagFormat[] = _T("<%s %s>");		// <ACTION TIMESTAMP>
static const std::tstring s_filePairSep = _T(" -> ");
static const std::string s_endOfBatchSep = "<END OF BATCH>";

const std::tstring& CUndoChangeLog::GetFilePath( void )
{
	static std::tstring undoLogPath;
	if ( undoLogPath.empty() )
	{
		TCHAR fullPath[ _MAX_PATH ];
		::GetModuleFileName( AfxGetApp()->m_hInstance, fullPath, COUNT_OF( fullPath ) );

		fs::CPathParts parts( fullPath );
		parts.m_fname += s_undoFnSuffix;
		parts.m_ext = _T(".log");
		undoLogPath = parts.MakePath();
	}

	return undoLogPath;
}

bool CUndoChangeLog::Save( void ) const
{
	const std::tstring& undoLogPath = GetFilePath();
	std::ofstream output( str::ToUtf8( undoLogPath.c_str() ).c_str(), std::ios_base::out | std::ios_base::trunc );

	if ( output.is_open() )
	{
		SaveRenameBatches( output );
		SaveTouchBatches( output );

		output.close();
	}
	if ( output.fail() )
	{
		TRACE( _T(" * CUndoChangeLog::Save(): error saving undo change log file: %s\n"), undoLogPath.c_str() );
		ui::BeepSignal( MB_ICONERROR );
		return false;
	}
	return true;
}

bool CUndoChangeLog::Load( void )
{
	const std::tstring& undoLogPath = GetFilePath();
	std::ifstream input( str::ToUtf8( undoLogPath.c_str() ).c_str() );

	if ( !input.is_open() )
		return false;

	Action action = Rename;
	CTime timestamp;
	fs::PathPairMap batchRename;
	fs::TFileInfoSet batchTouch;

	// note: most recent rename batch is the last in the undo log
	for ( unsigned int parseLineNo = 1; !input.eof(); ++parseLineNo )
	{
		std::string line;
		std::getline( input, line );
		if ( line.empty() )		// ignore empty lines
			continue;

		bool validActionTag = ParseActionTag( action, timestamp, line );
		if ( validActionTag )
			continue;			// consume the tag line

		if ( str::EqualString< str::IgnoreCase >( line, s_endOfBatchSep ) )
		{
			switch ( action )
			{
				case Rename:
					if ( !batchRename.empty() )
					{
						m_renameUndoStack.push_back( CBatch< fs::PathPairMap >( timestamp ) );
						m_renameUndoStack.back().m_batch.swap( batchRename );
					}
					break;
				case Touch:
					if ( !batchTouch.empty() )
					{
						m_touchUndoStack.push_back( CBatch< fs::TFileInfoSet >( timestamp ) );
						m_touchUndoStack.back().m_batch.swap( batchTouch );
					}
					break;
				default:
					ASSERT( false );
			}
		}
		else
		{
			switch ( action )
			{
				case Rename:
					if ( ParseRenameLine( batchRename, str::FromUtf8( line.c_str() ) ) )
						continue;
					break;
				case Touch:
					if ( ParseTouchLine( batchTouch, str::FromUtf8( line.c_str() ) ) )
						continue;
					break;
				default:
					ASSERT( false );
			}

			TRACE( _T(" * CUndoChangeLog::Load(): %s - ignore badly formed %s line: %s\n"), undoLogPath.c_str(), GetTags_Action().GetUiTags()[ action ].c_str(), line.c_str() );
		}
	}
	input.close();
	return true;
}

bool CUndoChangeLog::ParseRenameLine( fs::PathPairMap& rBatchRename, const std::tstring& line )
{
	static const size_t sepLength = s_filePairSep.length();
	size_t sepPos = line.find( s_filePairSep );
	if ( sepPos != std::string::npos )
	{
		fs::CPath sourcePath( line.substr( 0, sepPos ) );
		fs::CPath destPath( line.substr( sepPos + sepLength ) );

		if ( !sourcePath.IsEmpty() && !destPath.IsEmpty() )
		{
			rBatchRename[ sourcePath ] = destPath;
			return true;
		}
	}

	return false;
}

bool CUndoChangeLog::ParseTouchLine( fs::TFileInfoSet& rBatchTouch, const std::tstring& line )
{
	fs::CFileInfo fileInfo;
	if ( !fileInfo.Parse( line ) )
		return false;

	rBatchTouch.insert( fileInfo );
	return true;
}

void CUndoChangeLog::SaveRenameBatches( std::ostream& output ) const
{
	for ( std::list< CBatch< fs::PathPairMap > >::const_iterator itBatch = m_renameUndoStack.begin(), itFirst = itBatch; itBatch != m_renameUndoStack.end(); ++itBatch )
	{
		if ( itBatch != itFirst )
			output << std::endl;		// inner batch extra line-end separator

		output << FormatActionTag( Rename, itBatch->m_timestamp ) << std::endl;		// add action tag

		for ( fs::PathPairMap::const_iterator itPair = itBatch->m_batch.begin(); itPair != itBatch->m_batch.end(); ++itPair )
		{
			ASSERT( !itPair->first.IsEmpty() && !itPair->second.IsEmpty() );
			output << itPair->first.Get() << s_filePairSep << itPair->second.Get() << std::endl;
		}
		output << s_endOfBatchSep << std::endl;
	}
}

void CUndoChangeLog::SaveTouchBatches( std::ostream& output ) const
{
	for ( std::list< CBatch< fs::TFileInfoSet > >::const_iterator itBatch = m_touchUndoStack.begin(), itFirst = itBatch; itBatch != m_touchUndoStack.end(); ++itBatch )
	{
		if ( itBatch != itFirst )
			output << std::endl;		// inner batch extra line-end separator

		output << FormatActionTag( Touch, itBatch->m_timestamp ) << std::endl;		// add action tag

		for ( fs::TFileInfoSet::const_iterator itPair = itBatch->m_batch.begin(); itPair != itBatch->m_batch.end(); ++itPair )
			output << itPair->Format() << std::endl;

		output << s_endOfBatchSep << std::endl;
	}
}

const CEnumTags& CUndoChangeLog::GetTags_Action( void )
{
	static const CEnumTags tags( _T("RENAME|TOUCH") );
	return tags;
}

bool CUndoChangeLog::ParseActionTag( Action& rAction, CTime& rTimestamp, const std::string& text )
{
	std::vector< std::tstring > parts;
	str::Tokenize( parts, str::FromAnsi( text.c_str() ).c_str(), _T("<> ") );

	if ( parts.size() <= 2 )
	{
		rAction = static_cast< Action >( GetTags_Action().ParseUi( parts[ 0 ] ) );
		if ( rAction != GetTags_Action().GetDefaultValue< Action >() )
		{
			if ( 2 == parts.size() )
				rTimestamp = time_utl::ParseTimestamp( parts[ 1 ] );
			return true;
		}
	}

	rAction = Rename;			// backwards compatible with no action tags
	rTimestamp = CTime();
	return false;				// not an action tag - a content tag
}

std::tstring CUndoChangeLog::FormatActionTag( Action action, const CTime& timestamp )
{
	return str::Format( s_actionTagFormat,
		GetTags_Action().GetUiTags()[ action ].c_str(),
		timestamp.GetTime() != 0 ? time_utl::FormatTimestamp( timestamp ).c_str() : _T("-") );
}
