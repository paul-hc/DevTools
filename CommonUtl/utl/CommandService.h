#ifndef CommandService_h
#define CommandService_h
#pragma once

#include "ICommand.h"


class CCommandModel;


class CCommandService : public svc::ICommandService
{
public:
	CCommandService( void );
	~CCommandService();

	const CCommandModel* GetCommandModel( void ) const { return m_pCommandModel.get(); }

	bool IsDirty( void ) const { return m_dirty; }
	void SetDirty( void ) { m_dirty = true; }		// use with care, called in very special cases

	// svc::ICommandService interface
	virtual utl::ICommand* PeekCmd( svc::StackType stackType ) const;
	virtual bool CanUndoRedo( svc::StackType stackType, int cmdTypeId = 0 ) const;
	virtual bool UndoRedo( svc::StackType stackType );
	virtual bool Execute( utl::ICommand* pCmd );

	template< typename PredType >
	void RemoveCommandsThat( PredType pred )
	{
		m_pCommandModel->RemoveCommandsThat( pred );
		m_dirty = true;
	}
protected:
	void SetDirty( bool dirty = true ) { m_dirty = dirty; }
protected:
	std::auto_ptr< CCommandModel > m_pCommandModel;		// self-encapsulated
private:
	bool m_dirty;
};


#endif // CommandService_h
