#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CEnumTags;


namespace app
{
	enum MenuCommand
	{
		Cmd_RenameFiles, Cmd_SendToCliboard, Cmd_TouchFiles,
		Cmd_RunUnitTests,

		// sub-menu additional commands:
		Cmd_RenameAndCopy, Cmd_RenameAndCapitalize, Cmd_RenameAndLowCaseExt, Cmd_RenameAndReplace, Cmd_RenameAndReplaceDelims,
		Cmd_RenameAndSingleWhitespace, Cmd_RenameAndRemoveWhitespace, Cmd_RenameAndDashToSpace, Cmd_RenameAndUnderbarToSpace, Cmd_RenameAndSpaceToUnderbar,
		Cmd_UndoRename,
			_CmdCount,
			_CmdFirstSubMenu = Cmd_RenameAndCopy
	};


	enum Action { RenameFiles, TouchFiles };

	const CEnumTags& GetTags_Action( void );
}


#endif // Application_fwd_h
