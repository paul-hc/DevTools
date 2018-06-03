#ifndef FileCommands_fwd_h
#define FileCommands_fwd_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace fs { class CPath; }


namespace cmd
{
	enum Command { RenameFile = 100, TouchFile };

	const CEnumTags& GetTags_Command( void );


	interface IErrorObserver
	{
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
		virtual void ClearFileErrors( void ) = 0;
	};
}


#endif // FileCommands_fwd_h
