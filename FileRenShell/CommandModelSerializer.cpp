
#include "stdafx.h"
#include "CommandModelSerializer.h"
#include "FileCommands.h"
#include "utl/Command.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/TimeUtils.h"
#include "utl/StringRange.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CCommandModelSerializer::s_sectionTagSeps[] = _T("[]");		// "[section]"
const TCHAR CCommandModelSerializer::s_tagSeps[] = _T("<>");			// "<tag>"
const TCHAR CCommandModelSerializer::s_tagEndOfBatch[] = _T("END OF BATCH");

CCommandModelSerializer::CCommandModelSerializer( void )
	: m_parseLineNo( 0 )
{
}

void CCommandModelSerializer::Save( std::ostream& os, const CCommandModel& commandModel ) const
{
	SaveStack( os, cmd::Undo, commandModel.GetUndoStack() );
	SaveStack( os, cmd::Redo, commandModel.GetRedoStack() );
}

void CCommandModelSerializer::SaveStack( std::ostream& os, cmd::StackType section, const std::deque< utl::ICommand* >& cmdStack ) const
{
	if ( cmdStack.empty() )
		return;

	if ( cmd::Redo == section )
		os << std::endl;		// push redo section down one line

	os << FormatSectionTag( GetTags_Section().FormatUi( section ).c_str() ) << std::endl;		// section tag

	for ( size_t i = 0; i != cmdStack.size(); ++i )
	{
		const CMacroCommand* pMacroCmd = checked_static_cast< const CMacroCommand* >( cmdStack[ i ] );
		ASSERT( !pMacroCmd->IsEmpty() );

		if ( i != 0 )
			os << std::endl;		// inner batch extra line-end separator

		os << FormatTag( pMacroCmd->Format( false ).c_str() ) << std::endl;	// action tag line

		for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = pMacroCmd->GetSubCommands().begin(); itSubCmd != pMacroCmd->GetSubCommands().end(); ++itSubCmd )
			os << ( *itSubCmd )->Format( false ) << std::endl;						// no action tag

		os << FormatTag( s_tagEndOfBatch ) << std::endl;
	}
}

void CCommandModelSerializer::Load( std::istream& is, CCommandModel* pOutCommandModel )
{
	ASSERT_PTR( pOutCommandModel );

	std::deque< utl::ICommand* > undoStack, redoStack;
	std::deque< utl::ICommand* >* pStack = &undoStack;
	CTimer timer;

	m_parseLineNo = 1;
	for ( ; !is.eof(); ++m_parseLineNo )
	{
		std::tstring line = stream::InputLine( is );
		str::TStringRange textRange( line );
		textRange.Trim();

		if ( !textRange.IsEmpty() )		// ignore empty lines
			if ( ParseSectionTag( textRange ) )
			{
				cmd::StackType section;
				if ( GetTags_Section().ParseUiAs( section, textRange.Extract() ) )
					pStack = cmd::Undo == section ? &undoStack : &redoStack;
			}
			else if ( ParseTag( textRange ) )
				if ( utl::ICommand* pMacroCmd = LoadMacroCmd( is, textRange ) )
				{
					pStack->push_back( pMacroCmd );
					continue;
				}
	}

	pOutCommandModel->SwapUndoStack( undoStack );
	pOutCommandModel->SwapRedoStack( redoStack );

	utl::ClearOwningContainer( undoStack );
	utl::ClearOwningContainer( redoStack );

	TRACE( _T("- CCommandModelSerializer::Load(): takes %.3f seconds\n"), timer.ElapsedSeconds() );
}

utl::ICommand* CCommandModelSerializer::LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange )
{
	cmd::CommandType cmdType;
	CTime timestamp;

	if ( ParseCommandTag( cmdType, timestamp, tagRange ) )
	{
		std::auto_ptr< cmd::CFileMacroCmd > pMacroCmd( new cmd::CFileMacroCmd( cmdType, timestamp ) );

		for ( ; !is.eof(); ++m_parseLineNo )
		{
			std::tstring line = stream::InputLine( is );
			str::TStringRange textRange( line );
			textRange.Trim();
			if ( textRange.IsEmpty() )
				continue;			// ignore empty lines

			if ( ParseTag( textRange ) && textRange.Equals( s_tagEndOfBatch ) )
				return !pMacroCmd->IsEmpty() ? pMacroCmd.release() : NULL;
			else if ( utl::ICommand* pSubCmd = LoadSubCmd( cmdType, textRange ) )
				pMacroCmd->AddCmd( pSubCmd );
		}
	}

	return NULL;
}

utl::ICommand* CCommandModelSerializer::LoadSubCmd( cmd::CommandType cmdType, const str::TStringRange& textRange )
{
	switch ( cmdType )
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


// details

const CEnumTags& CCommandModelSerializer::GetTags_Section( void )
{
	static const CEnumTags tags( _T("UNDO SECTION|REDO SECTION") );
	return tags;
}

std::tstring CCommandModelSerializer::FormatSectionTag( const TCHAR tag[] )
{
	ASSERT( !str::IsEmpty( tag ) );
	return fmt::FormatBraces( tag, s_sectionTagSeps );
}

bool CCommandModelSerializer::ParseSectionTag( str::TStringRange& rTextRange )
{
	return fmt::ParseBraces( rTextRange, s_sectionTagSeps );
}

std::tstring CCommandModelSerializer::FormatTag( const TCHAR tag[] )
{
	ASSERT( !str::IsEmpty( tag ) );
	return fmt::FormatBraces( tag, s_tagSeps );
}

bool CCommandModelSerializer::ParseTag( str::TStringRange& rTextRange )
{
	return fmt::ParseBraces( rTextRange, s_tagSeps );
}

bool CCommandModelSerializer::ParseCommandTag( cmd::CommandType& rCmdType, CTime& rTimestamp, const str::TStringRange& tagRange )
{
	if ( !tagRange.IsEmpty() )
	{
		std::tstring cmdTypeTag, timestampText;

		Range< size_t > sepPos;
		if ( tagRange.Find( sepPos, _T(' ') ) )
			tagRange.SplitPair( cmdTypeTag, timestampText, sepPos );
		else
			cmdTypeTag = tagRange.Extract();

		if ( cmd::GetTags_CommandType().ParseKeyAs( rCmdType, cmdTypeTag ) )
		{
			rTimestamp = time_utl::ParseTimestamp( timestampText );
			return true;
		}
	}
	return false;				// not an action tag, therefore a content tag
}
