#ifndef ICommandService_h
#define ICommandService_h
#pragma once

#include "utl/ISubject.h"


class CEnumTags;


namespace svc
{
	enum StackType { Undo, Redo };

	const CEnumTags& GetTags_StackType( void );


	interface ICommandService : public utl::ICommandExecutor
	{
		virtual utl::ICommand* PeekCmd( svc::StackType stackType ) const = 0;
		virtual bool CanUndoRedo( svc::StackType stackType, int typeId = 0 ) const = 0;
		virtual bool UndoRedo( svc::StackType stackType ) = 0;
		virtual bool Execute( utl::ICommand* pCmd ) = 0;

		template< typename CmdType >
		CmdType* PeekCmdAs( svc::StackType stackType ) const { return dynamic_cast< CmdType* >( PeekCmd( stackType ) ); }
	};
}


#endif // ICommandService_h
