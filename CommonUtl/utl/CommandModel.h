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

	// commands removal
	void RemoveExpiredCommands( size_t maxSize );

	template< typename PredType >
	void RemoveCommandsThat( PredType pred );
private:
	template< typename PredType >
	static void RemoveStackCommandsThat( std::deque< utl::ICommand* >& rStack, PredType pred );
private:
	// commands stored in UNDO and REDO must keep their objects alive
	std::deque< utl::ICommand* > m_undoStack;			// stack top at end
	std::deque< utl::ICommand* > m_redoStack;			// stack top at end
};


// template code

template< typename PredType >
void CCommandModel::RemoveStackCommandsThat( std::deque< utl::ICommand* >& rStack, PredType pred )
{
	for ( std::deque< utl::ICommand* >::iterator itCmd = rStack.begin(); itCmd != rStack.end(); )
		if ( pred( *itCmd ) )
		{
			delete *itCmd;
			itCmd = rStack.erase( itCmd );
		}
		else
			++itCmd;
}

template< typename PredType >
inline void CCommandModel::RemoveCommandsThat( PredType pred )
{
	RemoveStackCommandsThat( m_undoStack, pred );
	RemoveStackCommandsThat( m_redoStack, pred );
}


#endif // CommandModel_h