
#include "stdafx.h"
#include "UndoChangeLog.h"
#include "utl/EnumTags.h"
#include "utl/Guards.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/TimeUtl.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_sep[] = _T("|");

	enum { Attributes, CreationTime, ModifTime, AccessTime, _FieldCount };

	std::tstring DoFormatFileState( const fs::CFileState& state )
	{
		std::vector< std::tstring > parts;
		if ( !state.IsEmpty() )
		{
			parts.push_back( str::Format( _T("0x%08X"), state.m_attributes ) );
			parts.push_back( time_utl::FormatTimestamp( state.m_creationTime ) );
			parts.push_back( time_utl::FormatTimestamp( state.m_modifTime ) );
			parts.push_back( time_utl::FormatTimestamp( state.m_accessTime ) );
		}
		return str::Join( parts, s_sep );
	}

	bool DoParseFileState( fs::CFileState& rState, const std::tstring& text )
	{
		std::vector< std::tstring > parts;
		str::Split( parts, text.c_str(), s_sep );
		if ( parts.size() != _FieldCount )
		{
			rState.Clear();
			return false;
		}

		unsigned int attributes;
		str::TStringRange attrRange( parts[ Attributes ] );
		if ( attrRange.StripPrefix( _T("0x") ) )
			if ( 1 == _stscanf( attrRange.Extract().c_str(), _T("%X"), &attributes ) )
				rState.m_attributes = static_cast< BYTE >( attributes );

		rState.m_creationTime = time_utl::ParseTimestamp( parts[ CreationTime ] );
		rState.m_modifTime = time_utl::ParseTimestamp( parts[ ModifTime ] );
		rState.m_accessTime = time_utl::ParseTimestamp( parts[ AccessTime ] );
		return true;
	}

	std::tstring FormatBraces( const TCHAR core[], const TCHAR braces[] )
	{
		ASSERT( str::GetLength( braces ) >= 2 );
		return str::Format( _T("%c%s%c"), braces[ 0 ], core, braces[ 1 ] );
	}

	bool ParseBraces( str::TStringRange& rTextRange, const TCHAR braces[] )
	{
		ASSERT( str::GetLength( braces ) >= 2 );
		rTextRange.Trim();
		return rTextRange.Strip( braces[ 0 ], braces[ 1 ] );
	}
}


namespace fmt
{
	static const TCHAR s_pairSep[] = _T(" -> ");
	static const TCHAR s_touchSep[] = _T(" :: ");
	static const TCHAR s_stateBraces[] = { _T("{}") };

	std::tstring FormatFileState( const fs::CFileState& fileState )
	{
		return FormatBraces( DoFormatFileState( fileState ).c_str(), s_stateBraces );
	}

	bool ParseFileState( fs::CFileState& rState, str::TStringRange& rTextRange )
	{
		return
			ParseBraces( rTextRange, s_stateBraces ) ) &&
			DoParseFileState( rState, rTextRange.Extract() );
	}

	std::tstring FormatRenameEntry( const fs::CPath& srcPath, const fs::CPath& destPath )
	{
		return srcPath.Get() + s_pairSep + destPath.Get();
	}

	bool ParseRenameEntry( fs::CPath& rSrcPath, fs::CPath& rDestPath, const str::TStringRange& textRange )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_pairSep ) )
		{
			str::TStringRange srcRange = textRange.MakeLead( sepPos.m_start );
			str::TStringRange destRange = textRange.MakeTrail( sepPos.m_end );
			srcRange.Trim();
			destRange.Trim();

			rSrcPath = srcRange.Extract();
			rDestPath = destRange.Extract();
			return !rSrcPath.IsEmpty() && !rDestPath.IsEmpty();
		}
		return false;
	}

	std::tstring FormatTouchEntry( const fs::CFileState& srcState, const fs::CFileState& destState )
	{
		ASSERT( srcState.m_fullPath == destState.m_fullPath );
		return srcState.m_fullPath.Get() + s_touchSep + FormatFileState( srcState ) + s_pairSep + FormatFileState( destState );
	}

	bool ParseTouchEntry( fs::CFileState& rSrcState, fs::CFileState& rDestState, const str::TStringRange& textRange )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_touchSep ) )
		{
			rSrcState.m_fullPath = rDestState.m_fullPath = textRange.ExtractLead( sepPos.m_start );
			str::TStringRange nextRange = textRange.MakeTrail( sepPos.m_end );

			if ( nextRange.Find( sepPos, s_pairSep ) )
			{
				str::TStringRange srcRange = nextRange.MakeLead( sepPos.m_start );
				str::TStringRange destRange = nextRange.MakeTrail( sepPos.m_end );

				if ( ParseFileState( rSrcState, srcRange ) )
					if ( ParseFileState( rDestState, destRange ) )
						return true;
			}
		}
		return false;
	}
}


