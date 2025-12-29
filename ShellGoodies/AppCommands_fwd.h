#ifndef AppCommands_fwd_h
#define AppCommands_fwd_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace fs { class CPath; }


namespace cmd
{
	enum CommandType
	{
		// persistent commands - don't modify their values, so that it won't break serialization
		RenameFile = 100, TouchFile, FindDuplicates,
		DeleteFiles, CopyFiles, PasteCopyFiles, MoveFiles, PasteMoveFiles, CreateFolders, PasteCreateFolders, PasteCreateDeepFolders,
		CopyPasteFilesAsBackup, CutPasteFilesAsBackup,

		// transient commands (not persistent)
		ChangeDestPaths, ChangeDestFileStates, ResetDestinations,
		EditOptions,
		SortRenameList, OnRenameListSelChanged,

		Priv_UndeleteFiles
	};

	const CEnumTags& GetTags_CommandType( void );


	enum FileFormat { BinaryFormat };


	interface IFileDetailsCmd
	{
		virtual size_t GetFileCount( void ) const = 0;
		virtual void QueryDetailLines( std::vector<std::tstring>& rLines ) const = 0;
	};

	interface IPersistentCmd : public IFileDetailsCmd
	{
		virtual bool IsValid( void ) const = 0;
		virtual const CTime& GetTimestamp( void ) const = 0;
	};

	interface IErrorObserver
	{
		virtual void ClearFileErrors( void ) = 0;
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
	};
}


namespace cmd
{
	bool IsPersistentCmd( const utl::ICommand* pCmd );		// some persistent commands are also editor-specific file action commands
	bool IsZombieCmd( const utl::ICommand* pCmd );			// empty macro file action command with no effect?
}


namespace pred
{
	struct IsPersistentCmd
	{
		typedef utl::ICommand* argument_type;		// required by std::not1()

		bool operator()( const utl::ICommand* pCmd ) const
		{
			return cmd::IsPersistentCmd( pCmd );
		}
	};

	struct IsZombieCmd
	{
		bool operator()( const utl::ICommand* pCmd ) const
		{
			return cmd::IsZombieCmd( pCmd );
		}
	};
}


#endif // AppCommands_fwd_h
