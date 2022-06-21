#ifndef IFileEditor_h
#define IFileEditor_h
#pragma once

#include "utl/ISubject.h"
#include "utl/ICommand.h"
#include "AppCommands_fwd.h"


class CFileModel;


interface IFileEditor : public utl::IMemoryManaged
	, public utl::IObserver
	, public cmd::IErrorObserver
{
	virtual CFileModel* GetFileModel( void ) const = 0;
	virtual CDialog* GetDialog( void ) = 0;
	virtual bool IsRollMode( void ) const = 0;						// about to undo/redo mode?
	virtual void PostMakeDest( bool silent = false ) = 0;

	virtual void PopStackTop( svc::StackType stackType ) = 0;
};


#endif // IFileEditor_h
