#ifndef FileCommands_fwd_h
#define FileCommands_fwd_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace fs { class CPath; }


namespace cmd
{
	enum CommandType { RenameFile = 100, TouchFile, DestPathChanged };

	const CEnumTags& GetTags_CommandType( void );


	enum StackType { Undo, Redo };

	const CEnumTags& GetTags_StackType( void );


	interface IErrorObserver
	{
		virtual void ClearFileErrors( void ) = 0;
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
	};
}


#endif // FileCommands_fwd_h
