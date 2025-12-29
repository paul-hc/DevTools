#ifndef CommandService_h
#define CommandService_h
#pragma once

#include "CommandModel.h"


// service that implements application-wide command execution with Undo/Redo support

class CCommandService : public svc::ICommandService
{
public:
	CCommandService( void );
	~CCommandService();

	const CCommandModel* GetCommandModel( void ) const { return m_pCommandModel.get(); }

	bool IsDirty( void ) const { return m_dirty; }
	void SetDirty( void ) { m_dirty = true; }		// use with care, called in very special cases

	// svc::ICommandService interface
	virtual size_t FindCmdTopPos( svc::StackType stackType, utl::ICommand* pCmd ) const implement;
	virtual utl::ICommand* PeekCmd( svc::StackType stackType ) const implement;
	virtual bool CanUndoRedo( svc::StackType stackType, int cmdTypeId = 0 ) const implement;
	virtual bool UndoRedo( svc::StackType stackType ) implement;
	virtual bool Execute( utl::ICommand* pCmd ) implement;

	template< typename PredT >
	void RemoveCommandsThat( PredT pred )
	{
		m_pCommandModel->RemoveCommandsThat( pred );
		m_dirty = true;
	}

	// mutable acces (for special cases)
	std::deque<utl::ICommand*>& RefUndoStack( void ) { return const_cast< std::deque<utl::ICommand*>& >( GetCommandModel()->GetUndoStack() ); }
	std::deque<utl::ICommand*>& RefRedoStack( void ) { return const_cast< std::deque<utl::ICommand*>& >( GetCommandModel()->GetRedoStack() ); }
protected:
	void SetDirty( bool dirty = true ) { m_dirty = dirty; }
protected:
	std::auto_ptr<CCommandModel> m_pCommandModel;		// self-encapsulated
private:
	bool m_dirty;
};


#endif // CommandService_h
