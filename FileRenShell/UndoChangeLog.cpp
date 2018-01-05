
#include "stdafx.h"
#include "UndoChangeLog.h"
#include "utl/Utilities.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static const std::string filePairSep = " -> ";
static const std::string endOfBatchSep = "<END OF BATCH>";


CUndoChangeLog::CUndoChangeLog( void )
{
}

CUndoChangeLog::~CUndoChangeLog()
{
}

const std::tstring& CUndoChangeLog::GetFilePath( void )
{
	static std::tstring undoLogPath;
	if ( undoLogPath.empty() )
	{
		TCHAR fullPath[ _MAX_PATH ];
		::GetModuleFileName( AfxGetApp()->m_hInstance, fullPath, COUNT_OF( fullPath ) );

		fs::CPathParts parts( fullPath );
		parts.m_fname += _T("_undo");
		parts.m_ext = _T(".log");
		undoLogPath = parts.MakePath();
	}

	return undoLogPath;
}

bool CUndoChangeLog::Save( const std::list< fs::PathPairMap >& undoStack )
{
	const std::tstring& undoLogPath = GetFilePath();
	std::ofstream output( str::ToUtf8( undoLogPath.c_str() ).c_str(), std::ios_base::out | std::ios_base::trunc );

	if ( output.is_open() )
	{
		for ( std::list< fs::PathPairMap >::const_iterator itBatch = undoStack.begin(), itFirst = itBatch;
			  itBatch != undoStack.end(); ++itBatch )
		{
			if ( itBatch != itFirst )
				output << std::endl;		// inner batch extra line-end separator

			for ( fs::PathPairMap::const_iterator itPair = itBatch->begin(); itPair != itBatch->end(); ++itPair )
			{
				ASSERT( !itPair->first.IsEmpty() && !itPair->second.IsEmpty() );
				output << itPair->first.Get() << filePairSep << itPair->second.Get() << std::endl;
			}
			output << endOfBatchSep << std::endl;
		}
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

bool CUndoChangeLog::Load( std::list< fs::PathPairMap >& rUndoStack )
{
	const std::tstring& undoLogPath = GetFilePath();
	std::ifstream input( str::ToUtf8( undoLogPath.c_str() ).c_str() );

	if ( !input.is_open() )
		return false;

	fs::PathPairMap batch;

	// note: most recent rename batch is the last in the undo log
	for ( unsigned int parseLineNo = 1; !input.eof(); ++parseLineNo )
	{
		std::string line;
		std::getline( input, line );

		if ( !line.empty() )		// ignore empty lines
			if ( pred::Equal == str::CompareNoCase( line, endOfBatchSep ) && !batch.empty() )
			{
				rUndoStack.push_back( fs::PathPairMap() );
				rUndoStack.back().swap( batch );
			}
			else
			{
				static const size_t sepLength = filePairSep.length();
				size_t sepPos = line.find( filePairSep );
				if ( sepPos != std::string::npos )
				{
					fs::CPath sourcePath( str::FromUtf8( line.substr( 0, sepPos ).c_str() ) );
					fs::CPath destPath( str::FromUtf8( line.substr( sepPos + sepLength ).c_str() ) );

					if ( !sourcePath.IsEmpty() && !destPath.IsEmpty() )
						batch[ sourcePath ] = destPath;
				}
				else
					TRACE( _T(" * CUndoChangeLog::Load(): %s - ignoring badly formed line: %s\n"), undoLogPath.c_str(), line.c_str() );
			}
	}
	input.close();
	return true;
}