namespace fmt
{
	static const TCHAR s_tagSeps[] = _T("<>");		// "<tag>"
	static const TCHAR s_tagEndOfBatch[] = _T("END OF BATCH");

	std::tstring FormatTag( const TCHAR tag[] )
	{
		ASSERT( !str::IsEmpty( tag ) );
		return FormatBraces( tag, s_tagSeps );
	}

	bool ParseTag( str::TStringRange& rTextRange )
	{
		return ParseBraces( rTextRange, s_tagSeps );
	}
}


// CUndoChangeLog implementation

void CUndoChangeLog::Clear( void )
{
	m_renameUndoStack.Clear();
	m_touchUndoStack.Clear();
}

const fs::CPath& CUndoChangeLog::GetFilePath( void )
{
	static fs::CPath undoLogPath;
	if ( undoLogPath.IsEmpty() )
	{
		TCHAR fullPath[ _MAX_PATH ];
		::GetModuleFileName( AfxGetApp()->m_hInstance, fullPath, COUNT_OF( fullPath ) );

		fs::CPathParts parts( fullPath );
		parts.m_fname += _T("_undo");
		parts.m_ext = _T(".log");
		undoLogPath.Set( parts.MakePath() );
	}

	return undoLogPath;
}

bool CUndoChangeLog::Save( void ) const
{
	const fs::CPath& undoLogPath = GetFilePath();
	std::ofstream output( undoLogPath.GetUtf8().c_str(), std::ios_base::out | std::ios_base::trunc );

	if ( output.is_open() )
	{
		Save( output );
		output.close();
	}
	if ( output.fail() )
	{
		TRACE( _T(" * CUndoChangeLog::Save(): error saving undo change log file: %s\n"), undoLogPath.GetPtr() );
		ui::BeepSignal( MB_ICONERROR );
		return false;
	}
	return true;
}

bool CUndoChangeLog::Load( void )
{
	const fs::CPath& undoLogPath = GetFilePath();
	std::ifstream input( undoLogPath.GetUtf8().c_str() );

	if ( !input.is_open() )
		return false;

	Load( input );

	input.close();
	return true;
}

void CUndoChangeLog::Save( std::ostream& os ) const
{
	//utl::CSlowSectionGuard slowGuard( _T("CUndoChangeLog::Save()"), 0.2 );

	SaveRenameBatches( os );
	SaveTouchBatches( os );
}

void CUndoChangeLog::Load( std::istream& is )
{
	//utl::CSlowSectionGuard slowGuard( _T("CUndoChangeLog::Load()"), 0.2 );

	Clear();

	Action action = Rename;
	CTime timestamp;
	fs::TPathPairMap batchRename;
	fs::TFileStatePairMap batchTouch;

	// note: most recent rename batch is the last in the undo log
	for ( unsigned int parseLineNo = 1; !is.eof(); ++parseLineNo )
	{
		std::tstring line = stream::InputLine( is );
		str::TStringRange textRange( line );
		textRange.Trim();
		if ( textRange.IsEmpty() )
			continue;			// ignore empty lines

		if ( fmt::ParseTag( textRange ) )
		{
			if ( textRange.Equals( fmt::s_tagEndOfBatch ) )
			{
				switch ( action )
				{
					case Rename:
						if ( !batchRename.empty() )
							m_renameUndoStack.Push( timestamp ).swap( batchRename );
						break;
					case Touch:
						if ( !batchTouch.empty() )
							m_touchUndoStack.Push( timestamp ).swap( batchTouch );
						break;
					default: ASSERT( false );
				}
				timestamp = CTime();
				continue;			// consume the tag line
			}
			else if ( ParseActionTag( action, timestamp, textRange ) )
				continue;			// consume the tag line
		}
		else
		{
			switch ( action )
			{
				case Rename:
					if ( AddRenameLine( batchRename, textRange ) )
						continue;
					break;
				case Touch:
					if ( AddTouchLine( batchTouch, textRange ) )
						continue;
					break;
				default: ASSERT( false );
			}
		}

		TRACE( _T(" * CUndoChangeLog::Load(): ignore badly formed line: '%s'\n"), line.c_str() );
	}
}

