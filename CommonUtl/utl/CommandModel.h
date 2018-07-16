#ifndef CommandModel_h
#define CommandModel_h
#pragma once

#include "ISubject.h"
#include <deque>


class CCommandModel : public utl::ICommandExecutor
					, private utl::noncopyable
{
public:
	CCommandModel( void ) {}
	~CCommandModel();

	bool IsUndoEmpty( void ) const { return m_undoStack.empty(); }
	void Clear( void );
	void RemoveExpiredCommands( size_t maxSize );

	template< typename FuncType >
	void RemoveCommandsThat( FuncType func );

	// utl::ICommandExecutor interface
	virtual bool Execute( utl::ICommand* pCmd );

	void Push( utl::ICommand* pCmd );					// already executed by the caller

	bool Undo( size_t stepCount = 1 );
	bool Redo( size_t stepCount = 1 );

	bool CanUndo( void ) const;
	bool CanRedo( void ) const;

	utl::ICommand* PeekUndo( void ) const { return !m_undoStack.empty() ? m_undoStack.back() : NULL; }
	utl::ICommand* PeekRedo( void ) const { return !m_redoStack.empty() ? m_redoStack.back() : NULL; }

	const std::deque< utl::ICommand* >& GetUndoStack( void ) const { return m_undoStack; }
	const std::deque< utl::ICommand* >& GetRedoStack( void ) const { return m_redoStack; }

	void SwapUndoStack( std::deque< utl::ICommand* >& rUndoStack ) { m_undoStack.swap( rUndoStack ); }
	void SwapRedoStack( std::deque< utl::ICommand* >& rRedoStack ) { m_redoStack.swap( rRedoStack ); }
private:
	// commands stored in UNDO and REDO must keep their objects alive
	std::deque< utl::ICommand* > m_undoStack;			// stack top at end
	std::deque< utl::ICommand* > m_redoStack;			// stack top at end
};


// template code

template< typename FuncType >
inline void CCommandModel::RemoveCommandsThat( FuncType func )
{
	for ( std::deque< utl::ICommand* >::iterator itCmd = m_undoStack.begin(); itCmd != m_undoStack.end(); )
		if ( func( *itCmd ) )
		{
			delete *itCmd;
			itCmd = m_undoStack.erase( itCmd );
		}
		else
			++itCmd;

	for ( std::deque< utl::ICommand* >::iterator itCmd = m_redoStack.begin(); itCmd != m_redoStack.end(); )
		if ( func( *itCmd ) )
		{
			delete *itCmd;
			itCmd = m_redoStack.erase( itCmd );
		}
		else
			++itCmd;
}


#endif // CommandModel_h
