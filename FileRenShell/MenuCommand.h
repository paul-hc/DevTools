#ifndef MenuCommand_h
#define MenuCommand_h
#pragma once


enum MenuCommand
{
	Cmd_RenameFiles, Cmd_SendToCliboard,

	// sub-menu additional commands:
	Cmd_RenameAndCopy, Cmd_RenameAndCapitalize, Cmd_RenameAndLowCaseExt, Cmd_RenameAndReplace, Cmd_RenameAndReplaceDelims,
	Cmd_RenameAndSingleWhitespace, Cmd_RenameAndRemoveWhitespace, Cmd_RenameAndDashToSpace, Cmd_RenameAndUnderbarToSpace, Cmd_RenameAndSpaceToUnderbar,
	Cmd_UndoRename,
		_CmdCount,
		_CmdFirstSubMenu = Cmd_RenameAndCopy
};


#endif // MenuCommand_h