void CUndoChangeLog::SaveRenameBatches( std::ostream& os ) const
{
	for ( std::list< CBatch< fs::TPathPairMap > >::const_iterator itBatch = m_renameUndoStack.Get().begin(), itFirst = itBatch; itBatch != m_renameUndoStack.Get().end(); ++itBatch )
	{
		if ( itBatch != itFirst )
			os << std::endl;		// inner batch extra line-end separator

		os << FormatActionTag( Rename, itBatch->m_timestamp ) << std::endl;		// add action tag

		for ( fs::TPathPairMap::const_iterator itPair = itBatch->m_batch.begin(); itPair != itBatch->m_batch.end(); ++itPair )
		{
			ASSERT( !itPair->first.IsEmpty() && !itPair->second.IsEmpty() );
			os << fmt::FormatRenameEntry( itPair->first, itPair->second ) << std::endl;
		}
		os << fmt::FormatTag( fmt::s_tagEndOfBatch ) << std::endl;
	}
}

void CUndoChangeLog::SaveTouchBatches( std::ostream& os ) const
{
	if ( !m_renameUndoStack.Get().empty() && !m_touchUndoStack.Get().empty() )
		os << std::endl;			// extra line-end separator between RENAME and TOUCH sections

	for ( std::list< CBatch< fs::TFileStatePairMap > >::const_iterator itBatch = m_touchUndoStack.Get().begin(), itFirst = itBatch; itBatch != m_touchUndoStack.Get().end(); ++itBatch )
	{
		if ( itBatch != itFirst )
			os << std::endl;		// inner batch extra line-end separator

		os << FormatActionTag( Touch, itBatch->m_timestamp ) << std::endl;		// add action tag

		for ( fs::TFileStatePairMap::const_iterator itPair = itBatch->m_batch.begin(); itPair != itBatch->m_batch.end(); ++itPair )
			os << fmt::FormatTouchEntry( itPair->first, itPair->second ) << std::endl;

		os << fmt::FormatTag( fmt::s_tagEndOfBatch ) << std::endl;
	}
}

bool CUndoChangeLog::AddRenameLine( fs::TPathPairMap& rBatchRename, const str::TStringRange& textRange )
{
	fs::CPath srcPath, destPath;
	if ( fmt::ParseRenameEntry( srcPath, destPath, textRange ) )
	{
		rBatchRename[ srcPath ] = destPath;
		return true;
	}
	return false;
}

bool CUndoChangeLog::AddTouchLine( fs::TFileStatePairMap& rBatchTouch, const str::TStringRange& textRange )
{
	fs::CFileState srcFileState, destFileState;
	if ( fmt::ParseTouchEntry( srcFileState, destFileState, textRange ) )
	{
		rBatchTouch[ srcFileState ] = destFileState;
		return true;
	}
	return false;
}


const CEnumTags& CUndoChangeLog::GetTags_Action( void )
{
	static const CEnumTags tags( _T("RENAME|TOUCH") );
	return tags;
}

std::tstring CUndoChangeLog::FormatActionTag( Action action, const CTime& timestamp )
{
	std::tstring text; text.reserve( 64 );
	text = GetTags_Action().GetUiTags()[ action ];
	if ( timestamp.GetTime() != 0 )
		text += _T(' ') + time_utl::FormatTimestamp( timestamp );

	return fmt::FormatTag( text.c_str() );
}

bool CUndoChangeLog::ParseActionTag( Action& rAction, CTime& rTimestamp, const str::TStringRange& tagRange )
{
	if ( !tagRange.IsEmpty() )
	{
		std::tstring actionTag, timestampText;

		Range< size_t > sepPos;
		if ( tagRange.Find( sepPos, _T(' ') ) )
			tagRange.SplitPair( actionTag, timestampText, sepPos );
		else
			actionTag = tagRange.Extract();

		if ( GetTags_Action().ParseUiAs( rAction, actionTag ) )
		{
			rTimestamp = time_utl::ParseTimestamp( timestampText );
			return true;
		}
	}
	return false;				// not an action tag, therefore a content tag
}
