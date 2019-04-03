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
		RenameFile = 100, TouchFile, FindDuplicates,
		DeleteFiles, Priv_UndeleteFiles,
		ChangeDestPaths, ChangeDestFileStates, ResetDestinations,
		EditOptions
	};

	const CEnumTags& GetTags_CommandType( void );


	bool IsPersistentCmd( const utl::ICommand* pCmd );		// persistent commands are also editor-specific file action commands
	bool IsZombieCmd( const utl::ICommand* pCmd );			// empty macro file action command with no effect?


	interface IErrorObserver
	{
		virtual void ClearFileErrors( void ) = 0;
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
	};

	enum FileFormat { TextFormat, BinaryFormat };
}


namespace pred
{
	struct IsZombieCmd
	{
		bool operator()( const utl::ICommand* pCmd )
		{
			return cmd::IsZombieCmd( pCmd );
		}
	};
}


#endif // FileCommands_fwd_h
