#ifndef IFileEditor_h
#define IFileEditor_h
#pragma once

#include "utl/ISubject.h"
#include "FileCommands_fwd.h"


class CFileModel;


interface IFileEditor : public IMemoryManaged
					  , public utl::IObserver
					  , public cmd::IErrorObserver
{
	virtual CFileModel* GetFileModel( void ) const = 0;
	virtual CDialog* GetDialog( void ) = 0;
	virtual void PostMakeDest( bool silent = false ) = 0;

	virtual void PopUndoRedoTop( cmd::UndoRedo undoRedo ) = 0;
};


#endif // IFileEditor_h
