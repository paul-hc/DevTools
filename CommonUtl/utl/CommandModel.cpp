
#include "stdafx.h"
#include "CommandModel.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CCommandModel::~CCommandModel()
{
	Clear();
}

void CCommandModel::Clear( void )
{
	utl::ClearOwningContainer( m_undoStack );
	utl::ClearOwningContainer( m_redoStack );
}

void CCommandModel::RemoveExpiredCommands( size_t maxSize )
{
	ASSERT( maxSize > 1 );

	// UNDO takes 2/3 and REDO 1/3 of the history size. We expire oldest commands (at front).

	const size_t redoMaxSize = std::min( m_redoStack.size(), std::max( maxSize / 3, (size_t)1 ) );

	while ( m_redoStack.size() > redoMaxSize )
	{
		delete m_redoStack.front();
		m_redoStack.pop_front();
	}

	const size_t undoMaxSize = maxSize - redoMaxSize;

	while ( m_undoStack.size() > undoMaxSize )
	{
		delete m_undoStack.front();
		m_undoStack.pop_front();
	}
}

bool CCommandModel::Execute( utl::ICommand* pCmd )
{
	ASSERT_PTR( pCmd );

	if ( !pCmd->Execute() )
	{
		delete pCmd;
		return false;
	}

	Push( pCmd );
	return true;
}

void CCommandModel::Push( utl::ICommand* pCmd )
{
	ASSERT_PTR( pCmd );
	m_undoStack.push_back( pCmd );
	utl::ClearOwningContainer( m_redoStack );
}

bool CCommandModel::Undo( size_t stepCount /*= 1*/ )
{
	ASSERT( stepCount != 0 && stepCount <= m_undoStack.size() );

	bool succeeded = false;

	while ( stepCount-- != 0 && !m_undoStack.empty() )
	{
		utl::ICommand* pCmd = m_undoStack.back();

		if ( pCmd->IsUndoable() )
			succeeded = pCmd->Unexecute();

		if ( utl::Contains( m_undoStack, pCmd ) )		// not already removed?
		{
			m_undoStack.pop_back();
			m_redoStack.push_back( pCmd );
		}
	}

	return succeeded;
}

bool CCommandModel::Redo( size_t stepCount /*= 1*/ )
{
	ASSERT( stepCount != 0 && stepCount <= m_redoStack.size() );

	bool succeeded = true;

	while ( stepCount-- != 0 && !m_redoStack.empty() )
	{
		utl::ICommand* pCmd = m_redoStack.back();

		if ( !pCmd->Execute() )
			succeeded = false;

		if ( utl::Contains( m_redoStack, pCmd ) )		// not already removed?
		{
			m_redoStack.pop_back();
			m_undoStack.push_back( pCmd );
		}
	}

	return succeeded;
}

bool CCommandModel::CanUndo( void ) const
{
	for ( std::deque< utl::ICommand* >::const_reverse_iterator itCmd = m_undoStack.rbegin(); itCmd != m_undoStack.rend(); ++itCmd )
		if ( ( *itCmd )->IsUndoable() )
			return true;

	return false;
}

bool CCommandModel::CanRedo( void ) const
{
	return !m_redoStack.empty();
}
