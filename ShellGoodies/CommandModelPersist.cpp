
#include "stdafx.h"
#include "CommandModelPersist.h"
#include "FileMacroCommands.h"
#include "utl/Command.h"
#include "utl/CommandModel.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/FileSystem.h"
#include "utl/Guards.h"
#include "utl/Serialization.h"
#include "utl/StringRange.h"
#include "utl/StringUtilities.h"
#include "utl/Timer.h"
#include "utl/TimeUtils.h"
#include "utl/UI/MfcUtilities.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCommandModelPersist class

bool CCommandModelPersist::SaveUndoLog( const CCommandModel& commandModel, cmd::FileFormat fileFormat )
{
	//utl::CSlowSectionGuard slow( _T("CCommandModelPersist::SaveUndoLog"), 0.01 );

	std::auto_ptr<cmd::CLogSerializer> pSerializer( CreateSerializer( const_cast<CCommandModel*>( &commandModel ), fileFormat ) );
	return pSerializer->Save( GetUndoLogPath( fileFormat ) );
}

bool CCommandModelPersist::LoadUndoLog( CCommandModel* pOutCommandModel, cmd::FileFormat* pOutFileFormat /*= NULL*/ )
{
	cmd::FileFormat fileFormat;
	if ( !FindSavedUndoLogPath( fileFormat ) )
		return false;

	if ( pOutFileFormat != NULL )
		*pOutFileFormat = fileFormat;

	//utl::CSlowSectionGuard slow( _T("CCommandModelPersist::LoadUndoLog()"), 0.01 );
	std::auto_ptr<cmd::CLogSerializer> pSerializer( CreateSerializer( pOutCommandModel, fileFormat ) );
	return pSerializer->Load( GetUndoLogPath( fileFormat ) );
}

cmd::CLogSerializer* CCommandModelPersist::CreateSerializer( CCommandModel* pCommandModel, cmd::FileFormat fileFormat )
{
	switch ( fileFormat )
	{
		case cmd::TextFormat:	return new cmd::CTextLogSerializer( pCommandModel );
		case cmd::BinaryFormat:	return new cmd::CBinaryLogSerializer( pCommandModel );
	}
	ASSERT( false );
	return NULL;
}

fs::CPath CCommandModelPersist::GetUndoLogPath( cmd::FileFormat fileFormat )
{
	static const CEnumTags fmtExtTags( _T(".log|.dat") );

	TCHAR fullPath[ _MAX_PATH ];
	::GetModuleFileName( AfxGetApp()->m_hInstance, fullPath, COUNT_OF( fullPath ) );

	fs::CPathParts parts( fullPath );
	parts.m_fname += _T("_undo");
	parts.m_ext = fmtExtTags.FormatUi( fileFormat );
	return fs::CPath( parts.MakePath() );
}

bool CCommandModelPersist::FindSavedUndoLogPath( cmd::FileFormat& rFileFormat )
{
	CTime txtModifyTime = fs::ReadLastModifyTime( GetUndoLogPath( cmd::TextFormat ) );
	CTime binModifyTime = fs::ReadLastModifyTime( GetUndoLogPath( cmd::BinaryFormat ) );
	bool txtLogExists = time_utl::IsValid( txtModifyTime );
	bool binLogExists = time_utl::IsValid( binModifyTime );

	if ( txtLogExists && binLogExists )
		rFileFormat = txtModifyTime > binModifyTime ? cmd::TextFormat : cmd::BinaryFormat;		// choose the last modified one
	else if ( txtLogExists )
		rFileFormat = cmd::TextFormat;
	else if ( binLogExists )
		rFileFormat = cmd::BinaryFormat;
	else
		return false;

	return true;
}


namespace cmd
{
	// CLogSerializer class

	const TCHAR CLogSerializer::s_sectionTagSeps[] = _T("[]");		// "[section]"

