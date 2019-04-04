
#include "stdafx.h"
#include "EditingCommands.h"
#include "FileModel.h"
#include "RenameItem.h"
#include "TouchItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseChangeDestCmd implementation

CBaseChangeDestCmd::CBaseChangeDestCmd( cmd::CommandType cmdType, CFileModel* pFileModel, const std::tstring& cmdTag )
	: CCommand( cmdType, pFileModel, &cmd::GetTags_CommandType() )
	, m_cmdTag( cmdTag )
	, m_pFileModel( pFileModel )
	, m_hasOldDests( false )
{
	SetSubject( m_pFileModel );
}

std::tstring CBaseChangeDestCmd::Format( utl::Verbosity verbosity ) const
{
	if ( !m_cmdTag.empty() )
		return m_cmdTag;

	return __super::Format( verbosity );
}

bool CBaseChangeDestCmd::Execute( void )
{
	return ToggleExecute();
}

bool CBaseChangeDestCmd::Unexecute( void )
{
	// Since Execute() toggles between m_destPaths and OldDestPaths, calling the second time has the effect of Unexecute().
	// This assumes that this command is always executed through the command model for UNDO/REDO.
	REQUIRE( m_hasOldDests );
	return ToggleExecute();
}

bool CBaseChangeDestCmd::IsUndoable( void ) const
{
	return Changed == EvalChange();
}


// CChangeDestPathsCmd implementation

CChangeDestPathsCmd::CChangeDestPathsCmd( CFileModel* pFileModel, std::vector< fs::CPath >& rNewDestPaths, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestPaths, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetRenameItems().size() == rNewDestPaths.size() );

	m_srcPaths.reserve( m_pFileModel->GetRenameItems().size() );
	for ( std::vector< CRenameItem* >::const_iterator itItem = m_pFileModel->GetRenameItems().begin(); itItem != m_pFileModel->GetRenameItems().end(); ++itItem )
		m_srcPaths.push_back( ( *itItem )->GetSrcPath() );

	m_destPaths.swap( rNewDestPaths );
	ENSURE( m_srcPaths.size() == m_destPaths.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestPathsCmd::EvalChange( void ) const
{
	REQUIRE( m_srcPaths.size() == m_destPaths.size() );

	if ( m_destPaths.size() != m_pFileModel->GetRenameItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
	{
		const CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[ i ];

		if ( pRenameItem->GetFilePath() != m_srcPaths[ i ] )						// keys different?
			return Expired;
		else if ( pRenameItem->GetDestPath().Get() != m_destPaths[ i ].Get() )	// case sensitive string compare
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestPathsCmd::ToggleExecute( void )
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
			{
				CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[ i ];

				std::swap( pRenameItem->RefDestPath(), m_destPaths[ i ] );		// from now on m_destPaths stores OldDestPaths
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}


// CChangeDestFileStatesCmd implementation

CChangeDestFileStatesCmd::CChangeDestFileStatesCmd( CFileModel* pFileModel, std::vector< fs::CFileState >& rNewDestStates, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestFileStates, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetTouchItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetTouchItems().size() == rNewDestStates.size() );

	m_srcStates.reserve( m_pFileModel->GetTouchItems().size() );
	for ( std::vector< CTouchItem* >::const_iterator itItem = m_pFileModel->GetTouchItems().begin(); itItem != m_pFileModel->GetTouchItems().end(); ++itItem )
		m_srcStates.push_back( ( *itItem )->GetSrcState() );

	m_destStates.swap( rNewDestStates );
	ENSURE( m_srcStates.size() == m_destStates.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestFileStatesCmd::EvalChange( void ) const
{
	REQUIRE( m_srcStates.size() == m_destStates.size() );

	if ( m_destStates.size() != m_pFileModel->GetTouchItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
	{
		const CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[ i ];

		if ( pTouchItem->GetFilePath() != m_srcStates[ i ].m_fullPath )			// keys different?
			return Expired;
		else if ( pTouchItem->GetDestState() != m_destStates[ i ] )
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestFileStatesCmd::ToggleExecute( void )
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
			{
				CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[ i ];

				std::swap( pTouchItem->RefDestState(), m_destStates[ i ] );		// from now on m_destStates stores OldDestStates
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}


// CResetDestinationsCmd implementation

CResetDestinationsCmd::CResetDestinationsCmd( CFileModel* pFileModel )
	: CMacroCommand( _T("Reset"), cmd::ResetDestinations )
{
	if ( !pFileModel->GetRenameItems().empty() )		// lazy initialized?
	{
		std::vector< fs::CPath > destPaths; destPaths.reserve( pFileModel->GetRenameItems().size() );
		for ( std::vector< CRenameItem* >::const_iterator itRenameItem = pFileModel->GetRenameItems().begin(); itRenameItem != pFileModel->GetRenameItems().end(); ++itRenameItem )
			destPaths.push_back( ( *itRenameItem )->GetSrcPath() );		// DEST = SRC

		AddCmd( new CChangeDestPathsCmd( pFileModel, destPaths, m_userInfo ) );
	}

	if ( !pFileModel->GetTouchItems().empty() )			// lazy initialized?
	{
		std::vector< fs::CFileState > destStates; destStates.reserve( pFileModel->GetTouchItems().size() );
		for ( std::vector< CTouchItem* >::const_iterator itTouchItem = pFileModel->GetTouchItems().begin(); itTouchItem != pFileModel->GetTouchItems().end(); ++itTouchItem )
			destStates.push_back( ( *itTouchItem )->GetSrcState() );

		AddCmd( new CChangeDestFileStatesCmd( pFileModel, destStates, m_userInfo ) );
	}
}

std::tstring CResetDestinationsCmd::Format( utl::Verbosity verbosity ) const
{
	return GetSubCommands().front()->Format( verbosity );
}
