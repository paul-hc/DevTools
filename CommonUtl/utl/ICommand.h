#ifndef ICommand_h
#define ICommand_h
#pragma once

#include "ISubject.h"


class CEnumTags;


namespace utl
{
	enum ExecMode { ExecDo, ExecUndo, ExecRedo };

	const CEnumTags& GetTags_ExecMode( void );


	interface ICommand
		: public utl::IMemoryManaged
		, public IMessage
	{
		virtual bool Execute( void ) = 0;
		virtual bool Unexecute( void ) = 0;
		virtual bool IsUndoable( void ) const = 0;
	};

	interface ICommandExecutor
	{
		virtual bool Execute( ICommand* pCmd ) = 0;
	};
}


namespace svc
{
	enum StackType { Undo, Redo };

	const CEnumTags& GetTags_StackType( void );


	interface ICommandService : public utl::ICommandExecutor
	{
		virtual size_t FindCmdTopPos( svc::StackType stackType, utl::ICommand* pCmd ) const = 0;
		virtual utl::ICommand* PeekCmd( svc::StackType stackType ) const = 0;
		virtual bool CanUndoRedo( svc::StackType stackType, int typeId = 0 ) const = 0;
		virtual bool UndoRedo( svc::StackType stackType ) = 0;
		virtual bool Execute( utl::ICommand* pCmd ) = 0;
		virtual bool SafeExecuteCmd( utl::ICommand* pCmd, bool execInline = false ) = 0;

		template< typename CmdType >
		CmdType* PeekCmdAs( svc::StackType stackType ) const { return dynamic_cast<CmdType*>( PeekCmd( stackType ) ); }
	};
}


#endif // ICommand_h
