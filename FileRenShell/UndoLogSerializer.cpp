
#include "stdafx.h"
#include "UndoLogSerializer.h"
#include "FileCommands.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/TimeUtl.h"
#include "utl/StringRange.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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

	bool ParseCommandTag( cmd::Command& rCommand, CTime& rTimestamp, const str::TStringRange& tagRange )
	{
		if ( !tagRange.IsEmpty() )
		{
			std::tstring commandTag, timestampText;

			Range< size_t > sepPos;
			if ( tagRange.Find( sepPos, _T(' ') ) )
				tagRange.SplitPair( commandTag, timestampText, sepPos );
			else
				commandTag = tagRange.Extract();

			if ( cmd::GetTags_Command().ParseKeyAs( rCommand, commandTag ) )
			{
				rTimestamp = time_utl::ParseTimestamp( timestampText );
				return true;
			}
		}
		return false;				// not an action tag, therefore a content tag
	}
}


CUndoLogSerializer::CUndoLogSerializer( std::deque< utl::ICommand* >* pOutUndoStack )
	: m_pUndoStack( pOutUndoStack )
	, m_parseLineNo( 0 )
{
	ASSERT_PTR( m_pUndoStack );
}

void CUndoLogSerializer::Save( std::ostream& os ) const
{
	for ( size_t i = 0; i != m_pUndoStack->size(); ++i )
	{
		const CMacroCommand* pMacroCmd = checked_static_cast< const CMacroCommand* >( m_pUndoStack->at( i ) );
		ASSERT( !pMacroCmd->IsEmpty() );

		if ( i != 0 )
			os << std::endl;		// inner batch extra line-end separator

		os << fmt::FormatTag( pMacroCmd->Format( true ).c_str() ) << std::endl;		// action tag line

		for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = pMacroCmd->GetSubCommands().begin(); itSubCmd != pMacroCmd->GetSubCommands().end(); ++itSubCmd )
			os << ( *itSubCmd )->Format( false ) << std::endl;						// no action tag

		os << fmt::FormatTag( fmt::s_tagEndOfBatch ) << std::endl;
	}
}

void CUndoLogSerializer::Load( std::istream& is )
{
	m_parseLineNo = 1;
	for ( ; !is.eof(); ++m_parseLineNo )
	{
		std::tstring line = stream::InputLine( is );
		str::TStringRange textRange( line );
		textRange.Trim();

		if ( !textRange.IsEmpty() )		// ignore empty lines
			if ( fmt::ParseTag( textRange ) )
				if ( utl::ICommand* pMacroCmd = LoadMacroCmd( is, textRange ) )
				{
					m_pUndoStack->push_back( pMacroCmd );
					continue;
				}
	}
}

utl::ICommand* CUndoLogSerializer::LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange )
{
	cmd::Command command;
	CTime timestamp;

	if ( fmt::ParseCommandTag( command, timestamp, tagRange ) )
	{
		std::auto_ptr< cmd::CFileMacroCmd > pMacroCmd( new cmd::CFileMacroCmd( command, timestamp ) );

		for ( ; !is.eof(); ++m_parseLineNo )
		{
			std::tstring line = stream::InputLine( is );
			str::TStringRange textRange( line );
			textRange.Trim();
			if ( textRange.IsEmpty() )
				continue;			// ignore empty lines

			if ( fmt::ParseTag( textRange ) && textRange.Equals( fmt::s_tagEndOfBatch ) )
				return !pMacroCmd->IsEmpty() ? pMacroCmd.release() : NULL;
			else if ( utl::ICommand* pSubCmd = LoadSubCmd( command, textRange ) )
				pMacroCmd->AddCmd( pSubCmd );
		}
	}

	return NULL;
}

utl::ICommand* CUndoLogSerializer::LoadSubCmd( cmd::Command command, const str::TStringRange& textRange )
{
	switch ( command )
	{
		case cmd::RenameFile:
		{
			fs::CPath srcPath, destPath;
			if ( fmt::ParseRenameEntry( srcPath, destPath, textRange ) )
				return new CRenameFileCmd( srcPath, destPath );
			break;
		}
		case cmd::TouchFile:
		{
			fs::CFileState srcFileState, destFileState;
			if ( fmt::ParseTouchEntry( srcFileState, destFileState, textRange ) )
				return new CTouchFileCmd( srcFileState, destFileState );
			break;
		}
	}
	return NULL;
}
