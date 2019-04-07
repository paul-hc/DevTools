
#include "stdafx.h"
#include "AppCmdService.h"
#include "CommandModelPersist.h"
#include "GeneralOptions.h"
#include "utl/CommandModel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAppCmdService::CAppCmdService( void )
	: CCommandService()
{
}

CAppCmdService::~CAppCmdService()
{
}

bool CAppCmdService::SaveCommandModel( void )
{
	if ( !CGeneralOptions::Instance().m_undoLogPersist )
		return false;

	// cleanup the command stacks before saving
	m_pCommandModel->RemoveExpiredCommands( MaxCommands );
	RemoveCommandsThat( pred::IsZombieCmd() );		// zombie command: it has no effect on files (in most cases empty macros due to non-existing files)

	if ( !CCommandModelPersist::SaveUndoLog( *m_pCommandModel, CGeneralOptions::Instance().m_undoLogFormat ) )
		return false;

	SetDirty( false );
	return true;
}

bool CAppCmdService::LoadCommandModel( void )
{
	ASSERT( m_pCommandModel->IsUndoEmpty() && m_pCommandModel->IsRedoEmpty() );		// load once

	if ( CGeneralOptions::Instance().m_undoLogPersist )
		if ( CCommandModelPersist::LoadUndoLog( m_pCommandModel.get() ) )			// load the most recently modified log file (regardless of CGeneralOptions::m_undoLogFormat)
		{
			SetDirty( false );
			return true;
		}

	return false;
}

bool CAppCmdService::UndoAt( size_t topPos )
{
	std::deque< utl::ICommand* >* pUndoStack = &const_cast< std::deque< utl::ICommand* >& >( GetCommandModel()->GetUndoStack() );

	size_t pos = pUndoStack->size() - topPos - 1;		// position from the begining
	if ( pos >= pUndoStack->size() )
	{
		ASSERT( false );
		return false;
	}

	CScopedExecMode scopedExec( utl::ExecUndo );

	std::deque< utl::ICommand* >::iterator itCmd = pUndoStack->begin() + pos;
	std::auto_ptr< utl::ICommand > pCmd( *itCmd );
	pUndoStack->erase( itCmd );

	if ( pCmd->IsUndoable() && pCmd->Unexecute() )
	{
		std::deque< utl::ICommand* >* pRedoStack = &const_cast< std::deque< utl::ICommand* >& >( GetCommandModel()->GetRedoStack() );
		pRedoStack->push_back( pCmd.release() );
		return true;
	}

	return false;
}

bool CAppCmdService::RedoAt( size_t topPos )
{
	std::deque< utl::ICommand* >* pRedoStack = &const_cast< std::deque< utl::ICommand* >& >( GetCommandModel()->GetRedoStack() );

	size_t pos = pRedoStack->size() - topPos - 1;		// position from the begining
	if ( pos >= pRedoStack->size() )
	{
		ASSERT( false );
		return false;
	}

	CScopedExecMode scopedExec( utl::ExecUndo );

	std::deque< utl::ICommand* >::iterator itCmd = pRedoStack->begin() + pos;
	std::auto_ptr< utl::ICommand > pCmd( *itCmd );
	pRedoStack->erase( itCmd );

	if ( pCmd->Execute() )
	{
		std::deque< utl::ICommand* >* pUndoStack = &const_cast< std::deque< utl::ICommand* >& >( GetCommandModel()->GetUndoStack() );
		pUndoStack->push_back( pCmd.release() );
		return true;
	}
	return false;
}

bool CAppCmdService::UndoRedoAt( svc::StackType stackType, size_t topPos )
{
	SetDirty( true );
	return svc::Undo == stackType ? UndoAt( topPos ) : RedoAt( topPos );
}
