
#include "stdafx.h"
#include "CommandModel.h"
#include "ContainerUtilities.h"
#include "EnumTags.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	const CEnumTags& GetTags_ExecMode( void )
	{
		static const CEnumTags s_tags( _T("|Undo|Redo"), _T("|UNDO|REDO") );
		return s_tags;
	}
}


// CScopedExecMode implementation

CScopedExecMode::CScopedExecMode( utl::ExecMode execMode )
	: m_oldExecMode( CCommandModel::s_execMode )
{
	CCommandModel::s_execMode = execMode;
}

CScopedExecMode::~CScopedExecMode()
{
	CCommandModel::s_execMode = m_oldExecMode;
}


// CCommandModel implementation

utl::ExecMode CCommandModel::s_execMode = utl::ExecDo;

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

	// UNDO takes 2/3 and REDO 1/3 of the history size. We expire oldest commands (at front of stack).

	const size_t redoMaxSize = utl::min( m_redoStack.size(), utl::max( maxSize / 3, 1 ) );

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

	CScopedExecMode scopedExec( utl::ExecDo );

	try
	{
		if ( pCmd->Execute() )
		{
			Push( pCmd );
			return true;
		}
	}
	catch ( CUserAbortedException& exc )
	{	// cancelled by the user
		exc; TRACE( _T(" * Execute command %s: %s\n"), pCmd->Format( utl::Detailed ).c_str(), exc.GetMessage().c_str() );
	}

	delete pCmd;
	return false;
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

	CScopedExecMode scopedExec( utl::ExecUndo );
	bool succeeded = false;

	while ( stepCount-- != 0 && !m_undoStack.empty() )
	{
		std::auto_ptr<utl::ICommand> pCmd( m_undoStack.back() );
		m_undoStack.pop_back();

		try
		{
			succeeded = pCmd->IsUndoable() && pCmd->Unexecute();
			if ( succeeded )								// not a zombie command?
				m_redoStack.push_back( pCmd.release() );
		}
		catch ( CUserAbortedException& exc )
		{	// cancelled by the user: retain status-quo
			exc; TRACE( _T(" * Un-execute command %s: %s\n"), pCmd->Format( utl::Detailed ).c_str(), exc.GetMessage().c_str() );

			m_undoStack.push_back( pCmd.release() );		// put it back, ready for unexecution again
		}
	}

	return succeeded;
}

bool CCommandModel::Redo( size_t stepCount /*= 1*/ )
{
	ASSERT( stepCount != 0 && stepCount <= m_redoStack.size() );

	CScopedExecMode scopedExec( utl::ExecRedo );
	bool succeeded = false;

	while ( stepCount-- != 0 && !m_redoStack.empty() )
	{
		std::auto_ptr<utl::ICommand> pCmd( m_redoStack.back() );
		m_redoStack.pop_back();

		try
		{
			succeeded = pCmd->Execute();
			if ( succeeded )
				m_undoStack.push_back( pCmd.release() );
		}
		catch ( CUserAbortedException& exc )
		{	// cancelled by the user: retain status-quo
			exc; TRACE( _T(" * Re-execute command %s: %s\n"), pCmd->Format( utl::Detailed ).c_str(), exc.GetMessage().c_str() );

			m_redoStack.push_back( pCmd.release() );		// put it back, ready for re-execution again
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