	const CEnumTags& CLogSerializer::GetTags_Section( void )
	{
		static const CEnumTags tags( _T("UNDO SECTION|REDO SECTION") );
		return tags;
	}

	std::tstring CLogSerializer::FormatSectionTag( const TCHAR tag[] )
	{
		ASSERT( !str::IsEmpty( tag ) );
		return fmt::FormatBraces( tag, s_sectionTagSeps );
	}

	bool CLogSerializer::ParseSectionTag( str::TStringRange& rTextRange )
	{
		return fmt::ParseBraces( rTextRange, s_sectionTagSeps );
	}


	// CTextLogSerializer class

	const TCHAR CTextLogSerializer::s_tagSeps[] = _T("<>");			// "<tag>"
	const TCHAR CTextLogSerializer::s_tagEndOfBatch[] = _T("END OF BATCH");

	bool CTextLogSerializer::Save( const fs::CPath& undoLogPath ) override
	{
		std::ofstream output( undoLogPath.GetPtr(), std::ios_base::out | std::ios_base::trunc );

		if ( output.is_open() )
		{
			Save( output );
			output.close();
		}
		if ( output.fail() )
		{
			TRACE( _T(" * CTextLogSerializer::Save(): error saving undo changes log file: %s\n"), undoLogPath.GetPtr() );
			ASSERT( false );
			return false;
		}
		return true;
	}

	bool CTextLogSerializer::Load( const fs::CPath& undoLogPath ) override
	{
		std::ifstream input( undoLogPath.GetPtr() );

		if ( !input.is_open() )
			return false;				// undo log file doesn't exist

		Load( input );
		input.close();
		return true;
	}

	void CTextLogSerializer::Save( std::ostream& os ) const
	{
		SaveStack( os, svc::Undo, m_pCommandModel->GetUndoStack() );
		SaveStack( os, svc::Redo, m_pCommandModel->GetRedoStack() );
	}

