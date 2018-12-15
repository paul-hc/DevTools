#ifndef CommandService_h
#define CommandService_h
#pragma once

#include "utl/ICommandService.h"
#include "Application_fwd.h"


class CCommandModel;


class CCommandService : public svc::ICommandService
{
public:
	CCommandService( void );
	~CCommandService();

	const CCommandModel* GetCommandModel( void ) const { return m_pCommandModel.get(); }

	bool IsDirty( void ) const { return m_dirty; }
	bool SaveCommandModel( void );
	bool LoadCommandModel( void );

	// svc::ICommandService interface
	virtual utl::ICommand* PeekCmd( svc::StackType stackType ) const;
	virtual bool CanUndoRedo( svc::StackType stackType, int typeId = 0 ) const;
	virtual bool UndoRedo( svc::StackType stackType );
	virtual bool Execute( utl::ICommand* pCmd );

	template< typename PredType >
	void RemoveCommandsThat( PredType pred );
private:
	std::auto_ptr< CCommandModel > m_pCommandModel;		// self-encapsulated
	bool m_dirty;

	enum { MaxCommands = 60 };
};


// CCommandService template code

template< typename PredType >
void CCommandService::RemoveCommandsThat( PredType pred )
{
	m_pCommandModel->RemoveCommandsThat( pred );
	m_dirty = true;
}


#endif // CommandService_h
