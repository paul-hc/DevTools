
#include "stdafx.h"
#include "AppCmdService.h"
#include "CommandModelPersist.h"
#include "GeneralOptions.h"
#include "utl/CommandModel.h"
#include "utl/RuntimeException.h"

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

bool CAppCmdService::SafeExecuteCmd( utl::ICommand* pCmd, bool execInline /*= false*/ )
{
	if ( !execInline )
		if ( !CGeneralOptions::Instance().m_undoEditingCmds )
			execInline = !cmd::IsPersistentCmd( pCmd );				// local editing command?

	return CCommandService::SafeExecuteCmd( pCmd, execInline );
}

bool CAppCmdService::UndoAt( size_t topPos )
{
	std::deque< utl::ICommand* >& rUndoStack = RefUndoStack();

	size_t pos = rUndoStack.size() - topPos - 1;		// position from the begining
	if ( pos >= rUndoStack.size() )
	{
		ASSERT( false );
		return false;
	}

	CScopedExecMode scopedExec( utl::ExecUndo );

	std::deque< utl::ICommand* >::iterator itCmd = rUndoStack.begin() + pos;
	std::auto_ptr< utl::ICommand > pCmd( *itCmd );
	itCmd = rUndoStack.erase( itCmd );

	try
	{
		if ( pCmd->IsUndoable() && pCmd->Unexecute() )
		{
			std::deque< utl::ICommand* >& rRedoStack = RefRedoStack();
			rRedoStack.push_back( pCmd.release() );
			return true;
		}
	}
	catch ( CUserAbortedException& exc )
	{	// cancelled by the user: retain status-quo
		TRACE( _T(" * Un-execute command %s: %s\n"), pCmd->Format( utl::Detailed ).c_str(), exc.GetMessage().c_str() );

		rUndoStack.insert( itCmd, pCmd.release() );		// put it back, ready for unexecution again
	}

	return false;
}

bool CAppCmdService::RedoAt( size_t topPos )
{
	std::deque< utl::ICommand* >& rRedoStack = RefRedoStack();

	size_t pos = rRedoStack.size() - topPos - 1;		// position from the begining
	if ( pos >= rRedoStack.size() )
	{
		ASSERT( false );
		return false;
	}

	CScopedExecMode scopedExec( utl::ExecUndo );

	std::deque< utl::ICommand* >::iterator itCmd = rRedoStack.begin() + pos;
	std::auto_ptr< utl::ICommand > pCmd( *itCmd );
	itCmd = rRedoStack.erase( itCmd );

	try
	{
		if ( pCmd->Execute() )
		{
			std::deque< utl::ICommand* >& rUndoStack = RefUndoStack();
			rUndoStack.push_back( pCmd.release() );
			return true;
		}
	}
	catch ( CUserAbortedException& exc )
	{	// cancelled by the user: retain status-quo
		TRACE( _T(" * Re-execute command %s: %s\n"), pCmd->Format( utl::Detailed ).c_str(), exc.GetMessage().c_str() );

		rRedoStack.insert( itCmd, pCmd.release() );		// put it back, ready for re-execution again
	}

	return false;
}

bool CAppCmdService::UndoRedoAt( svc::StackType stackType, size_t topPos )
{
	SetDirty( true );
	return svc::Undo == stackType ? UndoAt( topPos ) : RedoAt( topPos );
}