	void CTextLogSerializer::SaveStack( std::ostream& os, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack ) const
	{
		if ( cmdStack.empty() )
			return;

		if ( svc::Redo == section )
			os << std::endl;		// push redo section down one line

		os << FormatSectionTag( GetTags_Section().FormatUi( section ).c_str() ) << std::endl;		// section tag

		for ( size_t i = 0; i != cmdStack.size(); ++i )
			if ( cmd::IsPersistentCmd( cmdStack[ i ] ) )
			{
				if ( i != 0 )
					os << std::endl;		// inner batch extra line-end separator

				if ( const CMacroCommand* pMacroCmd = dynamic_cast<const CMacroCommand*>( cmdStack[ i ] ) )
				{
					if ( !pMacroCmd->IsEmpty() )
					{
						os << FormatTag( pMacroCmd->Format( utl::Brief ).c_str() ) << std::endl;		// action tag line

						for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = pMacroCmd->GetSubCommands().begin(); itSubCmd != pMacroCmd->GetSubCommands().end(); ++itSubCmd )
							os << ( *itSubCmd )->Format( utl::Brief ) << std::endl;						// no action tag

						os << FormatTag( s_tagEndOfBatch ) << std::endl;
					}
				}
				else
					ASSERT( false );		// TODO: write text serialization code for this command type
			}
	}

	void CTextLogSerializer::Load( std::istream& is )
	{
		utl::CSlowSectionGuard slow( _T("UNDO/REDO log Load"), 0.1 );

		std::deque< utl::ICommand* > undoStack, redoStack;
		std::deque< utl::ICommand* >* pStack = &undoStack;
		CTimer timer;

		m_parseLineNo = 1;
		for ( std::tstring line; stream::InputLine( is, line ); ++m_parseLineNo )
		{
			str::TStringRange textRange( line );
			textRange.Trim();

			if ( !textRange.IsEmpty() )		// ignore empty lines
				if ( ParseSectionTag( textRange ) )
				{
					svc::StackType section;
					if ( GetTags_Section().ParseUiAs( section, textRange.Extract() ) )
						pStack = svc::Undo == section ? &undoStack : &redoStack;
				}
				else if ( ParseTag( textRange ) )
					if ( utl::ICommand* pMacroCmd = LoadMacroCmd( is, textRange ) )
					{
						pStack->push_back( pMacroCmd );
						continue;
					}
		}

		m_pCommandModel->SwapUndoStack( undoStack );
		m_pCommandModel->SwapRedoStack( redoStack );

		utl::ClearOwningContainer( undoStack );
		utl::ClearOwningContainer( redoStack );

		TRACE( _T("- CTextLogSerializer::Load(): takes %s\n"), timer.FormatElapsedDuration( 3 ).c_str() );
	}

	utl::ICommand* CTextLogSerializer::LoadMacroCmd( std::istream& is, const str::TStringRange& tagRange )
	{
		cmd::CommandType cmdType;
		CTime timestamp;

		if ( ParseCommandTag( cmdType, timestamp, tagRange ) )
		{
			std::auto_ptr<cmd::CFileMacroCmd> pMacroCmd( new cmd::CFileMacroCmd( cmdType, timestamp ) );

			for ( std::tstring line; stream::InputLine( is, line ); ++m_parseLineNo )
			{
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

	utl::ICommand* CTextLogSerializer::LoadSubCmd( cmd::CommandType cmdType, const str::TStringRange& textRange )
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

	std::tstring CTextLogSerializer::FormatTag( const TCHAR tag[] )
	{
		ASSERT( !str::IsEmpty( tag ) );
		return fmt::FormatBraces( tag, s_tagSeps );
	}

	bool CTextLogSerializer::ParseTag( str::TStringRange& rTextRange )
	{
		return fmt::ParseBraces( rTextRange, s_tagSeps );
	}

	bool CTextLogSerializer::ParseCommandTag( cmd::CommandType& rCmdType, CTime& rTimestamp, const str::TStringRange& tagRange )
	{
		if ( !tagRange.IsEmpty() )
		{
			std::tstring cmdTypeTag, timestampText;

			Range<size_t> sepPos;
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


	// CBinaryLogSerializer implementation

	bool CBinaryLogSerializer::Save( const fs::CPath& undoLogPath ) override
	{
		ui::CAdapterDocument doc( this, undoLogPath.Get() );
		return doc.Save();
	}

	bool CBinaryLogSerializer::Load( const fs::CPath& undoLogPath ) override
	{
		ui::CAdapterDocument doc( this, undoLogPath.Get() );
		return doc.Load();
	}

	void CBinaryLogSerializer::Save( CArchive& archive ) throws_( CException* )
	{
		SaveStack( archive, svc::Undo, m_pCommandModel->GetUndoStack() );
		SaveStack( archive, svc::Redo, m_pCommandModel->GetRedoStack() );
	}

	void CBinaryLogSerializer::Load( CArchive& archive ) throws_( CException* )
	{
		std::deque< utl::ICommand* > undoStack, redoStack;
		LoadStack( archive, undoStack );
		LoadStack( archive, redoStack );

		m_pCommandModel->SwapUndoStack( undoStack );
		m_pCommandModel->SwapRedoStack( redoStack );
	}

	void CBinaryLogSerializer::SaveStack( CArchive& archive, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack )
	{
		std::tstring sectionTag = FormatSectionTag( GetTags_Section().FormatUi( section ).c_str() );
		archive << &sectionTag;			// as Utf8; just for inspection

		serial::Save_CObjects_Mixed( archive, cmdStack );
	}

	void CBinaryLogSerializer::LoadStack( CArchive& archive, std::deque< utl::ICommand* >& rCmdStack )
	{
		std::tstring sectionTag; sectionTag;
		archive >> &sectionTag;			// as Utf8; just discard it

		serial::Load_CObjects_Mixed( archive, rCmdStack );
	}

} //namespace cmd
