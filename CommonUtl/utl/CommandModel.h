#ifndef CommandModel_h
#define CommandModel_h
#pragma once

#include "ICommand.h"
#include "Serialization_fwd.h"
#include <deque>


struct CScopedExecMode
{
	CScopedExecMode( utl::ExecMode execMode );
	~CScopedExecMode();
public:
	utl::ExecMode m_oldExecMode;
};


namespace func
{
	template< typename CmdT, typename HostT >
	struct SetCmdHost			// for convenient batch host re-assignment
	{
		SetCmdHost( HostT* pHost ) { m_pHost = pHost; ASSERT_PTR( m_pHost ); }

		void operator()( utl::ICommand* pCmd ) const
		{
			CmdT* pCommand = checked_static_cast<CmdT*>( pCmd );
			pCommand->SetHost( m_pHost );
		}
	private:
		HostT* m_pHost;
	};
}


class CCommandModel : public utl::ICommandExecutor
					, public serial::ISerializable
					, private utl::noncopyable
{
	friend struct CScopedExecMode;
public:
	CCommandModel( void ) {}
	~CCommandModel();

	bool IsUndoEmpty( void ) const { return m_undoStack.empty(); }
	bool IsRedoEmpty( void ) const { return m_redoStack.empty(); }
	void Clear( void );

	static utl::ExecMode GetExecMode( void ) { return s_execMode; }
	static std::tstring PrefixExecMessage( const std::tstring& message );		// prefixes the message with the current ExecMode (do/undo/redo)

	// utl::ICommandExecutor interface
	virtual bool Execute( utl::ICommand* pCmd ) implement;

	void Push( utl::ICommand* pCmd );					// already executed by the caller

	bool Undo( size_t stepCount = 1 );
	bool Redo( size_t stepCount = 1 );

	bool CanUndo( void ) const;
	bool CanRedo( void ) const;

	utl::ICommand* PeekUndo( void ) const { return !m_undoStack.empty() ? m_undoStack.back() : nullptr; }
	utl::ICommand* PeekRedo( void ) const { return !m_redoStack.empty() ? m_redoStack.back() : nullptr; }

	const std::deque<utl::ICommand*>& GetUndoStack( void ) const { return m_undoStack; }
	const std::deque<utl::ICommand*>& GetRedoStack( void ) const { return m_redoStack; }

	void SwapUndoStack( std::deque<utl::ICommand*>& rUndoStack ) { m_undoStack.swap( rUndoStack ); }
	void SwapRedoStack( std::deque<utl::ICommand*>& rRedoStack ) { m_redoStack.swap( rRedoStack ); }

	// persistence
	virtual void Serialize( CArchive& archive ) implement;

	template< typename CmdT, typename HostT >
	void ReHostCommands( HostT* pHost );			// called after loading, to update the host interface pointer (parent) of each persistent command

	// commands removal
	void RemoveExpiredCommands( size_t maxSize );

	template< typename PredT >
	void RemoveCommandsThat( PredT pred );
private:
	template< typename PredT >
	static void RemoveStackCommandsThat( std::deque<utl::ICommand*>& rStack, PredT pred );
private:
	// commands stored in UNDO and REDO must keep their objects alive
	persist std::deque<utl::ICommand*> m_undoStack;			// stack top at end
	persist std::deque<utl::ICommand*> m_redoStack;			// stack top at end

	static utl::ExecMode s_execMode;
};


// template code

template< typename CmdT, typename HostT >
void CCommandModel::ReHostCommands( HostT* pHost )
{	// called after loading, to update the host interface pointer (parent) of each persistent command
	func::SetCmdHost<CmdT, HostT> setCmdHost( pHost );

	std::for_each( m_undoStack.begin(), m_undoStack.end(), setCmdHost );
	std::for_each( m_redoStack.begin(), m_redoStack.end(), setCmdHost );
}

template< typename PredT >
void CCommandModel::RemoveStackCommandsThat( std::deque<utl::ICommand*>& rStack, PredT pred )
{
	for ( std::deque<utl::ICommand*>::iterator itCmd = rStack.begin(); itCmd != rStack.end(); )
		if ( pred( *itCmd ) )
		{
			delete *itCmd;
			itCmd = rStack.erase( itCmd );
		}
		else
			++itCmd;
}

template< typename PredT >
inline void CCommandModel::RemoveCommandsThat( PredT pred )
{
	RemoveStackCommandsThat( m_undoStack, pred );
	RemoveStackCommandsThat( m_redoStack, pred );
}


#endif // CommandModel_h
