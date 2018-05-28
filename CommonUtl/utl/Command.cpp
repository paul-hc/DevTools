
#include "stdafx.h"
#include "Command.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCommand implementation

CCommand::CCommand( unsigned int cmdId, utl::ISubject* pSubject )
	: m_cmdId( cmdId )
	, m_pSubject( pSubject )
{
	ASSERT( cmdId != 0 );
}

CCommand::~CCommand()
{
}

unsigned int CCommand::GetTypeID( void ) const
{
	return m_cmdId;
}

std::tstring CCommand::Format( bool detailed ) const
{
	detailed;
	return str::Load( m_cmdId );
}

bool CCommand::Unexecute( void )
{
	ASSERT( false );
	return false;
}

bool CCommand::IsUndoable( void ) const
{
	return true;
}

void CCommand::NotifyObservers( void )
{
	if ( m_pSubject != NULL )
		m_pSubject->UpdateAllObservers( this );
}


// CMacroCommand implementation

CMacroCommand::CMacroCommand( const std::tstring& userInfo /*= std::tstring()*/ )
	: m_userInfo( userInfo )
	, m_pMainCmd( NULL )
{
}

CMacroCommand::~CMacroCommand()
{
	utl::ClearOwningContainer( m_subCommands );
}

void CMacroCommand::AddCmd( utl::ICommand* pSubCmd )
{
	ASSERT_PTR( pSubCmd );
	ASSERT( !utl::Contains( m_subCommands, pSubCmd ) );

	m_subCommands.push_back( pSubCmd );
}

unsigned int CMacroCommand::GetTypeID( void ) const
{
	return MacroCmdId;
}

std::tstring CMacroCommand::Format( bool detailed ) const
{
	if ( m_pMainCmd != NULL )
		return m_pMainCmd->Format( detailed );			// main commmand provides all the info

	std::vector< std::tstring > cmdInfos;
	cmdInfos.reserve( m_subCommands.size() + 1 );

	if ( !m_userInfo.empty() )
		cmdInfos.push_back( m_userInfo + _T(":") );

	for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		cmdInfos.push_back( ( *itSubCmd )->Format( detailed ) );

	return str::Join( cmdInfos, detailed ? _T("\n") : _T(" ") );
}

bool CMacroCommand::Execute( void )
{
	bool succeded = !m_subCommands.empty();

	for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( !( *itSubCmd )->Execute() )
			succeded = false;

	return succeded;
}

bool CMacroCommand::Unexecute( void )
{
	bool succeded = !m_subCommands.empty();

	for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( !( *itSubCmd )->Unexecute() )
			succeded = false;

	return succeded;
}

bool CMacroCommand::IsUndoable( void ) const
{
	for ( std::vector< utl::ICommand* >::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( ( *itSubCmd )->IsUndoable() )
			return true;

	return false;
}


// CCommandModel implementation

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

	while ( stepCount != 0 && !m_undoStack.empty() )
	{
		utl::ICommand* pCmd = m_undoStack.back();

		if ( pCmd->IsUndoable() )
		{
			succeeded = pCmd->Unexecute();
			--stepCount;
		}

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

	while ( stepCount-- > 0 && !m_redoStack.empty() )
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
