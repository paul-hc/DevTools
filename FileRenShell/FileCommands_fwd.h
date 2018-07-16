#ifndef FileCommands_fwd_h
#define FileCommands_fwd_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace fs { class CPath; }


namespace cmd
{
	enum CommandType
	{
		RenameFile = 100, TouchFile,
		ChangeDestPaths, ChangeDestFileStates, ResetDestinations,
		EditOptions
	};

	const CEnumTags& GetTags_CommandType( void );


	enum StackType { Undo, Redo };

	const CEnumTags& GetTags_StackType( void );


	bool IsPersistentCmd( const utl::ICommand* pCmd );		// persistent commands are also editor-specific file action commands


	interface IErrorObserver
	{
		virtual void ClearFileErrors( void ) = 0;
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
	};

	enum FileFormat { TextFormat, BinaryFormat };
}


#endif // FileCommands_fwd_h
